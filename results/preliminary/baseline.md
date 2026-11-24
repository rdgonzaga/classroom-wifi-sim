# Baseline results

Recorded Nov 4, 2026 using the unmodified `classroom-wifi.cc` (copy of `wifi-he-network.cc`).

Settings: Wi-Fi 6 (802.11ax), MCS 5, 20 MHz channel, 800 ns guard interval, UDP, 2 s simulated.

| Direction | Students | Total throughput (Mbit/s) | Per student, approx. (Mbit/s) |
|-----------|---------:|--------------------------:|------------------------------:|
| Downlink  | 10       | 59.7212                   | 5.97                          |
| Downlink  | 30       | 57.5652                   | 1.92                          |
| Uplink    | 30       | 27.9216                   | 0.93                          |

The per-student column is just total divided by the number of students. The script doesn't measure each student separately yet, so it says nothing about fairness.

Commands used (from `~/ns-3`):

```sh
./ns3 run "classroom-wifi --nStations=10 --mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=30 --mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=30 --mcs=5 --channelWidth=20 --guardInterval=800 --simulationTime=2s --downlink=0"
```
