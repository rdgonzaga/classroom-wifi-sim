# Final Sweep Results

Date: November 6, 2026

These are the results used in the paper. They replace the preliminary 2-second numbers in `first-runs.md`, `demand.md` and `ofdma.md`. Every setting was run 5 times for 10 seconds of simulated time on the classroom seat layout, after the FqCoDel fix (`TIMELINE.md` item 14). The raw data is in `results/final/`.

## Settings

| Parameter | Value |
|---|---|
| Frequency band | 5 GHz |
| Channel width | 20 MHz |
| Guard interval | 800 ns |
| MCS | 5 (fixed, constant rate manager), except the far-student runs |
| Traffic | UDP, 700-byte packets, constant bit rate per student |
| Layout | Grid (`--layout=grid`): 7 m x 9 m room, 56 desks, AP on the ceiling |
| Students | 5 to 50 in steps of 5 (fixed demand), 5, 10, 20, 30, 40, 50 (saturated) |
| Simulation time | 10 s |
| Seed / runs | 1 / runs 1 to 5 (`--RngRun`), averaged |
| AP queue disc | FqCoDel with `Flows` set to 2^32 - 1 |
| Configurations | Wi-Fi 6 and Wi-Fi 5, downlink and uplink, OFDMA off; Wi-Fi 6 downlink with DL OFDMA |

## Definitions

Same as `demand.md`, plus:

| Term | Meaning |
|---|---|
| Worst (min-max) | The lowest and highest worst-student value among the 5 runs. |
| Mean delay | Each student's average one-way delay of received packets (FlowMonitor), averaged over students. |
| Worst delay | The highest per-student average delay in a run. |
| Loss | Packets sent but not received, as a percentage of packets sent, averaged over students. |
| Min / max | Lowest per-student throughput divided by the highest, in a saturated run. |

## Data Files

| Folder | Sweep | Rows |
|---|---|---:|
| `final/demand3/` | Fixed demand, 3 Mbit/s per student | 250 |
| `final/demand5/` | Fixed demand, 5 Mbit/s per student | 250 |
| `final/saturated/` | Saturated traffic | 150 |
| `final/far/` | One far student, 10 students, downlink, Ideal rate manager | 60 |

Each folder has `runs.csv` (one row per run) and `per-student.csv` (one row per student per run). The columns are the same as in the earlier result files.

## 1. Maximum Number of Students (Sub-question 1)

Largest number of students for which the worst student still received at least 95% of the demand, averaged over 5 runs. The next step (5 more students) was below 95%. The last column is the first number of students at which the worst student's delay passed 150 ms (ITU-T G.114 one-way limit) at 3 Mbit/s.

| Configuration | 3 Mbit/s per student | 5 Mbit/s per student | Worst delay > 150 ms from (3 Mbit/s) |
|---|---:|---:|---:|
| Wi-Fi 6, downlink | 20 | 10 | 25 |
| Wi-Fi 6, downlink, DL OFDMA | 15 | 5 | 25 |
| Wi-Fi 5, downlink | 15 | 5 | 20 |
| Wi-Fi 6, uplink | 10 | 5 | 15 |
| Wi-Fi 5, uplink | 5 | 5 | 15 |

Observations:

1. The delay limit is always reached at or after the throughput limit, so adding "worst delay below 150 ms" to the definition of usable does not change this table. At 5 Mbit/s the same holds.
2. Compared with the 2-second runs in `demand.md`, Wi-Fi 6 uplink at 3 Mbit/s moved from 5 to 10 (worst student 0.987, lowest run 0.982), and Wi-Fi 6 DL OFDMA at 5 Mbit/s moved from 10 to 5 (0.933, lowest run 0.915). Both were close to 0.95 before. The other limits did not change.
3. Wi-Fi 6 serves 5 more students than Wi-Fi 5 in every case except uplink at 5 Mbit/s, where both serve only 5.
4. The downlink limits are close to the saturated total throughput (Section 4) divided by the demand: 59.9 / 3 = 20 for Wi-Fi 6 and 45.3 / 3 = 15 for Wi-Fi 5.

## 2. 3 Mbit/s per Student

Averages of 5 runs. Mean and worst are delivered / demand.

