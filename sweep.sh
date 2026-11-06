#!/usr/bin/env bash
# ax and ac, downlink and uplink, plus ax DL OFDMA, RUNS runs each. Appends to OUT_DIR.
# Env: NS3_DIR, SIM_TIME, RUNS, DEMAND (Mbit/s, 0 = saturated), STUDENTS, LAYOUT, OUT_DIR.
set -e

NS3_DIR=${NS3_DIR:-$HOME/ns-3}
SIM_TIME=${SIM_TIME:-10s}
RUNS=${RUNS:-5}
DEMAND=${DEMAND:-0}
STUDENTS=${STUDENTS:-"5 10 20 30 40 50"}
LAYOUT=${LAYOUT:-circle}
REPO_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR=${OUT_DIR:-$REPO_DIR/results}

# fixed seed so reruns match
SEED=1

COMMON="--mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=$SIM_TIME --RngSeed=$SEED"
COMMON="$COMMON --demand=$DEMAND --layout=$LAYOUT"
COMMON="$COMMON --csv=$OUT_DIR/runs.csv --perStudentCsv=$OUT_DIR/per-student.csv"

cp "$REPO_DIR/classroom-wifi.cc" "$NS3_DIR/scratch/"
cd "$NS3_DIR"
./ns3 build scratch/classroom-wifi

for run in $(seq 1 "$RUNS"); do
    for standard in ax ac; do
        for downlink in 1 0; do
            for n in $STUDENTS; do
                ./ns3 run --no-build "classroom-wifi --standard=$standard --downlink=$downlink --nStations=$n --RngRun=$run $COMMON" |
                    grep -v "^  Student"
            done
        done
    done

    # UL OFDMA needs a 1600 ns GI, so it's left out
    for n in $STUDENTS; do
        ./ns3 run --no-build "classroom-wifi --standard=ax --downlink=1 --ofdma=1 --nStations=$n --RngRun=$run $COMMON" |
            grep -v "^  Student"
    done
done

echo "Done. Results appended to $OUT_DIR/runs.csv and $OUT_DIR/per-student.csv"
