# ABCD Auto Debug Handoff

Date: 2026-07-04

This note is the handoff for continuing the `A` command debug in a fresh
conversation. The target is the full `A -> B -> C -> D -> A` automatic loop.

## Working Rules

- Do not guess interfaces or behavior. Inspect current code, logs, and hardware
  notes before changing anything.
- Discuss behavior-changing logic before editing code.
- Preserve the known-good `F` and `G` behavior unless the user explicitly asks
  to retune them.
- Prefer small, testable state-machine changes over broad rewrites.
- Verify firmware changes with at least `git diff --check` and a Keil rebuild
  when the toolchain is available.

## Active Firmware

Use this project for continued work:

```text
firmware/abcd_fresh/
```

Do not continue from the old `firmware/bringup/` A-mode implementation or from
`firmware/auto_abcd_loop/`.

Current build tag after the last change:

```text
2026-07-04-abcd-fresh-cd-yaw-153
```

Keil project:

```text
firmware/abcd_fresh/keil/abcd_fresh_mspm0g3507_keil.uvprojx
```

Latest local rebuild command used successfully:

```bat
D:\Keil_v5\UV4\UV4.exe -r D:\steven\work\er2024\firmware\abcd_fresh\keil\abcd_fresh_mspm0g3507_keil.uvprojx -o D:\steven\work\er2024\firmware\abcd_fresh\keil\rebuild.log
```

Latest rebuild result:

```text
".\Objects\abcd_fresh_mspm0g3507_keil.axf" - 0 Error(s), 0 Warning(s).
```

Output hex:

```text
firmware/abcd_fresh/keil/Objects/abcd_fresh_mspm0g3507_keil.hex
```

## Known-Good Baselines

### F: white-gap straight

- Purpose: verify and tune blank-area straight driving without relying on the
  MPU6050.
- Current useful values: `base=180`, `gapBias=15`.
- Field gap measurements have been around `1558..1588` odom ticks.
- Recent F behavior before A work: only small drift; acceptable for starting
  A-mode testing.

### G: black-line following

- Purpose: verify plain line PID on the black arc.
- The user reported `G` can currently line-follow normally.
- Keep `G` as the trusted arc-control baseline.

### Y: IMU-assisted straight

- Purpose: isolated white-floor straight test using encoder balance plus
  MPU6050 yaw correction.
- This is the model used by A-mode `AB_STRAIGHT` and `CD_STRAIGHT`.
- It requires a successful uppercase `I` calibration first.
- It has improved straightness, but small left/right variation remains possible.

## MPU6050 Status

Wiring in use:

```text
GY-521 VCC -> 3V3
GY-521 GND -> GND
GY-521 SCL -> PB2
GY-521 SDA -> PB3
GY-521 AD0 -> open or GND
```

`I` initializes and calibrates gyro Z for about 2 seconds. The calibration is a
simple average of `gyroZ`; it does not yet reject motion using variance.

Evidence from the user's logs:

- `biasZ` and `rawZ` were close while still.
- `yawRate100` was near zero while still.
- AB straight yaw drift was small.

Interpretation: the initial IMU calibration looked basically normal. The C
failure was more likely caused by transition logic than by a clearly bad gyro
calibration.

Use the IMU as short-term heading hold. Do not use yaw reaching exactly 180 deg
as the only C-exit gate.

## Current A-State Logic

State order:

```text
AB_STRAIGHT -> B_GUARD -> BC_LINE -> C_ALIGN -> CD_STRAIGHT
            -> D_GUARD -> DA_LINE -> DONE/FAIL
```

Important parameters in `firmware/abcd_fresh/main.c`:

```text
ABCD_WHITE_BLACK_MIN_TICKS     1200
ABCD_WHITE_BLACK_MAX_TICKS     1900
ABCD_ENTRY_PROTECT_MIN_TICKS   15
ABCD_ENTRY_PROTECT_MAX_TICKS   45
ABCD_ARC_ENTRY_GRACE_TICKS     120
ABCD_ARC_EXIT_MIN_TICKS        1500
ABCD_ARC_EXIT_MAX_TICKS        2600
ABCD_ARC_LOST_CONFIRM_TICKS    3
LINE_PID_PERIOD_MS             20
UART baud                      115200
Keil startup Stack_Size        0x800 bytes
```

Straight segments:

- Wait for all-white before starting the segment odometer.
- Drive using encoder balancing plus yaw hold.
- Ignore black before `1200` ticks.
- After `1200` ticks, one black sample triggers the short B/D guard.
- If no black by `1900` ticks, fail with `white missed black`.

B/D guard:

- Very short protection window so an edge hit does not immediately feed a large
  PID correction.
- After `15` ticks, enter line PID if the center group sees black.
- By `45` ticks, enter line PID if any black is still visible.
- If the line is gone by `45` ticks, fail with `entry lost black`.

Arc segments:

- While the line is visible, use the trusted `G` line PID.
- Early `LOST` is tolerated using the last line correction.
- After `1500` ticks, confirmed all-white/LOST for 3 control ticks means arc
  exit.