### 2.1 Wi-Fi 6, downlink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 1.000-1.000 | 0.7 | 1.3 | 0.00 | 1.0000 |
| 10 | 30.00 | 1.000 | 1.000 | 1.000-1.000 | 2.3 | 2.5 | 0.00 | 1.0000 |
| 15 | 44.98 | 0.999 | 0.999 | 0.999-0.999 | 6.0 | 6.2 | 0.00 | 1.0000 |
| 20 | 59.07 | 0.984 | 0.981 | 0.980-0.981 | 52.5 | 54.3 | 1.02 | 1.0000 |
| 25 | 58.61 | 0.781 | 0.778 | 0.778-0.779 | 158.1 | 161.3 | 20.95 | 1.0000 |
| 30 | 57.95 | 0.644 | 0.641 | 0.641-0.642 | 413.7 | 426.2 | 33.37 | 1.0000 |
| 35 | 57.39 | 0.547 | 0.544 | 0.544-0.544 | 429.4 | 458.7 | 42.90 | 1.0000 |
| 40 | 56.79 | 0.473 | 0.471 | 0.471-0.471 | 407.4 | 435.4 | 50.62 | 1.0000 |
| 45 | 56.06 | 0.415 | 0.413 | 0.413-0.413 | 381.5 | 424.8 | 57.13 | 1.0000 |
| 50 | 55.50 | 0.370 | 0.368 | 0.368-0.368 | 360.6 | 402.6 | 62.29 | 1.0000 |

### 2.2 Wi-Fi 6, downlink, DL OFDMA

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 1.000-1.000 | 0.6 | 0.9 | 0.00 | 1.0000 |
| 10 | 30.00 | 1.000 | 1.000 | 1.000-1.000 | 1.4 | 2.0 | 0.00 | 1.0000 |
| 15 | 44.95 | 0.999 | 0.998 | 0.998-0.998 | 12.1 | 12.1 | 0.00 | 1.0000 |
| 20 | 48.61 | 0.810 | 0.809 | 0.809-0.810 | 131.7 | 135.3 | 18.19 | 1.0000 |
| 25 | 48.60 | 0.648 | 0.647 | 0.647-0.647 | 422.6 | 428.0 | 33.21 | 1.0000 |
| 30 | 48.61 | 0.540 | 0.539 | 0.539-0.539 | 488.3 | 503.8 | 44.03 | 1.0000 |
| 35 | 48.61 | 0.463 | 0.462 | 0.462-0.462 | 461.1 | 486.8 | 52.30 | 1.0000 |
| 40 | 48.61 | 0.405 | 0.404 | 0.404-0.404 | 430.8 | 485.1 | 58.74 | 1.0000 |
| 45 | 48.61 | 0.360 | 0.359 | 0.359-0.359 | 406.0 | 470.3 | 63.69 | 1.0000 |
| 50 | 48.61 | 0.324 | 0.323 | 0.323-0.323 | 380.9 | 409.7 | 67.34 | 1.0000 |

### 2.3 Wi-Fi 5, downlink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 1.000-1.000 | 0.8 | 1.4 | 0.00 | 1.0000 |
| 10 | 29.99 | 1.000 | 1.000 | 1.000-1.000 | 3.0 | 3.3 | 0.00 | 1.0000 |
| 15 | 44.84 | 0.996 | 0.994 | 0.993-0.994 | 35.0 | 35.1 | 0.00 | 1.0000 |
| 20 | 45.30 | 0.755 | 0.751 | 0.751-0.751 | 204.3 | 207.7 | 23.28 | 1.0000 |
| 25 | 45.04 | 0.601 | 0.597 | 0.597-0.597 | 515.0 | 546.1 | 37.54 | 1.0000 |
| 30 | 44.70 | 0.497 | 0.494 | 0.494-0.494 | 511.1 | 563.0 | 48.53 | 1.0000 |
| 35 | 44.34 | 0.422 | 0.420 | 0.420-0.420 | 484.6 | 517.1 | 57.04 | 1.0000 |
| 40 | 43.99 | 0.367 | 0.365 | 0.364-0.365 | 456.4 | 517.7 | 62.88 | 1.0000 |
| 45 | 43.66 | 0.323 | 0.322 | 0.321-0.322 | 427.3 | 483.0 | 67.23 | 1.0000 |
| 50 | 43.31 | 0.289 | 0.287 | 0.287-0.287 | 399.9 | 442.7 | 70.65 | 1.0000 |

