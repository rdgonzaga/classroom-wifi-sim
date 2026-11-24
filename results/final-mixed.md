# Device Mix and 2x2 Control Results

Date: November 24, 2026

Results for configurations B (device mix) and C (2x2 control). Configuration A (one 1x1 device per student, 1x1 AP) is in `final.md`, and its numbers are repeated here for comparison. Settings are the same as `final.md` (5 GHz, 20 MHz, 800 ns GI, MCS 5, UDP 700 bytes, grid layout, 10 s, seed 1, runs 1 to 5). The raw data is in `results/final-mixed/`.

| Configuration | AP | Devices per student | Demand per student |
|---|---|---|---|
| A. Baseline | 1x1 | One 1x1 device | 3 Mbit/s |
| B. Device mix | 2x2 | A 1x1 phone, plus a 2x2 laptop with probability p (p = 1 and p = 0.5) | Phone 1 Mbit/s, laptop 3 Mbit/s |
| C. Control | 2x2 | One 2x2 device | 3 Mbit/s |

B skips DL OFDMA because ns-3.48 asserts when one DL MU PPDU carries both 1-stream and 2-stream users (see `TIMELINE.md`).

## Data Files

| Folder | Sweep | Rows |
|---|---|---:|
| `mixed-p1/` | B, p = 1, fixed demand, 5 to 50 students | 200 |
| `mixed-p05/` | B, p = 0.5, fixed demand, 5 to 50 students | 200 |
| `mixed-saturated/` | B, p = 1, saturated | 120 |
| `control-demand3/` | C, 3 Mbit/s, 5 to 50 students (incl. DL OFDMA) | 250 |
| `control-saturated/` | C, saturated (incl. DL OFDMA) | 150 |
| `mixed-far/` | B, p = 1, one far student, 10 students, DL, Ideal | 60 |
| `control-far/` | C, one far student, 10 students, DL, Ideal | 60 |

In `per-student.csv`, one row is one STA: `studentId` is whose device it is and `deviceType` is `laptop`, `phone` or `sta`.

## 1. Maximum Number of Students (Sub-question 1)

Largest number of students for which every device still received at least 95% of its demand (`minSatisfaction`, averaged over 5 runs). The next step was below 95%. In brackets: the first number of students at which the worst device's delay passed 150 ms.

| Configuration | A | C | B, p = 1 | B, p = 0.5 |
|---|---:|---:|---:|---:|
| Wi-Fi 6, downlink | 20 (25) | 35 (45) | 20 (25) | 30 (35) |
| Wi-Fi 6, downlink, DL OFDMA | 15 (25) | 30 (45) | - | - |
| Wi-Fi 5, downlink | 15 (20) | 25 (40) | 15 (20) | 20 (30) |
| Wi-Fi 6, uplink | 10 (15) | 15 (20) | 10 (15) | 10 (20) |
| Wi-Fi 5, uplink | 5 (15) | 15 (20) | 5 (10) | 10 (15) |

Worst laptop and worst phone (delivered / demand) at the first count that failed:

| Configuration | B, p = 1 | Laptop | Phone | B, p = 0.5 | Laptop | Phone |
|---|---:|---:|---:|---:|---:|---:|
| Wi-Fi 6, downlink | 25 | 0.727 | 0.994 | 35 | 0.654 | 0.993 |
| Wi-Fi 5, downlink | 20 | 0.748 | 0.993 | 25 | 0.898 | 0.992 |
| Wi-Fi 6, uplink | 15 | 0.456 | 0.619 | 15 | 0.879 | 0.896 |
| Wi-Fi 5, uplink | 10 | 0.850 | 0.880 | 15 | 0.675 | 0.710 |

Observations:

1. C (two streams on the AP and every device) serves about 1.5x to 3x as many students as A: 35 vs 20 on Wi-Fi 6 downlink, 25 vs 15 on Wi-Fi 5 downlink, and 15 vs 5 on Wi-Fi 5 uplink.
2. B at p = 1 has exactly the same limits as A, even with a 2x2 AP. Each student asks for 4 Mbit/s instead of 3 (1.33x), and B's saturated capacity is only about 1.32x A's (Section 2), so the two cancel out.
3. In downlink, laptops fail first and phones stay near 1.0. The AP gives every STA about the same throughput (Section 2), so the 3 Mbit/s laptops run short before the 1 Mbit/s phones. In uplink both types fall together.
4. At p = 0.5 there are about half as many laptops (e.g. 15.8 on average at 30 students), so the limits go up by 5 to 10 students.

## 2. Saturated Traffic (Capacity and Fairness)

Averages of 5 runs. B is p = 1.

### 2.1 Total Throughput (Mbit/s)

