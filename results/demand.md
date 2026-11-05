# Fixed-Demand Test Runs

Date: November 4, 2026

In the earlier runs every student's traffic was saturated, so per-student throughput was always the total divided by the number of students. That cannot show when a classroom stops being usable. In these runs each student asks for a fixed data rate (`--demand`), and the result is how much of that rate each student actually receives. Each run is 2 seconds of simulated time, so these are preliminary results.

## Settings

| Parameter | Value |
|---|---|
| Frequency band | 5 GHz |
| Channel width | 20 MHz |
| Guard interval | 800 ns |
| MCS | 5 (fixed, constant rate manager) |
| Traffic | UDP, 700-byte packets, constant bit rate per student |
| Demand per student | 3 Mbit/s and 5 Mbit/s |
| Students | 5 to 50, in steps of 5 |
| Student distance from AP | 1 m |
| Simulation time | 2 s |
| Seed / runs | 1 / runs 1, 2 and 3 (`--RngRun`), averaged |
| Configurations | Wi-Fi 6 and Wi-Fi 5, downlink and uplink, OFDMA off; Wi-Fi 6 downlink with DL OFDMA |

The demand values stand for typical classroom use. 3 Mbit/s is roughly one HD video stream or a video call. 5 Mbit/s is a heavier stream or several applications at once.

## Definitions

| Term | Meaning |
|---|---|
| Demand | The data rate each student's application tries to send or receive, in Mbit/s of application payload. |
| Delivered / demand | A student's received throughput divided by the demand. 1.00 means the student received everything it asked for. |
| Worst student | The lowest delivered / demand among all students in a run, averaged over the 3 runs. |
| Usable | Every student receives at least 95% of the demand (worst student at least 0.95). |
| Jain's fairness index | (sum x)^2 / (n * sum x^2), where x is each student's throughput. 1 means all students received the same throughput. |

## 1. Maximum Number of Students (Sub-question 1)

Largest number of students tested for which the worst student still received at least 95% of the demand. The next step (5 more students) was below 95%.

| Configuration | 3 Mbit/s per student | 5 Mbit/s per student |
|---|---:|---:|
| Wi-Fi 6, downlink | 20 | 10 |
| Wi-Fi 6, downlink, DL OFDMA | 15 | 10 |
| Wi-Fi 5, downlink | 15 | 5 |
| Wi-Fi 6, uplink | 5 | 5 |
| Wi-Fi 5, uplink | 5 | 5 |

The step size is 5 students, so the real limit lies between the value shown and 5 more. At 3 Mbit/s, Wi-Fi 6 downlink at 20 students was just above the limit (0.951).

Observations:

1. In downlink, the limit is close to the saturated total throughput divided by the demand. Wi-Fi 6 carries about 58 to 60 Mbit/s in total, so it can serve about 20 students at 3 Mbit/s or about 12 at 5 Mbit/s. Wi-Fi 5 carries about 45 Mbit/s, so it can serve about 15 students at 3 Mbit/s or 9 at 5 Mbit/s.
2. Wi-Fi 6 served 5 more students than Wi-Fi 5 at both demand levels in downlink.
3. DL OFDMA lowered the limit at 3 Mbit/s (15 instead of 20). This matches `ofdma.md`: with OFDMA on, the total throughput is about 48.6 Mbit/s instead of 58 to 60.
4. Uplink failed much earlier than downlink. Wi-Fi 6 uplink at 10 students and 3 Mbit/s asked for 30 Mbit/s in total and received 29.56, but the worst student received only 0.946 of its demand on average (0.903 in the worst run). Because each student contends for the channel on its own, collisions make the shares uneven well before the channel is full.

## 2. Wi-Fi 6 vs Wi-Fi 5 at 3 Mbit/s per Student

Averages of 3 runs. "Mean" and "worst" are delivered / demand.

### 2.1 Downlink

| Students | Wi-Fi 6 total (Mbit/s) | Wi-Fi 6 mean | Wi-Fi 6 worst | Wi-Fi 5 total (Mbit/s) | Wi-Fi 5 mean | Wi-Fi 5 worst |
|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 15.00 | 1.000 | 1.000 |
| 10 | 29.98 | 0.999 | 0.999 | 29.97 | 0.999 | 0.998 |
| 15 | 44.88 | 0.997 | 0.995 | 44.22 | 0.983 | 0.969 |
| 20 | 58.31 | 0.972 | 0.951 | 45.12 | 0.752 | 0.733 |
| 25 | 58.33 | 0.778 | 0.760 | 44.91 | 0.599 | 0.582 |
| 30 | 57.75 | 0.642 | 0.628 | 44.58 | 0.495 | 0.481 |
| 35 | 57.21 | 0.545 | 0.534 | 44.23 | 0.421 | 0.410 |
| 40 | 56.64 | 0.472 | 0.256 | 43.89 | 0.366 | 0.198 |
| 45 | 55.91 | 0.414 | 0.216 | 43.55 | 0.323 | 0.163 |
| 50 | 55.32 | 0.369 | 0.192 | 43.20 | 0.288 | 0.147 |