### 2.4 Wi-Fi 6, uplink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 1.000-1.000 | 1.1 | 1.1 | 0.00 | 1.0000 |
| 10 | 29.92 | 0.997 | 0.987 | 0.982-0.993 | 25.2 | 33.8 | 0.00 | 1.0000 |
| 15 | 35.52 | 0.789 | 0.568 | 0.554-0.590 | 146.6 | 199.1 | 19.08 | 0.9825 |
| 20 | 35.14 | 0.586 | 0.275 | 0.168-0.355 | 208.8 | 299.7 | 39.18 | 0.9340 |
| 25 | 34.01 | 0.453 | 0.179 | 0.153-0.228 | 237.5 | 349.7 | 52.65 | 0.8978 |
| 30 | 32.06 | 0.356 | 0.108 | 0.061-0.156 | 266.6 | 361.9 | 62.61 | 0.8663 |
| 35 | 31.84 | 0.303 | 0.081 | 0.056-0.115 | 281.9 | 404.2 | 68.10 | 0.8351 |
| 40 | 30.94 | 0.258 | 0.046 | 0.026-0.067 | 298.5 | 411.2 | 72.94 | 0.8018 |
| 45 | 29.91 | 0.222 | 0.034 | 0.022-0.046 | 311.7 | 432.6 | 76.72 | 0.7728 |
| 50 | 29.77 | 0.198 | 0.019 | 0.011-0.034 | 320.9 | 442.1 | 79.28 | 0.7122 |

### 2.5 Wi-Fi 5, uplink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 1.000-1.000 | 1.2 | 1.2 | 0.00 | 1.0000 |
| 10 | 28.54 | 0.951 | 0.883 | 0.871-0.916 | 88.9 | 121.3 | 3.74 | 0.9982 |
| 15 | 28.55 | 0.634 | 0.404 | 0.285-0.498 | 209.6 | 283.3 | 34.38 | 0.9556 |
| 20 | 27.38 | 0.456 | 0.190 | 0.161-0.225 | 256.9 | 345.1 | 52.39 | 0.8736 |
| 25 | 26.51 | 0.353 | 0.107 | 0.059-0.127 | 291.3 | 393.1 | 63.04 | 0.8456 |
| 30 | 25.39 | 0.282 | 0.075 | 0.008-0.109 | 318.2 | 420.4 | 70.58 | 0.8256 |
| 35 | 24.48 | 0.233 | 0.050 | 0.017-0.084 | 339.6 | 449.3 | 75.55 | 0.7825 |
| 40 | 24.00 | 0.200 | 0.027 | 0.017-0.050 | 340.7 | 448.2 | 79.01 | 0.7646 |
| 45 | 23.25 | 0.172 | 0.021 | 0.000-0.036 | 357.2 | 461.9 | 81.96 | 0.6931 |
| 50 | 22.65 | 0.151 | 0.020 | 0.008-0.036 | 369.0 | 525.3 | 84.19 | 0.7223 |

Observations:

1. **Downlink degrades evenly.** Until the total demand reaches the channel's capacity, every student receives the full demand. After that, the worst student stays within 0.003 of the mean and Jain's index stays at 1.0000. The 5 runs give almost the same values.
2. **Uplink degrades unevenly.** The total stops rising at about 35 Mbit/s for Wi-Fi 6 and 28.5 Mbit/s for Wi-Fi 5, then falls as students are added. The worst student falls far below the mean (at 20 students on Wi-Fi 6: 0.275 against 0.586), Jain's index drops to about 0.7 at 50 students, and the runs differ much more than in downlink. Each student contends for the channel on its own, so collisions and backoff give some students far fewer transmissions than others.
3. **Delay stops growing at about 400 to 550 ms.** Once the AP's queue is full, ns-3 drops packets that have waited longer than the Wi-Fi MAC queue's maximum delay (500 ms by default), so the extra load shows up as loss instead of delay. Loss is 0 until the capacity limit and then rises quickly.
4. **DL OFDMA** gives lower delay than OFDMA off at 5 and 10 students (0.6 and 1.4 ms against 0.7 and 2.3 ms), but caps total throughput at 48.6 Mbit/s, so it fills up at 20 students instead of 25 and its delay passes 100 ms earlier.

## 3. 5 Mbit/s per Student

