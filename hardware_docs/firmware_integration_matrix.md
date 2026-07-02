# Firmware Integration Matrix For Purchased Hardware

Status: purchased hardware partially received; Yahboom sensor interface and `OUT` voltage verified on 2026-06-29.

## Architecture Gate

The purchase contains two competing motion paths:

1. `MS42CG` encoder stepper + `D36A` dual stepper driver.
2. Four 12 V 520 encoder DC motors + TB6612FNG dual-channel module + 4WD chassis.

Choose the vehicle drive path after mechanical fit and stall-current tests. Do not write one control layer that assumes both are installed.

The 4WD DC path currently looks closer to the purchased chassis, but it has an unresolved current problem: a dual TB6612 channel may have to drive a left/right motor pair in parallel. That is forbidden until the combined start/stall current is measured against the module limit.

## Provisional Firmware Contracts

| Module | Expected firmware interface | Must verify before pin assignment |
| --- | --- | --- |
| MSPM0G3507 core board | TI DriverLib/SysConfig project; GPIO, timers, capture, UART, ADC as allocated | Exact board pin map, clock/debug wiring and 3.3 V connector behavior |
| Yahboom 8-channel sensor | 3 GPIO address outputs plus 1 muxed `OUT` input sampled into one bit field | Verified `OUT`: white/bright=`0`, black/dark=`1`, 3.3 V-safe direct to `PA25`; still verify `X1..X8` mounted order and address stability |
| TB6612FNG dual driver | Typically 2 PWM, 4 direction, 1 standby output | D153C connector names, polarity, regulator topology and per-channel current |
| Four 520 encoders | Timer capture/GPIO interrupts; convert counts to wheel distance | Encoder supply, output type, channels per motor, CPR at motor/output shaft, gear ratio |
| D36A stepper driver | Unknown | Whether it accepts STEP/DIR or direct phase/PWM signals; pinout and current control |
| MS42CG encoder stepper | Step/phase command plus encoder capture | Step angle, phase current, wiring pairs, encoder voltage/type/CPR/index |
| Two 15 kg servos | 2 timer PWM channels | Supply voltage, pulse endpoints, neutral, direction, safe mechanical angles, stall current |
| 405 nm laser | `laser_set(bool)` via MOSFET/load switch | Voltage, current, polarity, internal driver and modulation support |
| 5 V spotlight LED | `illumination_set(bool)` via MOSFET/transistor | Current and polarity |
| SS12D10 switch | Static mode select or hardware isolation | Contact topology/rating and whether it is used for logic or power |
| 6 x 6 x 5 button | GPIO input with pull-up; 10-20 ms software debounce initial value | Four-pin pair topology and final active level |

## Required Safety Defaults

- Motor standby/enable, laser and spotlight outputs must remain off during reset and fault handling.
- Servo PWM must not start until the actuator rail is valid and a safe neutral is configured.
- Battery voltage must be treated as up to the fully charged pack voltage, not exactly 12.0 V.
- Never power motors, servos, laser or spotlight directly from MSPM0 GPIO.

## Data Needed For Control Algorithms

| Algorithm | Required measured constants |
| --- | --- |
| Line PID | Sensor bit order, sensor spacing, mounting height, black/white active level |
| DC motor speed PID | Encoder counts/output revolution, wheel diameter, gearbox ratio, control interval |
| Odometry | Left/right effective wheel diameters, track width, encoder sign and counts/metre |
| Stepper position loop | Steps/revolution, microstep/control mode, encoder CPR and zero/index behavior |
| Gimbal pointing | Servo pulse-to-angle maps, neutral pulses, hard limits, backlash, target geometry |

Keep these constants in board/configuration files, not scattered as literals through application code.

## Reference Firmware Intake

Prior MSPM0G3507 contest projects from `25E题底盘开源.zip` and `24H题第四问开源.zip` have been inspected and summarized in `../project_management/senior_code_reference.md`.

Use them for module structure, PID implementation, 200 Hz control-loop shape and field-tuning workflow. Do not treat their SysConfig pins, sensor ADC path, motor PWM order, wheel constants or track-distance thresholds as valid for this car until measured.
