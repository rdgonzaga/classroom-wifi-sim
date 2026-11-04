# classroom-wifi-sim

ns-3 simulation for the NSCOM03 (Data Communications, DLSU) Network Simulation Study, Group 5.

We're simulating Wi-Fi in a crowded classroom: 5 to 50 students on one access point, based on DLSU's campus Wi-Fi.

## Research question

How do per-student throughput and fairness degrade as more students join one AP, and does Wi-Fi 6 (802.11ax) handle it better than Wi-Fi 5 (802.11ac)?

1. At what number of students does per-student throughput fall below a usable level?
2. Does one student far from the AP slow everyone down (the 802.11 performance anomaly)?

## Current state

`classroom-wifi.cc` started as a copy of ns-3's `examples/wireless/wifi-he-network.cc` and now runs one configuration per execution. It supports 802.11ax and 802.11ac on 5 GHz, measures each student's throughput with FlowMonitor, computes Jain's fairness index, and can move one student far from the AP.

OFDMA is off by default. It can be turned on with `--ofdma` (downlink) and `--ulOfdma` (uplink).

## Setup

We use ns-3.48 on Ubuntu (WSL works fine).

```sh
git clone https://gitlab.com/nsnam/ns-3-dev.git ~/ns-3
cd ~/ns-3
git checkout -b ns-3.48-branch ns-3.48
./ns3 configure --enable-examples --enable-tests
./ns3 build
```

The first build takes a while.

## Running

Copy the script into ns-3's `scratch/` folder, then run it from `~/ns-3`:

```sh
cp classroom-wifi.cc ~/ns-3/scratch/
cd ~/ns-3
./ns3 run "classroom-wifi --standard=ax --nStations=30 --mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=2s"
```

Re-copy the file into `scratch/` every time you change it.

Options:

| Option | Default | Meaning |
|---|---|---|
| `--standard` | `ax` | `ax` (Wi-Fi 6) or `ac` (Wi-Fi 5) |
| `--nStations` | `1` | number of students |
| `--downlink` | `1` | `1` = AP sends to students, `0` = students send to AP |
| `--udp` | `1` | `1` = UDP, `0` = TCP |
| `--rateManager` | `constant` | `constant` (fixed `--mcs`), `minstrel` or `ideal` |
| `--mcs` | `5` | MCS for the constant rate manager (ax: 0-11, ac: 0-9) |
| `--channelWidth` | `20` | MHz: 20, 40, 80 or 160 |
| `--guardInterval` | `800` | ns. ax: 800/1600/3200, ac: 400/800 |
| `--distance` | `1` | meters from each student to the AP |
| `--farDistance` | `0` | if set, one student sits this far away instead |
| `--ofdma` | `0` | `1` = DL OFDMA on (ax only, uses the Spectrum PHY) |
| `--ulOfdma` | `0` | `1` = UL OFDMA on too (ax only, needs `--guardInterval=1600` or `3200`) |
| `--muSchedAccessReqInterval` | `3ms` | with UL OFDMA, how often the AP grabs the channel to send trigger frames |
| `--phyModel` | `Yans` | `Yans` or `Spectrum`. OFDMA always uses Spectrum |
| `--simulationTime` | `10s` | simulated time |
| `--csv` | `results/runs.csv` | summary row is appended here |
| `--perStudentCsv` | `results/per-student.csv` | one row per student is appended here |

Relative CSV paths are relative to where you run `./ns3` (so `~/ns-3/results/` by default). Pass full paths to write into this repo.

For the far-student test, use `--rateManager=ideal` or `minstrel`. With a fixed MCS the far student can't drop to a slower rate. On the default channel model a student past about 50 m is out of range, and the whole network stalls.

## Sweep

```sh
./sweep.sh
```

Runs 5, 10, 20, 30, 40 and 50 students for both standards and both directions, plus Wi-Fi 6 downlink with OFDMA (30 runs, fixed seed). Results are appended to `results/runs.csv` and `results/per-student.csv`. It expects ns-3 at `~/ns-3`. Set `NS3_DIR` or `SIM_TIME` to change that.

## Layout

```
classroom-wifi.cc   simulation script (copy into ~/ns-3/scratch/ to run)
sweep.sh            runs the main student-count sweep
results/            recorded results
```

Results so far:

- `results/baseline.md`: the unmodified example's numbers
- `results/first-runs.md`: test runs of the experiment script (baseline check, Wi-Fi 6 vs 5, far student)
- `results/ofdma.md`: test runs with OFDMA on