- Yaw is kept for heading hold and telemetry, not as a hard C-exit gate.
- If still not exited by `2600` ticks, fail with `arc exit missed`.

## Latest Field Evidence

Build under test:

```text
2026-07-04-abcd-fresh-arc-exit-hold
```

User report:

- The car entered `BC_LINE`, but before physically reaching C it started going
  straight.

Useful log facts:

- AB straight was healthy and entered BC normally.
- B transition was not the failure: `BC_LINE` telemetry appeared with
  `point=1`.
- Last visible BC-line sample in the pasted log was around
  `autoOdom=1742`, `line=0b00001000`, `autoYaw=-7195`.
- The next visible status was already `CD_STRAIGHT`, `autoOdom=59`,
  `point=2`, `autoTgt=-7648`.
- Back-calculating from the state reset, the false C transition happened around
  `BC_LINE autoOdom ~= 1930`, with held yaw only about `-75..-76 deg`.

Interpretation:

- This was a false C-exit caused by early arc line loss after the old
  `1500 tick` minimum window.
- It was not a C-align problem: yaw progress was far short of the intended
  C-side heading context.
- A later repeat test, still without flashing the yaw-gate build, false-exited
  after `BC_LINE autoOdom=1642` with `autoHold ~= -101 deg`, then timed out in
  `C_ALIGN`.
- Because the gyro yaw estimate is not reliable enough as a C transition
  requirement, do not use a hard `180 deg` yaw gate and do not rely on the
  unflashed `arc-yaw-gate` experiment.

## Last Code Change

Build tag:

```text
2026-07-04-abcd-fresh-cd-yaw-153
```

Reason for the change:

- Two `cd-yaw-155` field logs completed the full loop, but the car's forward
  direction on CD was still slightly right of the direct C-to-D heading.
- In the latest log, CD held `t=-15500` and actual `y` stayed around
  `-154.5 deg`, so this is a target-angle tuning issue rather than a state
  transition failure.

Implemented change:

- Changed `ABCD_YAW_TARGET_CD_DEG100` from `-15500` to `-15300`.
- Kept `ABCD_ALIGN_MAX_TICKS = 250` and
  `ABCD_ALIGN_YAW_TOL_DEG100 = 500` unchanged, so the next test isolates the
  CD target-angle change.
- C exit is still confirmed by arc odometry plus line/white LOST confirmation;
  yaw is still not a hard C-exit gate.

Previous change:

- The `cd-yaw-align` field log showed C exit at `h=-14569`, then `C_ALIGN`
  targeted `t=-18000`.
- The car turned right and stopped with `align timeout` at `y=-16100`,
  `err=1900`, `stateOdom=254`.
- Since `-143 deg` was visibly left of D and `-161 deg` was visibly right of D,
  the next target was moved between them.
- Changed `ABCD_YAW_TARGET_CD_DEG100` from `-18000` to `-15500`.
- C exit is still confirmed by arc odometry plus line/white LOST confirmation.
- During the short LOST confirmation window, the car still holds the last
  visible arc yaw to avoid reusing the final line PID correction.
- Once C exit is confirmed, `C_ALIGN`, `CD_STRAIGHT`, `D_GUARD`, and
  `D_ACQUIRE` target `ABCD_YAW_TARGET_CD_DEG100` instead of
  `g_abcdHoldYawDeg100`.
- Compact A telemetry includes `t`, the active yaw target, while `h` remains
  the last visible C/arc-exit yaw for comparison.
- Added a per-straight encoder baseline:
  `g_gapStartLeftTicks/g_gapStartRightTicks`.
- `step7_gapResetStraightController()` now records the current left/right
  encoder counts as the start of the next straight segment.
- `step7_gapStraightCorrection()` now computes the position term from
  `(left-startLeft) - (right-startRight)`, while keeping the existing per-loop
  speed-difference term.
- Added `ec` and `ic` to compact A telemetry.
- Removed the unflashed yaw-progress exit gate experiment.
- Changed `firmware/abcd_fresh/empty.syscfg` UART0 to `115200-8N1`; Keil
  rebuild regenerates the UART divisor files.
- Updated `tools/serial_monitor.py` to `BAUD = 115200`.
- Replaced the long automatic-mode periodic telemetry with compact lines.
- Checked the stack concern. The latest Keil static call graph reports maximum
  known stack usage around `392 bytes`; map output shows RAM usage is only about
  `1.43 KB` before stack and the RAM region is `32 KB`.
- Increased `firmware/abcd_fresh/keil/startup_mspm0g350x_uvision.s`
  `Stack_Size` from `0x400` to `0x800` as a conservative safety margin.
- After field testing `fast-pid-stack`, AB straight became twitchy because the
  100 Hz control loop made the existing encoder/IMU straight parameters too
  aggressive. Restore `LINE_PID_PERIOD_MS` to the 20 ms baseline while keeping
  115200 baud, compact A-mode logging, and the 2 KB stack.

- At C, the car could continue curving right after leaving the black arc.
- Likely cause: during the 3-tick white/LOST confirmation window, the old code
  still drove with the last line PID correction, which could be a large right
  turn from the end of the arc.

Implemented change:

