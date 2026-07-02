# Step 5 Motor And Encoder Bring-Up Record

Date: 2026-06-30

Status: motor wiring and encoder activity were bench-verified with wheels
lifted. The left-encoder sign and active-brake firmware changes are
build-verified and should be rechecked after reflashing. The active Step 5
firmware is the checked-in Keil project under `firmware/bringup`.

## 2026-07-01 Orientation Correction

The car had been checked while held in the opposite front direction. Current
physical orientation is:

| D153C channel | Current physical wheel |
| --- | --- |
| `PWMA` / `AO1-AO2` | right wheel |
| `PWMB` / `BO1-BO2` | left wheel |

The current forward direction is opposite the earlier Step 5 direction check.
Use the Step 6 firmware orientation flags as the current source of truth.

## Historical Step 5 Firmware Pin Map

Use this table for the current Step 5 firmware. Do not use the older
classmate PA2/PA14 or the earlier candidate PA21/PB20 PWM mapping for this
build.

| Function | Module pin | MSPM0G3507 board pin |
| --- | --- | --- |
| Left motor PWM | D153C `PWMA` | `PA8 / A08` |
| Right motor PWM | D153C `PWMB` | `PA9 / A09` |
| Left motor direction | D153C `AIN1` | `PA16 / A16` |
| Left motor direction | D153C `AIN2` | `PA21 / A21` |
| Right motor direction | D153C `BIN1` | `PA22 / A22` |
| Right motor direction | D153C `BIN2` | `PA23 / A23` |
| Motor standby | D153C `STBY` | `PB20 / B20` |
| Common ground | D153C `GND` | MSPM0 `GND` |
| Left encoder A/B | Left motor `A/B` | `PB17 / PB18` |
| Right encoder A/B | Right motor `A/B` | `PB24 / PB25` |
| Grayscale address | YB-MYX05 `AD0/AD1/AD2` | `PA26 / PA27 / PA24` |
| Grayscale output | YB-MYX05 `OUT` | `PA25` |

## Bench Wiring Used

- MSPM0 board was powered from USB-C during bring-up.
- D153C `5V/GND` was powered from the black power module 5 V rail.
- D153C `GND`, MSPM0 `GND`, encoder `G`, and sensor `GND` share common ground.
- Encoder `V` was connected to MSPM0 `3V3` first, so encoder A/B levels stay
  MCU-safe.
- Yahboom `YB-MYX05-V1.0` sensor was powered from 5 V; prior measurement showed
  `OUT` is about 3.3 V high and can connect directly to `PA25`.
- Left motor power wires go to D153C `AO1/AO2`; right motor power wires go to
  D153C `BO1/BO2`.
- A faulty or suspect D153C right channel was isolated by swapping the right
  wheel to `AO1/AO2`; replacing/reconnecting the D153C module restored right
  wheel operation.

## Observations

- Both wheels respond to the Step 5 sequence:
  `wait 2s -> FWD 2s -> STOP 1s -> REV 2s -> STOP 1s`.
- Right wheel direction was correct after the final wiring.
- In the historical Step 5 check, left wheel direction was physically reversed
  relative to that firmware's forward convention, so the old firmware reversed
  that side in software instead of requiring another motor-wire swap.
- UART output confirmed both encoders count while the wheels turn.
- Before encoder sign correction, forward motion made `encL` decrease while
  `encR` increased. The firmware now flips the left encoder sign so forward
  should make both printed counts increase.
- With open-loop equal PWM, the right wheel could coast about half a turn more
  than the left on stop. This is expected until speed PID is added, but the
  Step 5 stop state now uses TB6612 short brake to reduce coast.
- The line sensor printed `line=0b01111111` / similar during this motor test;
  sensor calibration and mounting remain a separate Step 6/7 concern.

## Firmware Changes From This Bring-Up

- The old left-side motor function was software-reversed to match the physical
  wiring known during the Step 5 check.
- `motorBrake()` was added for TB6612 active short brake:
  `AIN1=AIN2=BIN1=BIN2=1` and both PWM channels at full duty.
- The motor state machine now prints `[STOP] motors BRAKE` and uses active
  brake during both stop intervals.
- The left encoder ISR sign was inverted so the printed left/right encoder
  counts follow the same forward-positive convention.

Build verification:

```text
D:\Keil_v5\UV4\uVision.com -r D:\steven\work\er2024\firmware\bringup\keil\bringup_mspm0g3507_nortos_keil.uvprojx -o D:\steven\work\er2024\firmware\bringup\keil\build.log
0 Error(s), 0 Warning(s)
```

Generated hex:

```text
D:\steven\work\er2024\firmware\bringup\keil\Objects\bringup_mspm0g3507_nortos_keil.hex
```

## Next Step Notes

- Reflash the new hex and confirm `[FWD]` makes both `encL` and `encR`
  increase.
- Record encoder counts for exactly 10 wheel revolutions to derive counts per
  wheel revolution.
- Measure left/right encoder delta over one 2-second forward window; unequal
  deltas confirm why open-loop PWM cannot drive straight.
- Step 6 should add a low-rate speed PID using encoder deltas, with wheels
  lifted before floor testing.
- Watch D153C and motor temperature during repeated braking; reduce duty or
  brake time if the driver warms quickly.
