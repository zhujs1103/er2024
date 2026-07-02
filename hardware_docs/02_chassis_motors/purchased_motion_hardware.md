# Purchased Chassis And Motion Hardware

## `HW-20260621-05` — MS42CG Encoder Stepper

- 42-size stepper motor; thumbnail identifies `MS42CG` and a 5 mm output shaft.
- Selected option is the encoder version; exact suffix is truncated.
- Quantity 1; displayed price CNY 57.80.

Required before use: phase pairs, step angle, rated phase current, phase resistance/inductance, holding torque, encoder supply/output/CPR/index and connector pinout.

## `HW-20260621-06` — 4WD Chassis Kit

- Upgraded black fiberglass-board 4WD smart-car kit.
- Quantity 1; displayed price CNY 23.76.
- Product image appears to show yellow TT-style gear motors and wheels, but included contents and selected SKU are truncated.
- Wheel diameter confirmed by user on 2026-07-01: `65 mm`; wheel circumference is about `204.2 mm`.

Verify included motors, hole spacing and completed dimensions. The competition limit is `25 cm x 15 cm x 25 cm`.

## `HW-20260621-11` — 12 V 520 Encoder Gearmotors

- Title: `520编码器电机12V大扭矩PID源码`.
- Visible option starts `520单独编码器电机;320r/...`.
- Quantity 4; displayed price CNY 48.00 each shown.

Do not write odometry constants until these are measured/confirmed:

- Gear ratio and no-load output speed.
- Stall/start current at the intended battery voltage.
- Encoder supply, channel count, output type and logic voltage.
- Pulses/counts per motor revolution and per gearbox output revolution.
- Shaft size, mounting-hole pattern and compatibility with item 06.

Known odometry input so far:

| Parameter | Value |
| --- | ---: |
| Wheel diameter | `65 mm` |
| Wheel circumference | `204.2 mm` |
| Left encoder manual-forward measurement | `-3350 ticks / 10 rev = -335.0 ticks/rev` |
| Right encoder manual-forward measurement | `+3301 ticks / 10 rev = +330.1 ticks/rev` |
| Average absolute encoder ticks per wheel revolution | `332.6 ticks/rev` |
| Distance per encoder tick, using average absolute CPR | `0.614 mm/tick` |
| Encoder ticks per metre, using average absolute CPR | `1629 ticks/m` |

The left manual-forward hand test was negative, but the 2026-07-01 wheels-up
motor-driven `f` jog showed both logical encoders increasing:

```text
f jog: encL 0 -> 1384, encR 0 -> 1102, odom 0 -> 1243
```

Use the motor-driven `f` result as the source of truth for autonomous odometry:
the current encoder orientation is usable and does not need UART command `4`.

## Architecture Decision

The stepper path and four-DC-motor path are alternatives. Complete current, mass, mechanical-fit and encoder tests before selecting the primary drive architecture.