- While the arc line is visible, continuously save `g_abcdHoldYawDeg100`.
- During the post-min-distance LOST confirmation window, drive yaw-straight
  toward `g_abcdHoldYawDeg100` instead of reusing the last line PID correction.
- On confirmed C exit, enter `C_ALIGN` and then `CD_STRAIGHT` using that held
  yaw.
- The same arc-exit helper is used for DA, so the behavior also affects the
  final arc exit.

Important: this does not make yaw a hard 180 deg gate. C is still detected by
arc odometry plus confirmed white/LOST.

## Latest Field-Test Conclusion

Build now treated as the current checkpoint candidate:

```text
2026-07-04-abcd-fresh-cd-yaw-153
```

Observed field results after moving the fixed CD heading target to `-15300`:

- Two earlier A-mode runs completed the full ABCDA loop with stable CD behavior.
- A later two-run log on COM11 had:
  - Run 1: normal `DONE`, `point=4`.
  - Run 2: the user noted the car may have started slightly crooked; B and D
    both looked somewhat right-biased. B still entered `BC_LINE` normally.
    Near D, the car stopped with:

```text
[AUTO] entry lost black state=FAIL odom=5169 stateOdom=47 yaw=-15395 tgt=-15300 err=-95 hold=-14297
```

Interpretation:

- This D failure is probably not evidence that `ABCD_YAW_TARGET_CD_DEG100`
  should be retuned. At failure, actual yaw was only about `0.95 deg` from the
  `-153 deg` target.
- Current B and D transition code is symmetric: both use
  `abcd_updateWhiteStraight(...) -> *_GUARD -> abcd_updateGuard(...)`, with the
  shared `15..45` tick entry protection. `B_ACQUIRE` and `D_ACQUIRE` are defined
  but are not currently entered by the state machine.
- D is physically harder than B because it happens after BC arc + C alignment +
  CD white straight, so accumulated placement/heading error can expose the same
  guard logic there first.
- Decision for now: do not change code before checkpointing. Commit this as the
  current recoverable baseline. If D remains flaky, the smallest next change is
  to make D entry more forgiving, preferably by using/widening `D_ACQUIRE`, not
  by changing the CD target immediately.

## Next Test Plan

1. Flash `firmware/abcd_fresh/keil/Objects/abcd_fresh_mspm0g3507_keil.hex`.
2. Confirm the boot banner contains:

```text
Build: 2026-07-04-abcd-fresh-cd-yaw-153
```

3. Open UART at `115200-8N1`.
4. Keep the car still and send uppercase `I`.
5. Confirm `init=OK`, `ready=1`, and near-zero still `yawRate100`.
6. Place the car at A with the sensor slightly into the AB white area, front
   aimed toward B.
7. Send `A`.
8. Watch these compact fields around B and C:

```text
A s od ln st n e c ec ic L R enc y h t lost p
```

Expected B behavior:

- `AB_STRAIGHT` should find black around the known AB gap range.
- `B_GUARD` should last only about `15..45` ticks.
- It should then enter `BC_LINE` and use normal line PID, not continue straight.

Expected C behavior:

- In `BC_LINE`, `autoHold` should update while the line is visible.
- Near C, `st=LOST` / `n=0` should appear after `autoOdom >= 1500`.
- `autoLost` should count up to 3.
- During that confirmation, `L/R` should not keep the old extreme arc turn.
- The next states should be `C_ALIGN`, then `CD_STRAIGHT`.
- In `C_ALIGN` / `CD_STRAIGHT`, `t` should be `-15300` initially; compare `y`
  against `t` to see whether the car actually turns toward D.

## How To Interpret Failures

- If AB drives across B without entering `BC_LINE`, inspect B/D guard and black
  detection windows first.
- If it fails with `entry lost black`, inspect whether this only happens at D
  and whether CD yaw is still near `-15300`. If so, treat it as an entry/acquire
  robustness issue before retuning the CD heading target.
- If it fails with `arc lost early`, C/D arc exit happened before `1500` ticks;
  reduce `ABCD_ARC_EXIT_MIN_TICKS` only after checking the log.
- If it fails with `arc exit missed`, either the sensor never saw all-white or
  `ABCD_ARC_EXIT_MAX_TICKS` is too short.
- If it enters `CD_STRAIGHT` but misses D, tune `ABCD_YAW_TARGET_CD_DEG100`
  first. Do not jump directly to a hard 180 deg C-exit gate.
- If `imu lost` appears, re-check wiring and repeat `I`.

## Open Risks

- `ABCD_ARC_EXIT_MIN_TICKS` and `ABCD_ARC_EXIT_MAX_TICKS` are provisional.
  BC and DA may need separate windows after more logs.
- `ABCD_YAW_TARGET_CD_DEG100` is now a good checkpoint candidate at `-15300`,
  but it is still field-derived. Retune only if repeated clean starts show a
  consistent CD aiming bias.
- B/D entry protection is shared and short. D may need a more forgiving acquire
  path even if B remains reliable.
- The MPU6050 calibration has no stillness variance check yet.
- The working tree also contains other modified `abcd_fresh` files from earlier
  steps; inspect `git diff` before making unrelated edits.
