# Step 6 PID Line-Following Bring-Up Record

Date: 2026-06-30; orientation update: 2026-07-01

Status: Step 6 low-speed line following is bench- and floor-validated. The car
can jog forward, stop, enter line-following mode, and track the line on the
floor with the baseline parameters below.

## Firmware Behavior

Active project:

```text
D:\steven\work\er2024\firmware\bringup\keil\bringup_mspm0g3507_nortos_keil.uvprojx
```

The Step 6 firmware keeps the confirmed MCU pin map and replaces the old
open-loop forward/reverse test sequence with serial-controlled line following.
Boot mode is `STBY`; motors do not start until UART command `g` or jog command
`f`/`v`.

2026-07-01 physical orientation correction:

| Item | Correct current fact |
| --- | --- |
| D153C channel A | `PWMA` / `AO1-AO2` controls the physical right wheel |
| D153C channel B | `PWMB` / `BO1-BO2` controls the physical left wheel |
| Forward direction | Opposite of the earlier Step 5 check |
| Default motor flags | `mSwap=1 invA=1 invB=1` |
| Default sensor/encoder flags | `sensorFlip=1 eSwap=1 eInvA=1 eInvB=1` |

Default control constants:

| Parameter | Value | Meaning |
| --- | ---: | --- |
| Control period | `20 ms` | 50 Hz first floor-test loop |
| Base speed | `180 / 1000` | Raise with `+` if the car cannot move |
| `Kp` | `55` | Scaled by `100` |
| `Ki` | `0` | Disabled for first line test |
| `Kd` | `35` | Scaled by `100` |
| Steering limit | `240 / 1000` | Correction is clamped before motor output |

Accepted Step 6 baseline after the 2026-07-01 floor test:

```text
base=180 Kp=55 Ki=0 Kd=35
orient mSwap=1 invA=1 invB=1 sensorFlip=1 eSwap=1 eInvA=1 eInvB=1
```

Normal telemetry:

```text
mode=RUN line=0b00011000 st=OK n=2 err=0 corr=0 base=180 L=180 R=180 encL=123 encR=119
```

Telemetry fields:

| Field | Meaning |
| --- | --- |
| `mode` | `STBY`, `RUN`, or `BRAKE` |
| `line` | Raw muxed Yahboom byte, black/dark=`1`, white/bright=`0` |
| `st` | `OK`, `LOST` for `0x00`, or `FULL` for `0xFF` |
| `n` | Number of active black channels |
| `err` | Weighted line error; positive means line is to the robot's right |
| `corr` | PD steering correction |
| `L/R` | Logical left/right PWM commands after orientation correction |
| `encL/encR` | Logical left/right, forward-positive encoder totals |

## Sensor Direction Contract

Current board note before orientation correction:

```text
bit0 = X1 = old physical right side
bit7 = X8 = old physical left side
```

Because the car had been checked backwards, firmware default `sensorFlip=1`
reverses this sign for the current logical car frame.

Therefore:

| Manual sensor test | Expected telemetry |
| --- | --- |
| Black tape under center channels | `err` near `0`, `L` near `R` |
| Black tape under right side | `err` positive, `corr` positive, `L > R` |
| Black tape under left side | `err` negative, `corr` negative, `L < R` |

If those left/right signs are reversed after the sensor is mounted, do not swap
motor wires. Toggle `s` over UART for the current run, then change
`STEP6_REVERSE_SENSOR_LR_DEFAULT` if that setting should become permanent.

## First Test Sequence

1. Put the car on a stand with wheels lifted.
2. Open the onboard CH340 serial port at `9600-8N1`.
3. Confirm boot banner says `Step6` and `mode=STBY`.
4. Move black tape under the sensor by hand and verify `line`, `st`, `err`,
   `corr`, `L`, and `R` match the direction table above.
5. Send `f` with wheels still lifted. Confirm the car's logical left and right
   wheels both spin in the current forward direction. Send `x` to stop.
6. Confirm `encL` and `encR` both increase during the `f` test. If a sign is
   wrong, use `3` or `4`; if sides are swapped, use `e`.
7. Send `g` with wheels still lifted. Confirm both wheels spin forward when
   centered over a normal black line pattern such as `0b00011000`.
