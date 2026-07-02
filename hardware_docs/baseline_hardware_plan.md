# Baseline Hardware Plan

> Planning note: the 2026-06-21 actual purchase differs from this original baseline. It includes a 4WD chassis, four 12 V 520 encoder motors, a TB6612 module, and a separate MS42CG/D36A stepper path. See `purchased_hardware_registry.md` and `firmware_integration_matrix.md`; choose one drive architecture before firmware pin assignment.

## Design Goal

Make the first build reliable enough to pass the basic requirements, then add advanced functions without changing the whole architecture.

## Recommended Architecture

| Module | Baseline Choice | Reason |
| --- | --- | --- |
| Main controller | TI MSPM0G3507/TM4C/C2000-class MCU board | Satisfies TI-chip requirement and supports ADC/PWM/UART |
| Chassis | Differential-drive wheeled chassis | Simple control, legal, easy to tune |
| Motor feedback | Quadrature encoders | Needed for key-point timing and repeatability |
| Line sensor | 8 to 16 channel IR/grayscale array | Legal non-camera path recognition |
| Motor driver | Dual H-bridge driver | Simple PWM direction control |
| Target actuator | Two-axis servo gimbal | Fastest path to pointing and drawing |
| End effector | Laser pointer first, pen/marker second | Laser is easy for pointing; pen adds drawing capability |
| Power | Separate rails for logic, motors, servos | Reduces brownout and noise |
| UI/debug | Buttons, LEDs, buzzer, UART | Required for modes, prompts, and tuning |

## Key Design Decisions To Confirm

- Which TI MCU board is actually available.
- Whether the motors already include encoders.
- Whether the target operation is judged by visible pointing, laser dot, pen mark, or another action.
- Whether dynamic following needs physical marking or only aiming.
- Whether field black line includes dashed sections in the final test.

## Initial I/O Budget

| Signal Type | Estimated Count |
| --- | ---: |
| Line sensor analog/digital inputs | 8-16 |
| Motor PWM outputs | 2 |
| Motor direction outputs | 4 |
| Encoder inputs | 4 |
| Servo PWM outputs | 2-3 |
| Buzzer/LED outputs | 2-4 |
| Buttons | 2-4 |
| UART debug | 1 |
| Optional IMU I2C/SPI | 1 bus |

## Control Strategy

- Line tracking:
  - Sensor weighted average for lateral error.
  - PID steering with speed scheduling.
  - Special handling for dashed line and arc transitions.

- Key-point detection:
  - Combine track geometry, encoder distance, and line feature changes.
  - Use sound/light prompts at A/B/C/D.

- Target pointing:
  - Static stage first: calibrate car pose to target and map to gimbal angles.
  - Dynamic stage later: compute expected target angle from estimated car position on known track.

- Drawing:
  - Use gimbal calibration to map target-plane coordinates to servo angles.
  - Start with simple shapes: cross, square, circle approximation, or one letter.