### 3.1 Wi-Fi 6, downlink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 25.00 | 1.000 | 1.000 | 1.000-1.000 | 1.0 | 1.1 | 0.00 | 1.0000 |
| 10 | 49.97 | 0.999 | 0.999 | 0.999-0.999 | 5.7 | 5.7 | 0.00 | 1.0000 |
| 15 | 59.85 | 0.798 | 0.787 | 0.780-0.795 | 259.3 | 269.9 | 18.49 | 0.9981 |
| 20 | 59.25 | 0.593 | 0.590 | 0.590-0.590 | 542.5 | 563.7 | 38.80 | 1.0000 |
| 30 | 57.98 | 0.387 | 0.385 | 0.385-0.385 | 412.1 | 439.6 | 59.82 | 1.0000 |
| 50 | 55.51 | 0.222 | 0.221 | 0.221-0.221 | 270.0 | 303.0 | 77.14 | 1.0000 |

### 3.2 Wi-Fi 6, downlink, DL OFDMA

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 25.00 | 1.000 | 1.000 | 1.000-1.000 | 0.6 | 0.9 | 0.00 | 1.0000 |
| 10 | 47.52 | 0.950 | 0.933 | 0.915-0.949 | 73.2 | 149.0 | 4.26 | 0.9997 |
| 15 | 48.61 | 0.648 | 0.648 | 0.647-0.648 | 656.2 | 681.5 | 33.21 | 1.0000 |
| 20 | 48.62 | 0.486 | 0.486 | 0.486-0.486 | 587.5 | 616.1 | 49.30 | 1.0000 |
| 30 | 48.62 | 0.324 | 0.324 | 0.323-0.324 | 421.8 | 448.6 | 66.45 | 1.0000 |
| 50 | 48.60 | 0.194 | 0.194 | 0.194-0.194 | 275.0 | 311.7 | 80.07 | 1.0000 |

### 3.3 Wi-Fi 5, downlink

| Students | Total (Mbit/s) | Mean | Worst | Worst (min-max) | Mean delay (ms) | Worst delay (ms) | Loss (%) | Jain |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 5 | 25.00 | 1.000 | 1.000 | 1.000-1.000 | 1.3 | 1.3 | 0.00 | 1.0000 |
| 10 | 45.30 | 0.906 | 0.843 | 0.839-0.845 | 126.4 | 176.6 | 8.73 | 0.9938 |
| 15 | 45.32 | 0.604 | 0.602 | 0.602-0.602 | 698.6 | 725.7 | 36.84 | 1.0000 |
| 20 | 45.32 | 0.453 | 0.451 | 0.451-0.451 | 593.9 | 620.8 | 52.24 | 1.0000 |
| 30 | 44.71 | 0.298 | 0.296 | 0.296-0.296 | 426.9 | 452.9 | 68.88 | 1.0000 |
| 50 | 43.32 | 0.173 | 0.172 | 0.172-0.172 | 282.6 | 315.2 | 82.08 | 1.0000 |

### 3.4 Uplink

| Students | Wi-Fi 6 total (Mbit/s) | Wi-Fi 6 worst | Wi-Fi 6 worst delay (ms) | Wi-Fi 5 total (Mbit/s) | Wi-Fi 5 worst | Wi-Fi 5 worst delay (ms) |
|---:|---:|---:|---:|---:|---:|---:|
| 5 | 24.99 | 0.999 | 2.9 | 24.99 | 0.999 | 5.7 |
| 10 | 40.31 | 0.679 | 226.8 | 31.81 | 0.443 | 337.6 |
| 15 | 38.44 | 0.261 | 602.2 | 29.20 | 0.209 | 414.9 |
| 20 | 36.59 | 0.143 | 710.9 | 28.08 | 0.113 | 446.2 |
| 30 | 33.54 | 0.047 | 1164.6 | 25.65 | 0.045 | 467.5 |
| 50 | 30.02 | 0.019 | 1648.3 | 23.13 | 0.008 | 494.3 |

All 10 student counts (5 to 50) are in `final/demand5/runs.csv`; the rows above are a subset.

Observations:

1. At 5 Mbit/s, Wi-Fi 6 downlink still serves 10 students (49.97 Mbit/s total, below its 59.9 Mbit/s capacity). 10 students on Wi-Fi 5 ask for 50 Mbit/s, more than its 45.3 Mbit/s capacity, so it serves only 5.
2. Wi-Fi 6 uplink's worst delay grows past 1 second at 30 students or more, unlike every other configuration. In uplink each student has its own queue, so a student that keeps losing contention holds its packets longer. The cause of the values above the 500 ms queue limit was not investigated.

