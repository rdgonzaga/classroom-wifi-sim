# Timeline

What has been done so far, in order. Details are in `README.md` and `results/`.

## November 4, 2026

1. Setup. Installed ns-3.48 in WSL Ubuntu. Copied ns-3's `wifi-he-network.cc` example as `classroom-wifi.cc`.
2. Baseline. Ran the unmodified example (Wi-Fi 6, MCS 5, 20 MHz): 59.72 Mbit/s downlink at 10 students, 57.57 at 30, 27.92 uplink at 30. It only reported total throughput. See `results/baseline.md`.
3. Problem found. The example uses one fixed MCS for every student, so a far student can't drop to a slower rate. The 802.11 performance anomaly (sub-question 2) can't appear that way.
4. Rewrote the script into our experiment script:
   - `--standard=ax|ac` for Wi-Fi 6 vs Wi-Fi 5
   - per-student throughput (FlowMonitor) and Jain's fairness index
   - `--rateManager=constant|minstrel|ideal` (fixes item 3)
   - `--farDistance` to move one student away
   - one configuration per run, results appended to CSV files
   - `sweep.sh` for 5 to 50 students
5. Tested.
   - Downlink matches the baseline exactly.
   - Uplink is slightly different (29.01 vs 27.92) only because students are placed differently now. Confirmed by restoring the old placement.
   - Wi-Fi 6 gets about 30% more than Wi-Fi 5 in downlink and 25% more in uplink, mostly from its higher data rate.
   - Uplink is very unfair (some students get almost nothing).
   - The far student with `ideal` shows the anomaly: everyone slows down as one student moves away. At 60 m or more the student is out of range and the whole network stalls.
   - See `results/first-runs.md`.
6. Suggestion: OFDMA was off. OFDMA is Wi-Fi 6's main feature for crowded rooms, and the example had it off. Added `--ofdma` and `--ulOfdma` as optional switches (off by default so earlier results stay valid).
7. Tested OFDMA.
   - Downlink OFDMA lowered throughput by 15 to 19% with our saturated traffic, because splitting the channel between 4 students leaves fewer usable subcarriers. Confirmed by changing how many students share a transmission.
   - Uplink OFDMA did nothing at first (bug: the AP never started an OFDMA transmission). Fixed by adding `--muSchedAccessReqInterval`. After the fix it helps the worst student at 10 students, but not at 30.
   - See `results/ofdma.md`.
8. Suggestion: one run per setting isn't enough. Uplink results change from run to run because of random collisions (28.2 to 30.4 Mbit/s over five runs of 30 students uplink). Changed `sweep.sh` to run every setting 5 times (`--RngRun` 1 to 5) so results can be averaged.
9. Suggestion: traffic is always saturated. Every student downloads as fast as possible, so per-student throughput is just total ÷ number of students, and sub-question 1 is just division. Planned fix: a fixed demand per student (e.g. 3 to 5 Mbit/s) to find the real breaking point.
10. Added the fixed-demand mode. `--demand` sets a fixed rate per student (0 keeps the old saturated traffic, and the baseline numbers still match). The output now includes how much of the demand each student received. `sweep.sh` takes `DEMAND` and `STUDENTS`.
    - Short sweep: 3 and 5 Mbit/s, 5 to 50 students, 3 runs each.
    - At 3 Mbit/s per student, Wi-Fi 6 downlink serves 20 students before someone gets less than 95% of the demand, Wi-Fi 5 serves 15, and Wi-Fi 6 with DL OFDMA serves 15. Uplink already fails at 10 students for both.
    - Open question: with 40 or more students in downlink, four students get about half the share of the others. Cause not found yet.
    - See `results/demand.md`.
11. Double-checked the results. Re-ran every number in `results/`. All matched. Fixed the Wi-Fi 6 vs Wi-Fi 5 percentage (25% in uplink, not 32%) and made the "near student" column in the far-student tables a mean of the nine near students.

## November 6, 2026

12. Changes for the paper's full sweep.
    - `--layout=grid`: classroom seats instead of a circle. 7 m x 9 m room, 8 x 7 = 56 seats behind a 2 m teacher's area, desk height 0.75 m, AP on the ceiling at the center (3 m). `circle` stays the default so the baseline still matches. Students are now 2.27 to 5.53 m from the AP (3D distance).
    - Mean delay and packet loss per student from FlowMonitor, plus the min/max throughput ratio next to Jain's index.
    - The simulation now runs 1 s longer after the apps stop so queued packets arrive or get dropped. Throughput still only counts up to when the apps stop. Without this, FlowMonitor's loss count missed packets dropped in the Wi-Fi queue (82 counted vs 144 actually missing in one saturated run), and packets still queued at the end looked lost.
    - `--rtsCts=1` turns on RTS/CTS for every frame. ns-3's default threshold (4692480 bytes) is never reached by our A-MPDUs (65535 bytes max), so earlier runs never used RTS/CTS.
    - `sweep.sh` takes `LAYOUT`.
13. Checked.
    - Same throughput and Jain's index as before for: 10 students saturated (59.7212), 30 students uplink (29.0108), far student at 50 m with `ideal` (90.8824), and 20 students at 3 Mbit/s (worst student 0.951).
    - Grid vs circle, 30 students, MCS 5: identical throughput and Jain's index (57.5652, 0.999836). Only delay changes slightly.
    - Grid with `ideal`: identical to circle at 30 and 56 students, so no student drops MCS. Even the back corner seats (5.53 m) have about 41 dB SNR, more than MCS 11 needs.
    - 20 students at 3 Mbit/s: mean delay 42 ms, loss 0.1%.
14. Found the cause of the four students (item 10). In downlink all flows leave the AP through ns-3's default queue disc, FqCoDel. It hashes each flow into one of 1024 queues and serves the queues equally, so two students whose flows land in the same queue each get half a share. With 40 flows the chance of at least one collision is about 50%. Changing FqCoDel's hash salt (`Perturbation`) moved the effect to other students, and 65536 queues removed it. Uplink is not affected, since each student's queue disc only carries one flow.
    - Fix: `Flows` is now set to the full 32-bit range. ns-3 only creates a queue when a flow first uses it, so this costs nothing.
    - 40 students, 3 Mbit/s, run 1: Jain's index 0.978 to 0.9998, min/max 0.50 to 0.96 (same for grid). 50 students saturated, Wi-Fi 5: min 0.49 to 0.86 Mbit/s.
    - Unchanged (2 s runs, as in `baseline.md`): 1 student 59.9284, 10 students 59.7212 (Jain 0.999944), 30 students 57.5652, 30 students uplink 29.0108 (Jain 0.751592), 20 students at 3 Mbit/s (worst 0.951067).
    - The baseline numbers are for 2 s. The script's default is 10 s, which gives slightly different values (10 students: 59.8914), so always pass `--simulationTime=2s` when comparing against `results/`.

## Next

- Demo on Friday (November 6): short 2 s runs are enough.
- Full 10 s sweep for the paper, saturated and with `--demand`.

All results so far come from short 2 s test runs, not the final sweep.
