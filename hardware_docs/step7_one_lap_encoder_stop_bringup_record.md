# Step 7 One-Lap Encoder Stop Bring-Up Record

Date: 2026-07-01

Status: firmware prepared and build-verified for field testing. The car has not
yet been validated on a full A-to-A lap with the final track target value.

## Goal

Step 7 is the first basic-function scoring run:

```text
A start -> line following -> encoder odometer -> return to A -> automatic stop
```

The first implementation intentionally uses encoder ticks instead of centimetres
because the exact 520 motor encoder CPR is still not measured. The wheel
diameter is now known as `65 mm`, so the wheel circumference is about
`204.2 mm`. The first manual wheel test measured about `332.6` absolute encoder
ticks per wheel revolution, or about `0.614 mm/tick`. A later wheels-up
motor-driven `f` jog confirmed both logical encoders increase during actual
forward motion, so the current encoder orientation is usable.

## Active Firmware

```text
D:\steven\work\er2024\firmware\bringup\keil\bringup_mspm0g3507_nortos_keil.uvprojx
```

Step 7 keeps the accepted Step 6 control baseline:

```text
base=180 Kp=55 Ki=0 Kd=35
orient mSwap=1 invA=1 invB=1 sensorFlip=1 eSwap=1 eInvA=1 eInvB=1
```

New Step 7 behavior:

| Item | Value |
| --- | --- |
| `A` | Segmented auto run for the current non-closed field: AB white straight, BC black arc, CD white straight, DA black arc, stop near A |
| `g` | Start one-lap run from A, clear encoder counters, stop at target |
| `G` | Continuous line-following mode for Step 6 retest |
| `F` | Gap test: wait for all white, clear odometry, drive straight with encoder closed-loop balancing, stop and save result after 3 consecutive black detections |
| `,` / `.` | Tune `F` mode straight-run right-wheel bias down/up by 5 |
| Default lap target | `26000` encoder ticks |
| Lap target adjustment | `[` = -1000 ticks, `]` = +1000 ticks |
| Odometer | Average of logical `encL` and `encR`, negative counts clamped to 0 |
| Stop action | TB6612 short brake, `lap=DONE`, PB22 fast blink |
| Telemetry rate | 200 ms |
| Short lost-line handling | `st=LOST` tolerated for about 300 ms, then short brake |
| Full black handling | `st=FULL` immediate short brake |
| Segmented telemetry | `auto=... seg=AB/BC/CD/DA segOdom=... autoReady=...` |

Normal Step 7 telemetry includes:

```text
mode=RUN line=0b00011000 st=OK n=2 err=0 corr=0 base=180 L=180 R=180 encL=123 encR=119 odom=121 tgt=26000 gap=0 lap=RUN
```

## Bench Check Before Floor Run

1. Put the car on a stand with wheels lifted.
2. Open the onboard CH340 serial port at `9600-8N1`.
3. Confirm boot banner says `Step7` and `mode=STBY`.
4. Send `?`; confirm:
   - `base=180 Kp=55 Ki=0 Kd=35`
   - `orient mSwap=1 invA=1 invB=1 sensorFlip=1 eSwap=1 eInvA=1 eInvB=1`
   - `lapTarget=26000`
5. Send `r`; confirm `odom=0`.
6. Send `f` for a short wheels-up jog. Confirm `encL`, `encR`, and `odom`
   increase. Send `x`. This was confirmed on 2026-07-01 with
   `encL 0 -> 1384`, `encR 0 -> 1102`, and `odom 0 -> 1243`.
7. Put black tape under the center of the sensor and send uppercase `G`.
   Confirm continuous line PID still drives both wheels forward. Send `x`.
8. Send lowercase `g` with the same centered line pattern. Confirm `lap=RUN`
   and `odom` starts from 0. Send `x` before the wheels run for long.

Do not start the floor test if either logical encoder decreases during forward
motion. Fix encoder orientation with `e`, `3`, or `4` first.

## First Floor Run

