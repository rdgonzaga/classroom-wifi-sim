# Timeline

What has been done so far, in order. Details are in `README.md` and `results/`.

## November 4, 2026

1. **Setup.** Installed ns-3.48 in WSL Ubuntu. Copied ns-3's `wifi-he-network.cc` example as `classroom-wifi.cc`.
2. **Baseline.** Ran the unmodified example (Wi-Fi 6, MCS 5, 20 MHz): 59.72 Mbit/s downlink at 10 students, 57.57 at 30, 27.92 uplink at 30. It only reported total throughput. See `results/baseline.md`.
3. **Problem found.** The example uses one fixed MCS for every student, so a far student can't drop to a slower rate. The 802.11 performance anomaly (sub-question 2) can't appear that way.
4. **Rewrote the script** into our experiment script:
   - `--standard=ax|ac` for Wi-Fi 6 vs Wi-Fi 5
   - per-student throughput (FlowMonitor) and Jain's fairness index
   - `--rateManager=constant|minstrel|ideal` (fixes item 3)
   - `--farDistance` to move one student away
   - one configuration per run, results appended to CSV files
   - `sweep.sh` for 5 to 50 students
5. **Tested.**
   - Downlink matches the baseline exactly.
   - Uplink is slightly different (29.01 vs 27.92) only because students are placed differently now. Confirmed by restoring the old placement.
   - Wi-Fi 6 gets about 32% more than Wi-Fi 5, mostly from its higher data rate.
   - Uplink is very unfair (some students get almost nothing).
   - The far student with `ideal` shows the anomaly: everyone slows down as one student moves away. Past 60 m the student is out of range and the whole network stalls.
   - See `results/first-runs.md`.
6. **Suggestion: OFDMA was off.** OFDMA is Wi-Fi 6's main feature for crowded rooms, and the example had it off. Added `--ofdma` and `--ulOfdma` as optional switches (off by default so earlier results stay valid).
7. **Tested OFDMA.**
   - Downlink OFDMA lowered throughput by 15 to 19% with our saturated traffic, because splitting the channel between 4 students leaves fewer usable subcarriers. Confirmed by changing how many students share a transmission.
   - Uplink OFDMA did nothing at first (bug: the AP never started an OFDMA transmission). Fixed by adding `--muSchedAccessReqInterval`. After the fix it helps the worst student at 10 students, but not at 30.
   - See `results/ofdma.md`.
8. **Suggestion: one run per setting isn't enough.** Uplink results change from run to run because of random collisions (25.7 vs 30.3 Mbit/s for two runs of the same setting). Changed `sweep.sh` to run every setting 5 times (`--RngRun` 1 to 5) so results can be averaged.
9. **Suggestion: traffic is always saturated.** Every student downloads as fast as possible, so per-student throughput is just total ÷ number of students, and sub-question 1 is just division. Planned fix: a fixed demand per student (e.g. 3 to 5 Mbit/s) to find the real breaking point. **Not done yet.**

## Next

- Add the fixed-demand mode (item 9).
- Double-check the whole repo: code, and every number in `results/`.
- Demo on Friday (November 6): short 2 s runs are enough.
- Full 10 s sweep (150 runs) for the paper, after the demand mode is in.

All results so far come from short 2 s test runs, not the final sweep.
