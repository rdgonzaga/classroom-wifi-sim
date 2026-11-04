#!/usr/bin/env bash
# 5 to 50 students, ax and ac, downlink and uplink, plus ax downlink with OFDMA.
# Each setting is run RUNS times with a different --RngRun, so results can be averaged.
# Results get appended to results/runs.csv and results/per-student.csv.
# Set NS3_DIR if ns-3 isn't at ~/ns-3, SIM_TIME to change the run length, RUNS for the
# number of runs per setting.
set -e

NS3_DIR=${NS3_DIR:-$HOME/ns-3}
SIM_TIME=${SIM_TIME:-10s}
RUNS=${RUNS:-5}
REPO_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR=${OUT_DIR:-$REPO_DIR/results}

# fixed seed, only the run number changes, so reruns give the same numbers
SEED=1

COMMON="--mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=$SIM_TIME --RngSeed=$SEED"
COMMON="$COMMON --csv=$OUT_DIR/runs.csv --perStudentCsv=$OUT_DIR/per-student.csv"

cp "$REPO_DIR/classroom-wifi.cc" "$NS3_DIR/scratch/"
cd "$NS3_DIR"
./ns3 build scratch/classroom-wifi

for run in $(seq 1 "$RUNS"); do
    for standard in ax ac; do
        for downlink in 1 0; do
            for n in 5 10 20 30 40 50; do
                ./ns3 run --no-build "classroom-wifi --standard=$standard --downlink=$downlink --nStations=$n --RngRun=$run $COMMON" |
                    grep -v "^  Student"
            done
        done
    done

    # Wi-Fi 6 with DL OFDMA (UL OFDMA needs a 1600 ns GI, so it's not part of this sweep)
    for n in 5 10 20 30 40 50; do
        ./ns3 run --no-build "classroom-wifi --standard=ax --downlink=1 --ofdma=1 --nStations=$n --RngRun=$run $COMMON" |
            grep -v "^  Student"
    done
done

echo "Done. Results appended to $OUT_DIR/runs.csv and $OUT_DIR/per-student.csv"