1. Keep gimbal, laser, spotlight, and extra motor loads disconnected.
2. Keep the same motor rail and common-ground wiring used in Step 6.
3. Start with `base=180`; do not raise speed until lap stop works.
4. Place the car at A, centered on the black line, pointing in the intended lap
   direction.
5. Send `r`, then lowercase `g`.
6. Keep one hand near the motor power switch or serial `b` command.
7. Watch these fields:
   - `lap=RUN`
   - `odom` increasing smoothly
   - `st=OK` most of the time
   - `gap` briefly nonzero only over dashed gaps
8. If it reaches A and does not stop, send `b`, note the `odom` value at A, and
   decrease the target with `[` because the current `tgt` is too large.
9. If it stops before A, send `x`, press `]` one or more times, reset with `r`,
   return to A, and try again.
10. When it stops close to A, reduce or increase the target by 1000-tick steps
    to compensate for braking overshoot.

Step 7 is considered passed when the car starts from A, follows one lap, stops
back near A automatically, and PB22 fast-blinks after stopping.

For the current non-closed field, use uppercase `A` instead of lowercase `g` for
the first integrated run. Expected segment order:

```text
AB white straight -> BC black arc -> CD white straight -> DA black arc -> DONE
```

The first test goal is only to confirm correct segment transitions. Keep one
hand near the motor power switch or serial `b` command. Watch:

- `auto=RUN`
- `seg=AB`, then `BC`, then `CD`, then `DA`
- `segOdom` increasing from 0 at each segment
- `autoReady=0` means AB/CD is waiting for all-white before starting straight
  control; `autoReady=1` means the straight controller is active
- `auto=DONE` after DA, or `auto=FAIL` if a guard condition stops the car

For AB and CD, automatic mode intentionally matches the tested `F` start
behavior: motors stay off until all sensors see white, then odometry and the
straight controller are reset at that exact white start point. If the car is
placed with the sensor still on the A/D black endpoint, `A` will wait rather
than drive away. Put the sensor just into the white gap for the first AB test.

During `A`, serial commands other than `b/B` are ignored. This prevents stray
UART bytes or accidental text input from switching the car back to standby or
resetting the odometer mid-run. Use `b` or the motor power switch for emergency
stop.

BC/DA arc entry is intentionally tolerant: a short initial `FULL` or `LOST`
reading at the black-line entrance is treated as acquisition, not as an
immediate failure.

## Gap Segment Test

Use uppercase `F` to measure the blank A-to-B or C-to-D segment on the current
non-closed field. This command is designed for unplugged field testing: it can
be armed while the sensor is still on black, starts only after all sensors see
white, and keeps the final result in periodic telemetry after the USB cable is
reconnected.

1. With the USB cable connected, send uppercase `F`.
2. Unplug the USB cable if needed and place the car at the start of the blank
   segment, centered and aimed toward the next black arc entrance.
3. The car waits with motors off while it sees black.
4. When telemetry would show all white, the firmware clears odometry and starts
   driving:

   ```text
   line=0b00000000 st=LOST n=0
   ```

5. The car drives straight while the sensors remain white. It now uses encoder
   speed-plus-position balancing, so `gapDL` and `gapDR` show the latest
   left/right encoder increments and the slower side gets boosted on the next
   control tick.
6. If a single noisy black sample appears in the white area, the firmware keeps
   running and resets `gapBlk` back to 0 after white returns.
7. When any channel detects black (`n > 0`) for 3 consecutive control ticks,
   the firmware short-brakes and prints:

   ```text
   [GAP] black detected; brake at odom=...
   ```

8. Reconnect the USB cable and read the saved periodic telemetry:

   ```text
   gapTest=DONE gapOdom=... gapRaw=0b... gapBlk=3 gapDL=... gapDR=...
   ```

9. Record `gapOdom` as the first estimate for the blank segment length.

