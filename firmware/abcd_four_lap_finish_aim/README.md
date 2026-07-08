# ER2024 Four-Lap Finish-A Aim Firmware

This project is a new independent experiment copied from
`firmware/abcd_vision_link`. The old `abcd_fresh` and `abcd_vision_link`
projects are not modified by this branch.

Goal: keep the proven ABCD motion baseline, run 4 continuous laps, then perform
one final aiming task after the fourth lap exits A.

## Build

Keil project:

```text
firmware/abcd_four_lap_finish_aim/keil/abcd_four_lap_finish_aim_mspm0g3507_keil.uvprojx
```

Command-line rebuild:

```bat
D:\Keil_v5\UV4\uVision.com -r D:\steven\work\er2024\firmware\abcd_four_lap_finish_aim\keil\abcd_four_lap_finish_aim_mspm0g3507_keil.uvprojx -o D:\steven\work\er2024\firmware\abcd_four_lap_finish_aim\keil\build.log
```

Output hex:

```text
firmware/abcd_four_lap_finish_aim/keil/Objects/abcd_four_lap_finish_aim_mspm0g3507_keil.hex
```

Expected boot tag:

```text
Build: 2026-07-09-abcd-four-lap-finish-aim
```

## Runtime Commands

| Command | Effect |
| --- | --- |
| `Q` / `q` | Run 4 continuous laps, then align at final A and wait for K230 aim DONE. |
| `A` / `a` | Keep the original single-lap ABCD baseline. |
| `T` / `t` | Keep the older one-lap B-point vision-link baseline. |
| `F` | Run the old white-to-black straight test. |
| `G` | Run the old continuous black-line PID test. |
| `g` | Run the old Step7 encoder-distance lap stop. |
| `I` | Initialize MPU6050 and calibrate gyroZ while the car is still. |
| `i` | Read one MPU6050 sample after `I`. |
| `Y` / `y` | Run the isolated IMU-assisted straight test after `I`. |
| `b` | Short brake. |
| `x` | Coast stop and return to standby. |
| `?` | Print help and parameters. |

The physical B21/PB21 button now starts the `Q` mode after the existing
3-second countdown and still-car IMU calibration.

## Vision Link

Use common ground and 3.3 V logic only:

```text
MSPM0 PB11 -> K230 GPIO03  START, active high
MSPM0 PB14 <- K230 GPIO04  DONE, active high
GND        -- K230 GND     common ground
```

For `Q`, the MSPM0 keeps `START` low during all 4 laps. After the fourth lap's
DA arc exits A, the firmware enters `A_ALIGN`, then brakes in `A_WORK`, drives
`START` high, and waits for `DONE` high to be stable for 40 ms. A valid DONE
finishes the run. A 15 s timeout is treated as failure because the final aiming
task is required.

Matching K230 script:

```text
vision/k230_a_finish_aim_handshake.py
```

It aims at the black target frame, turns the laser on for 3 seconds after the
aim is stable, then holds DONE high until START goes low.

## Four-Lap State Strategy

One lap still follows:

```text
AB_STRAIGHT -> B_GUARD -> BC_LINE -> C_ALIGN -> CD_STRAIGHT
            -> D_GUARD -> DA_LINE
```

When `DA_LINE` exits A:

1. Increment `lap`.
2. If `lap < 4`, enter `A_NEXT_ALIGN`, keep driving toward the next-lap AB
   heading target, then rebase the local ABCD yaw reference to 0 deg and enter
   the next `AB_STRAIGHT`.
3. If `lap == 4`, enter `A_ALIGN`, then `A_WORK`.

This avoids using absolute yaw targets such as `-72000` or `-108000`, which
would exceed the existing ABCD yaw clamp. Each lap keeps the same proven
targets:

```text
AB target: 0 deg
CD target: -153 deg
DA/A target: -360 deg
```

Final aim angle is separated as:

```c
#define ABCD_YAW_TARGET_AIM_DEG100 (-36000L)
```

Next-lap AB re-entry angle is also separated as:

```c
#define ABCD_YAW_TARGET_NEXT_AB_DEG100 (-32500L)
```

Tune `ABCD_YAW_TARGET_AIM_DEG100` only if the car reaches final A reliably but
the K230 target is not in view or is consistently biased after A exit.

Tune `ABCD_YAW_TARGET_NEXT_AB_DEG100` if the next lap starts left or right and
misses B. Keep it separate from the final aiming angle so motion re-entry and
target pointing can be debugged independently. Do not retune
`ABCD_YAW_TARGET_CD_DEG100` unless CD/D entry is repeatedly biased during the
lap motion itself.

Current field-derived values (2026-07-09, stable 4-lap baseline):

| Parameter | Value | Notes |
|-----------|-------|-------|
| `CD_YAW` | `-15000` | CD straight heading. Tuned from -15300: car was right-biased and missed D |
| `NEXT_AB` | `-30300` | Next-lap AB re-entry. Tuned: -325 too right, -300 too left, -303 balanced |
| `AIM` | `-36000` | Final A-point aim target. A_ALIGN: short 150-tick correction then enter A_FINISH_WORK |
| `CORR_MAX` | `40` | IMU correction cap. Encoder balance disabled during align states |

A_ALIGN strategy: `abcd_driveYawAlign` (encoder balance off, pure IMU yaw correction),
150 ticks max then unconditionally enter `A_FINISH_WORK` to trigger K230.
K230 handles fine aiming via camera + servo gimbal.

## First Test Order

1. Flash this project's hex and confirm the boot tag above.
2. Run `I`, then confirm `ready=1` and near-zero still `yawRate100`.
3. Run `Y`, `F`, and `G` as quick regressions.
4. Run `A` once to confirm the original single-lap baseline still works.
5. Run `Q` with K230 disconnected or DONE forced low once, only to confirm it
   reaches final `A_WORK` and times out safely.
6. Run the K230 script and test `Q` with the START/DONE wiring connected.

Compact AUTO telemetry includes:

```text
lap=<completed>/<target>
```

For `Q`, expected lap progress is `0/4`, then `1/4`, `2/4`, `3/4`, and finally
`4/4` before `A_ALIGN` / `A_WORK`. After laps 1-3, expect a short
`A_NEXT_ALIGN` segment before the next `AB_STRAIGHT`.
