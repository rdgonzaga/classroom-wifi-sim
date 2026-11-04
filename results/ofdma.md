# OFDMA Test Runs

Date: November 4, 2026

OFDMA (orthogonal frequency-division multiple access) is a Wi-Fi 6 feature. It lets the AP split one channel into smaller parts called resource units (RUs) and serve several students in the same transmission. Wi-Fi 5 does not have it. These runs check what OFDMA does in our scenario. Each run is 2 seconds of simulated time, so these are preliminary results.

## Settings

Same as `first-runs.md` unless stated: 5 GHz, 20 MHz, MCS 5 (constant rate manager), UDP with 700-byte packets, students 1 m from the AP, seed 1 run 1.

OFDMA in ns-3 requires the Spectrum PHY model, while the other runs use the Yans PHY model. To separate the effect of OFDMA from the effect of the PHY model, Wi-Fi 6 was also run on the Spectrum PHY with OFDMA off.

The AP uses ns-3's round-robin scheduler (`RrMultiUserScheduler`). By default it serves 4 students per OFDMA transmission.

## 1. Downlink

Guard interval 800 ns. Total throughput in Mbit/s, with the minimum per student in parentheses.

| Students | Wi-Fi 5 | Wi-Fi 6, Yans | Wi-Fi 6, Spectrum | Wi-Fi 6, Spectrum, DL OFDMA |
|---:|---:|---:|---:|---:|
| 10 | 45.1864 (4.2000) | 59.7212 (5.8996) | 59.9004 (5.9304) | 48.5688 (4.8552) |
| 30 | 44.3464 (1.4336) | 57.5652 (1.8732) | 57.7836 (1.8816) | 48.5296 (1.6044) |
| 50 | 42.9520 (0.4928) | 55.1264 (0.5964) | 55.2356 (0.5908) | 46.7348 (0.5236) |

Jain's index was above 0.98 in all downlink runs.

Observations:

1. The PHY model has little effect. Yans and Spectrum differ by less than 0.4%.
2. With DL OFDMA on, total throughput was about 15 to 19% lower than with OFDMA off. It was still higher than Wi-Fi 5.
3. The drop comes from how the channel is divided. A 20 MHz channel used by one student has 234 data subcarriers. When it is split into 4 RUs of 52 tones for 4 students, each RU has 48 data subcarriers, or 192 in total. That is 82% of the full channel, which matches the observed result (48.5688 / 59.9004 = 0.81).

To check this, the number of students per OFDMA transmission was changed (`--ns3::RrMultiUserScheduler::NStations`), with 10 students in total:

| Students per transmission | RUs used | Data subcarriers | Expected (Mbit/s) | Total (Mbit/s) |
|---:|---|---:|---:|---:|
| 1 | 1 x 242-tone | 234 | - | 59.3936 |
| 2 | 2 x 106-tone | 204 | 51.78 | 51.8616 |
| 4 | 4 x 52-tone | 192 | 48.73 | 48.5688 |

The expected value is the 1-student result multiplied by (data subcarriers / 234). The results follow the expected values closely. Setting this value to 9 caused ns-3 to crash, so it was not tested further.

This means that with saturated traffic and large packets, OFDMA does not increase total throughput. Its expected benefits are for many small packets and lower delay, which these runs do not measure.

## 2. Uplink

UL OFDMA requires a guard interval of at least 1600 ns, so all runs in this section use 1600 ns. In UL OFDMA, the AP sends a trigger frame and several students transmit at the same time in their own RUs.

| Students | Configuration | Total (Mbit/s) | Min per student (Mbit/s) | Jain's index |
|---:|---|---:|---:|---:|
| 10 | Wi-Fi 6, Spectrum, OFDMA off | 34.8684 | 0.1512 | 0.803530 |
| 10 | Wi-Fi 6, UL OFDMA, interval 1 ms | 34.2636 | 0.9996 | 0.835296 |
| 10 | Wi-Fi 6, UL OFDMA, interval 3 ms (default) | 30.4808 | 0.9996 | 0.822730 |
| 10 | Wi-Fi 6, UL OFDMA, interval 10 ms | 32.3344 | 1.1228 | 0.874190 |
| 30 | Wi-Fi 6, Spectrum, OFDMA off | 24.6540 | 0 | 0.698623 |
| 30 | Wi-Fi 6, UL OFDMA, interval 1 ms | 24.2620 | 0 | 0.627091 |
| 30 | Wi-Fi 6, UL OFDMA, interval 3 ms (default) | 24.7072 | 0 | 0.669332 |
| 30 | Wi-Fi 6, UL OFDMA, interval 10 ms | 23.8868 | 0 | 0.658462 |

The interval (`--muSchedAccessReqInterval`) is how often the AP takes the channel to send a trigger frame. It is needed because in uplink the AP has no data of its own to send, so without it the AP never starts an OFDMA transmission. Before this setting was added, the UL OFDMA results were identical to the results with OFDMA off.

Observations:

1. With 10 students, UL OFDMA raised the worst student's throughput from 0.1512 to about 1 Mbit/s and slightly improved Jain's index. Total throughput was the same or lower.
2. With 30 students, UL OFDMA made no clear difference. At least one student still received no throughput. Students still compete for the channel normally in addition to being triggered, so collisions still happen.
3. The results change with the interval, but not in a consistent direction. More runs with different seeds are needed before drawing conclusions about uplink.

## Notes for the Paper

- The configuration table should state whether OFDMA was on or off for each Wi-Fi 6 result, along with the PHY model and the scheduler (round-robin, 4 students per transmission).
- Without OFDMA, the Wi-Fi 6 advantage in these results comes from its higher data rate (see `first-runs.md`).

## Commands

Run from `~/ns-3` after copying `classroom-wifi.cc` into `scratch/`:

```sh
./ns3 run "classroom-wifi --nStations=10 --phyModel=Spectrum --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=10 --ofdma=1 --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=10 --downlink=0 --guardInterval=1600 --ulOfdma=1 --simulationTime=2s"
```