If the car curves right in the blank segment, the physical right wheel is still
too slow; send `.` before the next run to increase `gapBias`. If it curves left,
send `,` to reduce `gapBias`. The current field-calibrated default is
`gapBias=15`. If `gapDR` stays much smaller than `gapDL` even while telemetry
shows `R` much larger than `L`, treat it as a hardware problem first: check
whether the right tire rubs, the gearbox binds, motor wires are loose, or the
TB6612 right-side output has a poor connection.

Do not send `r`, `x`, `b`, or another movement command before reading the
result, because those commands clear the saved GAP test state.

## Notes For Step 8

- Record the final `tgt` value and the actual stop offset from A.
- Step 8 should replace the pure odometer threshold with key-point detection or
  odometer windows for B/C/D/A.
- If dashes are longer than the 300 ms lost-line grace at `base=180`, increase
  grace only after verifying the car does not leave the track during a forced
  white-gap test.

## 2026-07-01 Gap Measurements

First A-to-B blank segment test with uppercase `F`:

```text
[GAP] black detected; brake at odom=1587 raw=0b10000000
final telemetry: encL=1590 encR=1603 odom=1596 gapTest=DONE gapOdom=1587 gapRaw=0b10000000 gapBias=20
```

Interpretation:

- `AB_GAP_TICKS` initial value: `1587`.
- Using `0.614 mm/tick`, this is about `974 mm`, close to the nominal `100 cm`
  AB length.
- `encL` and `encR` differed by only `13 ticks` at the end, so `gapBias=20`
  plus encoder balancing is a good first straight-run setting.
- `gapRaw=0b10000000` means the confirmed black was at one edge of the
  sensor array. For the later full state machine, this is suitable as an arc
  entrance trigger; the next state should switch from blank straight drive to
  black-line acquisition/line following instead of stopping permanently.

## 2026-07-02 Gap Calibration

Straight white-gap run with the improved encoder balancing:

```text
[GAP] black detected; brake at odom=1573 raw=0b00010000
final telemetry: encL=1583 encR=1585 odom=1584 gapTest=DONE gapOdom=1573 gapRaw=0b00010000 gapBlk=3 gapDL=5 gapDR=6 gapBias=15
```

Interpretation:

- Use `gapBias=15` as the current default straight-run setting.
- `encL` and `encR` differed by only `2 ticks`, and the observed path was
  straight.
- `AB/CD_GAP_TICKS` should stay near `1575` to `1590`; keep `1588` as a safe
  first target for the later segment state machine until more repeat runs are
  collected.

Additional straight runs from the same session:

```text
[GAP] black detected; brake at odom=1576 raw=0b00001000
final telemetry: encL=1582 encR=1591 odom=1586 gapTest=DONE gapOdom=1576 gapRaw=0b00001000 gapBlk=3 gapDL=5 gapDR=5 gapBias=15

[GAP] black detected; brake at odom=1558 raw=0b00010000
final telemetry: encL=1568 encR=1569 odom=1568 gapTest=DONE gapOdom=1558 gapRaw=0b00010000 gapBlk=3 gapDL=6 gapDR=6 gapBias=15
```

These confirm `base=180` and `gapBias=15` are good enough to start segmented
automatic testing.

### 2026-07-02 A-mode AB->BC field test

Command `A` completed AB white straight and entered BC. At the C-side white
exit it stopped with:

```text
[AUTO] arc lost early at segOdom=1888
auto=FAIL seg=FAIL
```

This means the car physically reached the C blank area, but the firmware still
used the earlier BC target estimate too strictly. Because the car may enter
BC slightly after the ideal B point, the remaining arc odometry can be shorter
than the standalone B-to-C measurement. For A-mode, use an exit window instead:
BC accepts confirmed white exit after `1500 ticks` and force-transitions by
`2500 ticks`; DA starts with a wider `1800..2800 ticks` window until measured
in the full automatic run.