| Students | Wi-Fi 6 DL (A / C / B) | Wi-Fi 5 DL (A / C / B) | Wi-Fi 6 UL (A / C / B) | Wi-Fi 5 UL (A / C / B) | Wi-Fi 6 DL OFDMA (A / C) |
|---:|---:|---:|---:|---:|---:|
| 5 | 59.91 / 115.45 / 78.90 | 45.32 / 89.27 / 60.14 | 48.73 / 97.27 / 57.75 | 36.91 / 75.24 / 47.30 | 48.61 / 97.42 |
| 10 | 59.88 / 115.42 / 77.67 | 45.30 / 89.25 / 59.75 | 42.72 / 87.76 / 51.10 | 31.70 / 67.55 / 40.85 | 48.61 / 97.42 |
| 20 | 59.21 / 112.66 / 73.40 | 45.27 / 87.74 / 57.37 | 36.06 / 76.55 / 42.87 | 27.12 / 57.72 / 33.70 | 48.61 / 97.43 |
| 30 | 57.90 / 108.07 / 69.50 | 44.64 / 85.02 / 55.08 | 31.57 / 66.77 / 37.81 | 23.84 / 51.40 / 29.39 | 48.60 / 97.33 |
| 40 | 56.75 / 103.74 / 65.95 | 43.94 / 82.45 / 52.92 | 28.65 / 62.10 / 33.98 | 21.52 / 47.41 / 25.77 | 48.60 / 96.01 |
| 50 | 55.41 / 99.82 / 62.72 | 43.24 / 80.04 / 50.98 | 26.89 / 57.41 / 31.52 | 20.27 / 44.20 / 23.72 | 48.59 / 90.17 |

For B, students = number of students (the STA count is twice that).

### 2.2 Device Mix by Type (B, p = 1)

Mean throughput per laptop and per phone (Mbit/s), and Jain's index on all STAs.

| Students | Wi-Fi 6 DL laptop / phone | Jain | Wi-Fi 6 UL laptop / phone | Jain | Wi-Fi 5 UL laptop / phone | Jain |
|---:|---:|---:|---:|---:|---:|---:|
| 5 | 7.89 / 7.89 | 1.0000 | 5.48 / 6.07 | 0.9096 | 5.38 / 4.08 | 0.9023 |
| 10 | 3.88 / 3.88 | 1.0000 | 2.31 / 2.80 | 0.8428 | 2.03 / 2.05 | 0.8108 |
| 20 | 1.84 / 1.84 | 1.0000 | 0.91 / 1.23 | 0.7070 | 0.78 / 0.90 | 0.7255 |
| 50 | 0.63 / 0.63 | 1.0000 | 0.25 / 0.38 | 0.6420 | 0.21 / 0.27 | 0.6239 |

Wi-Fi 5 DL behaves like Wi-Fi 6 DL (laptop = phone, Jain 1.0000).

Observations:

1. C carries about twice A's total in every case (115.45 vs 59.91 on Wi-Fi 6 DL at 5 students), since every link has two streams.
2. B carries about 1.32x A's total in downlink (78.90 / 59.91). Laptops and phones get the same throughput, because the AP shares transmissions per packet, not per airtime. A phone's packets take twice as long as a laptop's, so the effective rate is the harmonic mean of 137.6 and 68.8 Mbit/s, which is 91.7 Mbit/s, or 1.33x the 1x1 rate of 68.8. The 2x2 laptops raise total capacity but don't get more throughput than phones.
3. B's uplink is the least fair setting in the study: Jain falls to about 0.63 at 50 students (A: 0.88, C: 0.82), and the slowest STA gets close to nothing (min / max 0.0000 to 0.0001 at 50 students). B has twice as many STAs contending for the channel.

## 3. One Far Student (Sub-question 2)

10 students, downlink, saturated, Ideal rate manager. "Near" is the mean throughput per near student (for B, laptop + phone). "Change" is compared with no far student.

| Far distance (m) | Wi-Fi 6 A | Wi-Fi 6 C | Wi-Fi 6 B | Wi-Fi 5 A | Wi-Fi 5 C | Wi-Fi 5 B |
|---:|---:|---:|---:|---:|---:|---:|
| 0 (near, Mbit/s) | 12.00 | 22.26 | 15.02 | 6.76 | 13.01 | 8.71 |
| 10 | -2.3% | -2.1% | -2.2% | 0.0% | 0.0% | 0.0% |
| 20 | -5.8% | -5.4% | -5.5% | -1.9% | -1.8% | -1.9% |
| 30 | -9.2% | -8.6% | -8.7% | -4.7% | -4.4% | -4.5% |
| 40 | -14.3% | -13.3% | -13.5% | -9.0% | -8.5% | -8.7% |
| 50 | -23.2% | -21.5% | -22.0% | -16.6% | -15.8% | -16.1% |

Observations:

1. The anomaly is about the same size in all three configurations: about 22% on Wi-Fi 6 and 16% on Wi-Fi 5 at 50 m. Adding streams or devices doesn't protect the near students.
2. As in A, the far student gets the same throughput as the near students (e.g. B Wi-Fi 6 at 50 m: 11.72 vs 11.71 Mbit/s per student), and in B each near laptop and phone get the same share (5.86 Mbit/s each at 50 m).

## Notes for the Paper

- Section 1 answers sub-question 1 for B and C; Section 3 answers sub-question 2.
- B differs from A in two ways (2x2 AP and more devices). Compare A with C for the stream upgrade, and C with B for the device mix.
- The demands of 1 and 3 Mbit/s are our assumptions, and B's limits depend on them.

## Commands

From the repo in WSL:

```sh
JOBS=14 ./sweep-mixed.sh
```

The whole sweep (1,040 runs, 14 at a time) ran from 12:35 to 16:00, including a restart after a shutdown. `sweep-mixed.sh` skips runs that already finished, so it can be restarted.
