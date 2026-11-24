# Initial Test Runs

Date: November 4, 2026

These runs were done to check the experiment script (`classroom-wifi.cc`) after it was modified from the ns-3 example. Each run is 2 seconds of simulated time, so these are preliminary results and not the final sweep.

## Settings

Unless stated otherwise, all runs use:

| Parameter | Value |
|---|---|
| Frequency band | 5 GHz |
| Channel width | 20 MHz |
| Guard interval | 800 ns |
| MCS | 5 (fixed, constant rate manager) |
| Traffic | UDP, 700-byte packets, saturated |
| Student distance from AP | 1 m |
| Simulation time | 2 s |
| Seed / run | 1 / 1 |
| OFDMA | Off |

## Definitions

| Term | Meaning |
|---|---|
| Downlink | The AP sends data to the students. |
| Uplink | The students send data to the AP. |
| MCS | Modulation and coding scheme. It sets the data rate. A higher MCS is faster but needs a stronger signal. |
| Rate manager | The algorithm that chooses the MCS. `constant` uses the same MCS for every student. `ideal` and `minstrel` choose an MCS for each student based on signal quality. |
| Total throughput | Data successfully received by all students combined, in Mbit/s. |
| Min per student | Throughput of the student with the lowest throughput. |
| Jain's fairness index | (sum x)^2 / (n * sum x^2), where x is each student's throughput. A value of 1 means all students received equal throughput. The minimum is 1/n. |

## 1. Comparison with the Baseline

The modified script was run with the same settings as `baseline.md` to confirm that it produces the same results.

| Configuration | Total (Mbit/s) | Baseline (Mbit/s) | Min per student (Mbit/s) | Jain's index |
|---|---:|---:|---:|---:|
| Wi-Fi 6, 1 student, downlink | 59.9284 | - | 59.9284 | 1.000000 |
| Wi-Fi 6, 10 students, downlink | 59.7212 | 59.7212 | 5.8996 | 0.999944 |
| Wi-Fi 6, 30 students, downlink | 57.5652 | 57.5652 | 1.8732 | 0.999836 |
| Wi-Fi 6, 30 students, uplink | 29.0108 | 27.9216 | 0.0364 | 0.751592 |

The downlink results match the baseline exactly. The uplink result is 1.09 Mbit/s higher. The difference comes from student placement. The original example placed every second student at the same position as the AP. The modified script places all students in a circle 1 m from the AP. When the original placement was restored for testing, the uplink result was 27.9216 Mbit/s, the same as the baseline.

With one student, Jain's index is 1, as expected.

## 2. Wi-Fi 6 (802.11ax) vs Wi-Fi 5 (802.11ac)

| Standard | Students | Direction | Total (Mbit/s) | Mean per student (Mbit/s) | Min per student (Mbit/s) | Jain's index |
|---|---:|---|---:|---:|---:|---:|
| Wi-Fi 6 | 10 | Downlink | 59.7212 | 5.9721 | 5.8996 | 0.999944 |
| Wi-Fi 5 | 10 | Downlink | 45.1864 | 4.5186 | 4.2000 | 0.994377 |
| Wi-Fi 6 | 30 | Downlink | 57.5652 | 1.9188 | 1.8732 | 0.999836 |
| Wi-Fi 5 | 30 | Downlink | 44.3464 | 1.4782 | 1.4336 | 0.999685 |
| Wi-Fi 6 | 30 | Uplink | 29.0108 | 0.9670 | 0.0364 | 0.751592 |
| Wi-Fi 5 | 30 | Uplink | 23.2596 | 0.7753 | 0.0028 | 0.676590 |

Observations:

1. Wi-Fi 6 had about 30% higher total throughput than Wi-Fi 5 in downlink (32% at 10 students, 30% at 30) and 25% higher in uplink. At MCS 5 and 20 MHz, the Wi-Fi 6 data rate is 68.8 Mbit/s and the Wi-Fi 5 data rate is 52 Mbit/s (32% higher), which accounts for most of this difference.
2. Downlink was fair for both standards (Jain's index above 0.99). The AP serves the students in turn, so each student receives about the same share.
3. Uplink was unfair for both standards. With 30 students contending for the channel, collisions occur and some students receive very little throughput. The lowest was 0.0028 Mbit/s for one Wi-Fi 5 student.
4. OFDMA was off in these runs, so the Wi-Fi 6 advantage here comes only from its higher data rate. OFDMA is tested separately in `ofdma.md`.

## 3. One Student Far from the AP

Ten students, Wi-Fi 6, downlink. Nine students stay at 1 m and one student is placed at the distance shown (`--farDistance`). "Near students" is the mean throughput of the nine students at 1 m.

### 3.1 Ideal rate manager

| Far student distance | Total (Mbit/s) | Near students, mean (Mbit/s) | Far student (Mbit/s) |
|---|---:|---:|---:|
| None (all at 1 m) | 118.076 | 11.81 | - |
| 10 m | 116.581 | 11.65 | 11.69 |
| 20 m | 111.166 | 11.12 | 11.11 |
| 30 m | 107.274 | 10.74 | 10.62 |
| 40 m | 101.142 | 10.12 | 10.07 |
| 50 m | 90.882 | 9.10 | 9.01 |
| 60 m | 0.148 | 0.0165 | 0 |
| 80 m | 0.148 | 0.0165 | 0 |

As the far student moves away, it uses a lower MCS and takes longer to transmit each packet. Since the AP gives each student an equal share of packets, the slower student takes up more of the channel time and the throughput of every student goes down. This is the 802.11 performance anomaly. At 50 m, the near students' throughput dropped by about 23% (from 11.81 to 9.10 Mbit/s) because of one far student.

At 60 m and beyond, the far student is out of range and the throughput of the whole network drops to almost zero. A likely cause is that the AP's queue fills with packets that cannot be delivered to that student, which blocks the other students' packets. This has not been verified. Far-student tests should be kept at 50 m or less.

The total with all students at 1 m is higher than in Section 1 (118.076 vs 59.7212 Mbit/s) because the ideal rate manager selects MCS 11 instead of the fixed MCS 5.

### 3.2 Constant rate manager (fixed MCS 5)

| Far student distance | Total (Mbit/s) | Near students, mean (Mbit/s) | Far student (Mbit/s) |
|---|---:|---:|---:|
| 10 m to 30 m | 59.7 | 5.97 | 5.94 |
| 40 m | 36.162 | 3.62 | 3.54 |
| 50 m | 5.555 | 0.62 | 0 |
| 60 m | 0.056 | 0.0062 | 0 |

With a fixed MCS, the far student cannot switch to a lower rate. From about 40 m, the signal is too weak for MCS 5 and the far student's packets start to fail. The AP retransmits these packets, which uses channel time and lowers the throughput of the other students as well. This is a different effect from the performance anomaly. For sub-question 2, the `ideal` or `minstrel` rate manager should be used.

### 3.3 Other checks

| Configuration | Total (Mbit/s) |
|---|---:|
| Wi-Fi 5, ideal, all at 1 m | 66.609 |
| Wi-Fi 5, ideal, far student at 60 m | 0.106 |
| Wi-Fi 6, minstrel, far student at 60 m | 0.129 |

## Commands

Run from `~/ns-3` after copying `classroom-wifi.cc` into `scratch/`:

```sh
./ns3 run "classroom-wifi --standard=ax --nStations=10 --simulationTime=2s"
./ns3 run "classroom-wifi --standard=ac --nStations=30 --downlink=0 --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=10 --rateManager=ideal --farDistance=50 --simulationTime=2s"
```