Follow-up A-mode log reached BC white exit at `segOdom=2004`, which is inside
the new BC exit window. The car still stopped around C because CD straight
startup reused the old odometry value from BC on the same control tick that it
reset the encoders. Firmware now forces `g_lastOdomTicks=0` immediately after
the white-start odometry reset, so the CD straight timeout check starts from
the CD white segment instead of the previous arc.

Another A-mode log completed AB but then printed `enter seg=OFF` at B instead
of entering BC. The AB straight telemetry was otherwise healthy, so the
straight controller itself was not the cause. Firmware now makes each helper
return only "segment complete" and lets the outer A-state `switch` choose the
next segment explicitly: AB->BC, BC->CD, CD->DA, DA->DONE. It also prints
`[AUTO] transition to=... code=...` before each segment change. The next AB
test should show `transition to=BC code=2` followed by `enter seg=BC`.

The next field log did show `transition to=BC code=2 from=AB`, so the explicit
AB->BC decision was correct. The car still failed around B, and the following
test showed a worse symptom: AB kept driving straight through B instead of
catching the arc. Code review found two control-order problems:

- black detection in A-mode reused the more conservative F-mode 3-sample black
  confirmation, so a short B-line detection could be missed
- transition telemetry was printed before the arc motor command, and at 9600
  baud this left the car driving straight while it was already at the B entry

Firmware build `2026-07-02-auto-direct-pid` removes the earlier shadow-recovery
and fixed arc-acquire experiments, then makes the A-state machine simpler:

- AB/CD use one-sample black confirmation after the minimum odometer window
- AB/CD brake with `gap missed black` if the black line is missed past the
  guard window
- AB->BC and CD->DA immediately run the normal line PID from the current
  black-line sensor error
- BC/DA `LOST` uses the previous normal PID correction briefly, then fails or
  transitions by the measured odometer window; there is no fixed turning
  search command

For the next test, B entry should show `transition to=BC code=2` from black
detection, then `corr` should come from the normal line PID rather than a fixed
arc-acquire value.

### 2026-07-02 A-mode direct-PID B-entry failure

Build under test: `2026-07-02-auto-direct-pid`.

Observed behavior:

- AB white-gap straight was stable.
- B black line was detected correctly.
- The firmware printed `transition to=BC code=2 from=AB segOdom=1573`.
- Immediately after `enter seg=BC`, the car stopped instead of following BC.

Key serial evidence transcribed from the field screenshot:

```text
[AUTO] transition to=BC code=2 from=AB segOdom=1573
[AUTO] enter seg=BC odom=1589 ... autoReady=0
mode=STBY line=0b00000011 st=OK n=2 err=-300 corr=0 base=180 L=0 R=0 ... auto=BAD seg=BAD segOdom=16686 autoReady=0
```

Interpretation:

- This is not an AB straight-drive problem. AB reached the expected distance
  range and found the black line at B.
- This is not a grayscale detection failure. At B, telemetry had
  `line=0b00000011 st=OK n=2 err=-300`, which is a usable line reading.
- The failure is inside the A-mode transition/state handling immediately after
  AB->BC. A valid line was present, but `g_driveMode` appeared as `STBY`,
  motor commands were `L=0 R=0`, and `auto/seg` became `BAD`.
- The `enter seg=BC` line also showed impossible metadata (`autoReady=0` for
  BC, and a corrupted-looking `segOdom` field), so the next rewrite should not
  keep layering patches on the current AUTO implementation.

Rewrite recommendation:

- Reuse the proven low-level modules from `main.c`: sensor reading, motor
  output, encoder transforms, Step 6 line PID, and F-mode straight balancing.
- Rewrite only the A/B/C/D automatic state machine as a small fresh module.
- Use monotonic odometry plus `segmentStartOdom`; avoid resetting encoder
  counters inside every segment transition.
- Keep F-mode test logic separate from A-mode logic. AB/CD may reuse the same
  straight-balance calculation, but should not share gap-test state variables
  such as `g_gapTestState` and `g_gapBlackTicks`.
- Keep transition actions nonblocking and minimal. If UART logging is needed,
  print short event lines after motor commands have already been set.
