#!/usr/bin/env bash
# Option B sweeps: phone + laptop device mix, plus the 2x2 identical control. Runs jobs in
# parallel; each job writes its own CSVs, which are merged per set at the end.
# Env: NS3_DIR, SIM_TIME, RUNS, JOBS (parallel runs), SETS, OUT_DIR.
set -e

NS3_DIR=${NS3_DIR:-$HOME/ns-3}
SIM_TIME=${SIM_TIME:-10s}
RUNS=${RUNS:-5}
JOBS=${JOBS:-12}
SETS=${SETS:-"mixed-p1 mixed-p05 mixed-saturated control-demand3 control-saturated mixed-far control-far"}
REPO_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR=${OUT_DIR:-$REPO_DIR/results/final-mixed}

DEMAND_N="5 10 15 20 25 30 35 40 45 50"
SAT_N="5 10 20 30 40 50"
BASE="--mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=$SIM_TIME --RngSeed=1 --layout=grid"
MIXED="--devices=mixed"
CONTROL="--devices=identical --apStreams=2 --staStreams=2"

cp "$REPO_DIR/classroom-wifi.cc" "$NS3_DIR/scratch/"
cd "$NS3_DIR"
./ns3 build scratch/classroom-wifi

# prints "set|args" per run: ax/ac DL/UL, plus ax DL OFDMA when ofdma=1.
# mixed sets skip OFDMA: ns-3.48 hits an NS_ASSERT (WifiPhyStateHelper::SwitchFromRxEndOk)
# when one DL MU PPDU carries both 1-stream and 2-stream users
configs() {
    local set=$1 counts=$2 extra=$3 ofdma=$4
    local cfgs=("--standard=ax --downlink=1" "--standard=ax --downlink=0"
        "--standard=ac --downlink=1" "--standard=ac --downlink=0")
    [ "$ofdma" = 1 ] && cfgs+=("--standard=ax --downlink=1 --ofdma=1")
    for run in $(seq 1 "$RUNS"); do
        for n in $counts; do
            for cfg in "${cfgs[@]}"; do
                printf '%s|%s\n' "$set" "$cfg --nStations=$n --RngRun=$run $extra"
            done
        done
    done
}

# 10 students, downlink, saturated, Ideal rate manager, last student moved out
far() {
    local set=$1 extra=$2
    for run in $(seq 1 "$RUNS"); do
        for standard in ax ac; do
            for d in 0 10 20 30 40 50; do
                printf '%s|%s\n' "$set" "--standard=$standard --downlink=1 --nStations=10 --rateManager=ideal --farDistance=$d --RngRun=$run $extra"
            done
        done
    done
}

jobs() {
    for set in $SETS; do
        case $set in
        mixed-p1) configs "$set" "$DEMAND_N" "$MIXED" 0 ;;
        mixed-p05) configs "$set" "$DEMAND_N" "$MIXED --laptopProb=0.5" 0 ;;
        mixed-saturated) configs "$set" "$SAT_N" "$MIXED --laptopDemand=0 --phoneDemand=0" 0 ;;
        control-demand3) configs "$set" "$DEMAND_N" "$CONTROL --demand=3" 1 ;;
        control-saturated) configs "$set" "$SAT_N" "$CONTROL" 1 ;;
        mixed-far) far "$set" "$MIXED --laptopDemand=0 --phoneDemand=0" ;;
        control-far) far "$set" "$CONTROL" ;;
        *) echo "Unknown set: $set" >&2; exit 1 ;;
        esac
    done
}

run_one() {
    local set=$1 args=$2
    local id
    id=$(echo "$args" | md5sum | cut -c1-12)
    mkdir -p "$OUT_DIR/$set/parts"
    # binary directly: parallel "./ns3 run" calls would all re-check the build
    # shellcheck disable=SC2086
    "$BIN" $args $BASE --csv="$OUT_DIR/$set/parts/$id-runs.csv" \
        --perStudentCsv="$OUT_DIR/$set/parts/$id-ps.csv" >/dev/null 2>"$OUT_DIR/$set/parts/$id.err" ||
        { echo "FAILED $set: $args" | tee -a "$OUT_DIR/failed.txt"; return 0; }
    rm -f "$OUT_DIR/$set/parts/$id.err"
    echo "$set: $args"
}
export -f run_one
BIN=$(ls "$NS3_DIR"/build/scratch/ns3.*-classroom-wifi-default)
export OUT_DIR BASE BIN

total=$(jobs | wc -l)
echo "$total runs, $JOBS at a time"
jobs | xargs -d '\n' -P "$JOBS" -I{} bash -c 'run_one "${1%%|*}" "${1#*|}"' _ {}

# merge parts: one header, rows sorted for stable diffs
for set in $SETS; do
    for kind in runs ps; do
        name=$([ $kind = runs ] && echo runs.csv || echo per-student.csv)
        files=("$OUT_DIR/$set/parts/"*-$kind.csv)
        { head -1 "${files[0]}"; tail -q -n +2 "${files[@]}" | sort; } >"$OUT_DIR/$set/$name"
    done
    # keep the .err files of failed runs
    rm "$OUT_DIR/$set/parts/"*.csv
    rmdir --ignore-fail-on-non-empty "$OUT_DIR/$set/parts"
done

echo "Done. Results in $OUT_DIR/<set>/runs.csv and per-student.csv"
