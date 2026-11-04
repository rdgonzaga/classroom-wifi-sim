# classroom-wifi-sim

ns-3 simulation for the NSCOM03 (Data Communications, DLSU) Network Simulation Study, Group 5.

We're simulating Wi-Fi in a crowded classroom: 5 to 50 students on one access point, based on DLSU's campus Wi-Fi.

## Research question

How do per-student throughput and fairness degrade as more students join one AP, and does Wi-Fi 6 (802.11ax) handle it better than Wi-Fi 5 (802.11ac)?

1. At what number of students does per-student throughput fall below a usable level?
2. Does one student far from the AP slow everyone down (the 802.11 performance anomaly)?

## Current state

`classroom-wifi.cc` is still an unmodified copy of ns-3's `examples/wireless/wifi-he-network.cc`. It only reports total throughput, only does Wi-Fi 6, and uses a fixed MCS. Per-student measurement and the Wi-Fi 5 comparison aren't in yet.

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
./ns3 run "classroom-wifi --nStations=30 --mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=2s"
```

That's downlink (AP sending to students). Add `--downlink=0` for uplink (students sending to the AP).

Re-copy the file into `scratch/` every time you change it.

## Layout

```
classroom-wifi.cc   simulation script (copy into ~/ns-3/scratch/ to run)
results/            recorded results, one markdown file per set of runs
```

See `results/baseline.md` for the first numbers.