## 4. Saturated Traffic (Capacity and Fairness)

Every student sends or receives as fast as possible. Averages of 5 runs.

### 4.1 Total Throughput (Mbit/s)

| Students | Wi-Fi 6 DL | Wi-Fi 6 DL OFDMA | Wi-Fi 5 DL | Wi-Fi 6 UL | Wi-Fi 5 UL |
|---:|---:|---:|---:|---:|---:|
| 5 | 59.91 | 48.61 | 45.32 | 48.73 | 36.91 |
| 10 | 59.88 | 48.61 | 45.30 | 42.72 | 31.70 |
| 20 | 59.21 | 48.61 | 45.27 | 36.06 | 27.12 |
| 30 | 57.90 | 48.60 | 44.64 | 31.57 | 23.84 |
| 40 | 56.75 | 48.60 | 43.94 | 28.65 | 21.52 |
| 50 | 55.41 | 48.59 | 43.24 | 26.89 | 20.27 |

### 4.2 Jain's Fairness Index

| Students | Wi-Fi 6 DL | Wi-Fi 6 DL OFDMA | Wi-Fi 5 DL | Wi-Fi 6 UL | Wi-Fi 5 UL |
|---:|---:|---:|---:|---:|---:|
| 5 | 1.0000 | 1.0000 | 1.0000 | 0.9886 | 0.9864 |
| 10 | 1.0000 | 1.0000 | 0.9899 | 0.9519 | 0.9570 |
| 20 | 1.0000 | 1.0000 | 1.0000 | 0.9248 | 0.9098 |
| 30 | 1.0000 | 1.0000 | 1.0000 | 0.8821 | 0.9039 |
| 40 | 1.0000 | 1.0000 | 1.0000 | 0.8987 | 0.8897 |
| 50 | 1.0000 | 1.0000 | 1.0000 | 0.8751 | 0.8795 |

### 4.3 Min / Max Per-Student Throughput

| Students | Wi-Fi 6 DL | Wi-Fi 6 DL OFDMA | Wi-Fi 5 DL | Wi-Fi 6 UL | Wi-Fi 5 UL |
|---:|---:|---:|---:|---:|---:|
| 5 | 0.9983 | 0.9993 | 0.9982 | 0.7697 | 0.7316 |
| 10 | 0.9955 | 0.9993 | 0.8106 | 0.4183 | 0.4792 |
| 20 | 0.9917 | 0.9982 | 0.9900 | 0.2633 | 0.2876 |
| 30 | 0.9915 | 0.9966 | 0.9881 | 0.1953 | 0.1828 |
| 40 | 0.9909 | 0.9959 | 0.9881 | 0.1947 | 0.1749 |
| 50 | 0.9904 | 0.9941 | 0.9879 | 0.1285 | 0.1446 |

Observations:

1. From 5 to 50 students, total downlink throughput falls by 7.5% on Wi-Fi 6 (59.91 to 55.41) and 4.6% on Wi-Fi 5 (45.32 to 43.24). Uplink falls by 45% on both (48.73 to 26.89 and 36.91 to 20.27), because more students means more collisions.
2. Wi-Fi 6 carries 32% more than Wi-Fi 5 in downlink at 5 students (59.91 / 45.32 = 1.32) and 28% to 33% more in uplink. OFDMA is off, so this comes from the PHY rate: HE-MCS 5 at 20 MHz and 800 ns GI is 68.8 Mbit/s, VHT-MCS 5 is 52 Mbit/s, a ratio of 1.32.
3. DL OFDMA stays at 48.6 Mbit/s for every number of students, 81% of Wi-Fi 6 without OFDMA. This is the RU subcarrier loss explained in `ofdma.md` (192 of 234 data subcarriers, 82%).
4. Downlink fairness stays at 1.0000 except Wi-Fi 5 at 10 students (Section 6). Uplink fairness falls to about 0.88 at 50 students, and the slowest student gets 13 to 14% of the fastest student's throughput.

## 5. One Far Student (Sub-question 2)