8. Move tape to the right side; left wheel command should rise above right.
9. Move tape to the left side; right wheel command should rise above left.
10. Send `x` to coast stop, then put the car on the floor.
11. Start on a short straight black tape segment and send `g`.
12. If it stalls, press `+` once or twice. If it oscillates, press `P` or `D`
    to reduce steering response.

## Safety Notes

- Keep USB for MCU/debug power and keep the motor rail current-limited if
  possible during first floor tests.
- Keep gimbal, spotlight, laser, and unused drive motors disconnected.
- `st=LOST` (`0x00`) and `st=FULL` (`0xFF`) command TB6612 short brake. These
  states are not yet the dashed-line or key-point logic; that belongs in later
  steps.
- Watch D153C/TB6612 and motor temperature during repeated braking.

## 2026-07-01 Validation Log Summary

Source log:

```text
C:\Users\steven\.codex\attachments\d2a45bc5-76df-4d0b-8ef1-77dabdb9a2b2\pasted-text.txt
```

Observed serial port in this run: `COM11` at `9600-8N1`. Earlier notes saw the
same onboard CH340 interface as `COM9`; use whichever Windows assigns, but do
not use the J-Link virtual serial port.

Useful command sequence from the log:

```text
f -> [FWD] jog forward; send x to stop
x -> [STBY] motors coast
? -> printed base/K/orientation values
f -> g -> [RUN] line PID armed
```

Log statistics extracted from 2306 telemetry rows:

| Item | Result |
| --- | ---: |
| `STBY` rows | `1237` |
| `FWD` rows | `276` |
| `RUN` rows | `793` |
| `OK` sensor rows | `1752` |
| `LOST` rows | `535` |
| `FULL` rows | `19` |
| Base speed | `180` for all rows |
| `FWD` encoder delta | `encL +4182`, `encR +3553` |
| `RUN` encoder delta | `encL +14429`, `encR +9069` |
| `RUN` error range | `-300 .. 350` |
| `RUN` correction range | `-182 .. 192` |

Representative closed-loop evidence:

```text
mode=RUN line=0b00011100 st=OK n=3 err=-50 corr=-27 base=180 L=153 R=207
mode=RUN line=0b11000001 st=OK n=3 err=83 corr=45 base=180 L=225 R=135
mode=RUN line=0b11011011 st=OK n=6 err=0 corr=0 base=180 L=180 R=180
mode=RUN line=0b11111111 st=FULL n=8 err=0 corr=0 base=180 L=0 R=0
```

Interpretation:

- Motor orientation is correct: user confirmed `f` drives forward and `x`
  stops.
- Encoder logical signs are usable for Step 7: both `encL` and `encR`
  increased during forward jog and line-following runs.
- Sensor left/right sign is correct for control: negative `err` reduces left
  PWM and raises right PWM (`L < R`), while positive `err` raises left PWM and
  lowers right PWM (`L > R`).
- `FULL` (`0xFF`) still brakes the car as intended. Avoid testing with a large
  all-black surface; use tape/track width that activates only part of the array.
- The visible binary byte prints bit 7 on the left and bit 0 on the right, so
  a rightmost printed `1` does not by itself mean physical right. Trust `err`
  and `L/R` for the control-side interpretation.

Step 6 exit criteria met:

- `f` jog forward works with the current front direction.
- `x` stops the car.
- `g` starts line following.
- The car was placed on the floor and followed the line successfully.
- No code change is needed before starting Step 7.

Next Step 7 baseline:

- Use direct `g` from `STBY` when starting on the line; `f` is only a jog test.
- Add a one-lap exit condition using encoder distance first, then refine with
  line-feature/key-point detection.
- Keep `base=180` as the first Step 7 speed unless the track requires a small
  increase after repeatable lap stopping.

## Build Verification

```text
D:\Keil_v5\UV4\uVision.com -r D:\steven\work\er2024\firmware\bringup\keil\bringup_mspm0g3507_nortos_keil.uvprojx -o D:\steven\work\er2024\firmware\bringup\keil\build.log
0 Error(s), 0 Warning(s)
```

Generated hex:

```text
D:\steven\work\er2024\firmware\bringup\keil\Objects\bringup_mspm0g3507_nortos_keil.hex
```
