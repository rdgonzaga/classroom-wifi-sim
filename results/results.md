# Results Summary

Short version of the final results. Full tables are in `final.md` (configuration A) and `final-mixed.md` (configurations B and C). All numbers are averages of 5 runs of 10 s each, on the classroom seat grid, at 5 GHz, 20 MHz, 800 ns GI and MCS 5, unless stated otherwise.

| Configuration | AP | Devices per student | Fixed demand |
|---|---|---|---|
| A. Baseline | 1x1 | One 1x1 device | 3 or 5 Mbit/s |
| B. Device mix | 2x2 | A 1x1 phone, plus a 2x2 laptop with probability p | Phone 1, laptop 3 Mbit/s |
| C. Control | 2x2 | One 2x2 device | 3 Mbit/s |

## Answer to the Research Question

Wi-Fi 6 handles a crowded classroom better than Wi-Fi 5, but mostly because of its higher PHY rate at the same MCS (68.8 vs 52.0 Mbit/s per stream, a 1.32x ratio), not its multi-user features. DL OFDMA lowered capacity in our setup. Downlink stays fair as students are added; uplink does not.

## Sub-question 1: How Many Students One AP Can Serve

Largest number of students for which every device still gets at least 95% of its demand. In brackets: the first count where the worst device's delay passes 150 ms (ITU-T G.114).

| Configuration | A (3 Mbit/s) | A (5 Mbit/s) | C | B, p = 1 | B, p = 0.5 |
|---|---:|---:|---:|---:|---:|
| Wi-Fi 6, downlink | 20 (25) | 10 | 35 (45) | 20 (25) | 30 (35) |
| Wi-Fi 6, downlink, DL OFDMA | 15 (25) | 5 | 30 (45) | - | - |
| Wi-Fi 5, downlink | 15 (20) | 5 | 25 (40) | 15 (20) | 20 (30) |
| Wi-Fi 6, uplink | 10 (15) | 5 | 15 (20) | 10 (15) | 10 (20) |
| Wi-Fi 5, uplink | 5 (15) | 5 | 15 (20) | 5 (10) | 10 (15) |

- Wi-Fi 6 serves 5 to 10 more students than Wi-Fi 5 in most cases. The exceptions are uplink in C and at 5 Mbit/s, where both serve the same number.
- The delay limit is never reached before the throughput limit, so the 95% rule alone decides usability.
- Two streams on every link (C) serve about 1.5x to 3x as many students as A.
- B at p = 1 has the same limits as A: its capacity is about 1.32x A's, but each student also asks for 1.33x more (4 vs 3 Mbit/s).
- In downlink, laptops fail first and phones stay near 100%, since the AP gives every device about the same throughput.
- Downlink limits match capacity divided by demand (59.9 / 3 = 20 for Wi-Fi 6, 45.3 / 3 = 15 for Wi-Fi 5).

## Capacity and Fairness (Saturated Traffic)

Total throughput (Mbit/s) at 5 and 50 students:

| Configuration | Wi-Fi 6 DL | Wi-Fi 6 DL OFDMA | Wi-Fi 5 DL | Wi-Fi 6 UL | Wi-Fi 5 UL |
|---|---:|---:|---:|---:|---:|
| A, 5 students | 59.91 | 48.61 | 45.32 | 48.73 | 36.91 |
| A, 50 students | 55.41 | 48.59 | 43.24 | 26.89 | 20.27 |
| C, 5 students | 115.45 | 97.42 | 89.27 | 97.27 | 75.24 |
| C, 50 students | 99.82 | 90.17 | 80.04 | 57.41 | 44.20 |
| B (p = 1), 5 students | 78.90 | - | 60.14 | 57.75 | 47.30 |
| B (p = 1), 50 students | 62.72 | - | 50.98 | 31.52 | 23.72 |

- From 5 to 50 students, downlink total drops 5 to 8% in A and 10 to 21% in B and C. Uplink drops 40 to 50% everywhere, because more students means more collisions.
- Downlink fairness stays at Jain 1.00 for every configuration. Uplink falls to about 0.88 (A) and 0.82 (C) at 50 students, and to 0.63 in B, which has twice as many STAs contending. The slowest uplink student gets 13 to 14% of the fastest one's throughput in A.
- DL OFDMA holds a flat 48.6 Mbit/s in A, 81% of Wi-Fi 6 without it, because resource units use fewer data subcarriers (192 of 234). It does give lower delay at 5 and 10 students.
- In B, laptops and phones get the same downlink throughput. The AP shares per packet, not per airtime, so the 2x2 laptops raise total capacity without getting more than phones.
- Delay levels off at about 400 to 550 ms after saturation, because the MAC queue drops packets older than 500 ms. The extra load shows up as loss instead.

## Sub-question 2: One Far Student

10 students, saturated downlink, Ideal rate manager. Change in each near student's throughput compared with no far student:

| Far distance (m) | Wi-Fi 6 A | Wi-Fi 6 C | Wi-Fi 6 B | Wi-Fi 5 A | Wi-Fi 5 C | Wi-Fi 5 B |
|---:|---:|---:|---:|---:|---:|---:|
| 10 | -2.3% | -2.1% | -2.2% | 0.0% | 0.0% | 0.0% |
| 30 | -9.2% | -8.6% | -8.7% | -4.7% | -4.4% | -4.5% |
| 50 | -23.2% | -21.5% | -22.0% | -16.6% | -15.8% | -16.1% |

- Yes, one far student slows everyone down (the 802.11 performance anomaly). The far student gets the same throughput as the rest, so its slow transmissions take airtime from all of them.
- The effect is about the same in A, B and C. More streams or devices don't protect the near students.
- Wi-Fi 6 loses more in percentage terms, since its near students run at a higher rate.

## Caveats

- Most of Wi-Fi 6's lead comes from the PHY rate at the same MCS, with OFDMA off.
- B skips DL OFDMA (ns-3.48 asserts on mixed 1- and 2-stream MU transmissions), and UL OFDMA was not run (it needs a 1600 ns GI).
- B's limits depend on our assumed 1 and 3 Mbit/s demands.
- Uplink varies a lot between runs, so report its 5-run spread.
- Saturated Wi-Fi 5 downlink at 10 students splits unevenly (Jain 0.99, min/max 0.81). The cause was not investigated; it doesn't change any limit above (`final.md` Section 6).