10 students, downlink, saturated, Ideal rate manager (each student's MCS is chosen from its signal quality). One student is moved to 10 to 50 m from the AP; 0 means no far student, all 10 seated in the room. Averages of 5 runs.

| Far distance (m) | Wi-Fi 6 near students (Mbit/s each) | Wi-Fi 6 far student | Wi-Fi 6 change | Wi-Fi 5 near students (Mbit/s each) | Wi-Fi 5 far student | Wi-Fi 5 change |
|---:|---:|---:|---:|---:|---:|---:|
| 0 | 12.00 | - | - | 6.76 | - | - |
| 10 | 11.73 | 11.72 | -2.3% | 6.76 | 6.78 | 0.0% |
| 20 | 11.31 | 11.29 | -5.8% | 6.63 | 6.64 | -1.9% |
| 30 | 10.90 | 10.90 | -9.2% | 6.45 | 6.43 | -4.7% |
| 40 | 10.28 | 10.26 | -14.3% | 6.16 | 6.13 | -9.0% |
| 50 | 9.22 | 9.21 | -23.2% | 5.64 | 5.63 | -16.6% |

"Change" is the near students' throughput compared with no far student.

Observations:

1. The far student receives the same throughput as the near students, and every near student loses throughput as it moves away: 23.2% on Wi-Fi 6 and 16.6% on Wi-Fi 5 at 50 m. This is the 802.11 performance anomaly. The AP gives each student the same number of transmissions, so the far student's slower (longer) transmissions take airtime from everyone.
2. With the Ideal rate manager, the near students use a higher MCS than 5, so the totals (120.0 Mbit/s for Wi-Fi 6 and 67.7 Mbit/s for Wi-Fi 5 with no far student) are higher than in Sections 2 to 4.
3. Wi-Fi 6 loses more in percentage terms because its near students' rate is higher, so the gap between near and far rates is larger.

## 6. Uneven Split at the Edge of Saturation (Open)

Saturated Wi-Fi 5 downlink with 10 students has Jain 0.9899 and min / max 0.81, while every other downlink count is at 1.0000. In each of the 5 runs, 3 or 4 consecutive students (for example students 3, 4 and 5 in run 1, students 7, 8 and 9 in run 3) receive about 5.16 Mbit/s with about 65 ms delay, and the rest receive about 4.10 Mbit/s with about 286 ms delay. The same kind of split appears in Wi-Fi 5 downlink at 5 Mbit/s and 10 students (worst 0.843, mean 0.906) and slightly in Wi-Fi 6 downlink at 5 Mbit/s and 15 students (Jain 0.9981). These are all cases where the total demand is just above capacity.

This is not the FqCoDel collision fixed in `TIMELINE.md` item 14 (that halved a student's share and picked students at random, not in consecutive groups). The cause was not investigated. It does not change any limit in Section 1, since all of these cases are already above capacity.

## Notes for the Paper

- Use Section 1 for Table II and Section 5 for sub-question 2. Section 4 is for capacity and fairness, not for "how many students".
- When comparing Wi-Fi 6 with Wi-Fi 5, state that OFDMA is off and that most of the gap comes from the PHY rate at the same MCS.
- Report the 5-run spread for uplink (Section 2.4 and 2.5), since its runs differ much more than downlink.

## Commands

From the repo in WSL, after the FqCoDel fix:

```sh
LAYOUT=grid DEMAND=3 STUDENTS="5 10 15 20 25 30 35 40 45 50" OUT_DIR=$PWD/results/final/demand3 ./sweep.sh
LAYOUT=grid DEMAND=5 STUDENTS="5 10 15 20 25 30 35 40 45 50" OUT_DIR=$PWD/results/final/demand5 ./sweep.sh
LAYOUT=grid OUT_DIR=$PWD/results/final/saturated ./sweep.sh
```

The far-student runs, from `~/ns-3`:

```sh
OUT=~/dlsu/nscom03/classroom-wifi-sim/results/final/far
for run in 1 2 3 4 5; do
  for std in ax ac; do
    for far in 0 10 20 30 40 50; do
      ./ns3 run --no-build "classroom-wifi --standard=$std --nStations=10 --layout=grid --rateManager=ideal --farDistance=$far --simulationTime=10s --RngSeed=1 --RngRun=$run --csv=$OUT/runs.csv --perStudentCsv=$OUT/per-student.csv" | grep -v "^  Student"
    done
  done
done
```

With 3 sweeps in parallel, the whole set took about 80 minutes (the far-student loop about 10 minutes).