### 2.2 Uplink

| Students | Wi-Fi 6 total (Mbit/s) | Wi-Fi 6 mean | Wi-Fi 6 worst | Wi-Fi 5 total (Mbit/s) | Wi-Fi 5 mean | Wi-Fi 5 worst |
|---:|---:|---:|---:|---:|---:|---:|
| 5 | 15.00 | 1.000 | 1.000 | 15.00 | 1.000 | 1.000 |
| 10 | 29.56 | 0.985 | 0.946 | 28.10 | 0.937 | 0.755 |
| 15 | 33.62 | 0.747 | 0.238 | 27.18 | 0.604 | 0.143 |
| 20 | 31.73 | 0.529 | 0.038 | 25.71 | 0.428 | 0.026 |
| 25 | 31.46 | 0.419 | 0.020 | 23.83 | 0.318 | 0.000 |
| 30 | 29.75 | 0.331 | 0.000 | 21.52 | 0.239 | 0.000 |
| 40 | 26.73 | 0.223 | 0.000 | 20.69 | 0.172 | 0.000 |
| 50 | 26.10 | 0.174 | 0.000 | 20.76 | 0.138 | 0.000 |

Observations:

1. In downlink, all students receive the full demand until the total demand reaches the capacity of the channel. After that point, every student's share falls by about the same amount (the worst student stays close to the mean), so downlink degrades evenly.
2. In uplink, the total throughput stops increasing at about 30 to 34 Mbit/s for Wi-Fi 6 and 26 to 28 Mbit/s for Wi-Fi 5, then slowly decreases as more students are added. From 15 students upward, the worst student receives less than a quarter of its demand, and from about 25 to 30 students upward at least one student receives nothing.
3. Wi-Fi 6 delivers more than Wi-Fi 5 in every case once the channel is full. OFDMA was off, so this comes from the higher data rate, as in `first-runs.md`.

## 3. Wi-Fi 6 at 5 Mbit/s per Student

| Students | Downlink total (Mbit/s) | Downlink worst | DL OFDMA total (Mbit/s) | DL OFDMA worst | Uplink total (Mbit/s) | Uplink worst |
|---:|---:|---:|---:|---:|---:|---:|
| 5 | 24.99 | 1.000 | 25.00 | 1.000 | 24.98 | 0.997 |
| 10 | 49.88 | 0.996 | 48.41 | 0.966 | 39.28 | 0.419 |
| 15 | 59.63 | 0.771 | 48.55 | 0.645 | 36.55 | 0.014 |
| 20 | 59.10 | 0.578 | 48.59 | 0.483 | 34.40 | 0.006 |
| 30 | 57.87 | 0.377 | 48.59 | 0.323 | 31.15 | 0.000 |
| 50 | 55.40 | 0.115 | 46.70 | 0.100 | 25.21 | 0.000 |

At 5 Mbit/s the network fills up at about 10 to 12 students in downlink. With DL OFDMA the 10-student case still passes (0.966), but the total stays at about 48.6 Mbit/s for 15 students or more, below the 58 to 60 Mbit/s without OFDMA.

## 4. Four Students at 40 Students or More (Fixed)

The cause was a hash collision in the AP's FqCoDel queue disc, fixed in `TIMELINE.md` item 14. The 40+ student rows in this file were produced before the fix.

In downlink with 40 or more students, the worst student receives about half of what the other students receive, and Jain's index falls from about 0.9998 to about 0.98. In each such run, exactly four students are affected. For example, with Wi-Fi 6, 3 Mbit/s, 40 students and run 1, students 8, 13, 23 and 26 received 0.76 to 0.81 Mbit/s while the mean was 1.42 Mbit/s. Which students are affected changes with the number of students. All students are 1 m from the AP, so distance is not the cause. The same pattern appears in the saturated runs (`ofdma.md`, 50 students: minimum 0.4928 Mbit/s against a mean of 0.86 Mbit/s for Wi-Fi 5). It does not affect the limits in Section 1, which are all below 40 students.

## Notes for the Paper

- Sub-question 1 should be answered with these fixed-demand results, not with the saturated runs. The saturated runs only show total / N.
- The demand value must be stated in the configuration table, since the limit depends on it directly.
- These runs use 2 s and 3 runs per setting. The final runs should use the full sweep length.

## Commands

Run from `~/ns-3` after copying `classroom-wifi.cc` into `scratch/`:

```sh
./ns3 run "classroom-wifi --nStations=20 --demand=3 --simulationTime=2s"
./ns3 run "classroom-wifi --standard=ac --nStations=20 --demand=3 --simulationTime=2s"
./ns3 run "classroom-wifi --nStations=10 --demand=3 --downlink=0 --simulationTime=2s"
```

The full set can be run from the repo with:

```sh
DEMAND=3 SIM_TIME=2s RUNS=3 STUDENTS="5 10 15 20 25 30 35 40 45 50" ./sweep.sh
```
