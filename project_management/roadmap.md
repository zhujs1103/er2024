# Roadmap

## Current Status - 2026-07-01

- Step 1-3 passed: Keil project builds, J-Link SWD flashing reaches `Verify OK`, PB22 health blink works, and UART debug is available through the onboard CH340.
- Step 4 passed: Yahboom `YB-MYX05-V1.0` muxed grayscale sensor reads correctly; black/dark=`1`, white/bright=`0`, and `OUT` is 3.3 V-safe.
- Step 5 passed: two-wheel motor jog/stop works with current orientation. Current physical mapping is `PWMA/AO1-AO2` = right wheel and `PWMB/BO1-BO2` = left wheel; current forward is opposite the earlier Step 5 check.
- Step 6 passed: low-speed line following works on the floor with `base=180 Kp=55 Ki=0 Kd=35` and orientation flags `mSwap=1 invA=1 invB=1 sensorFlip=1 eSwap=1 eInvA=1 eInvB=1`.
- Current observed CH340 port in the latest validation log was `COM11` at `9600-8N1`; earlier it appeared as `COM9`. Use the Windows-assigned CH340 port, not the J-Link virtual serial port.
- Next bench/field step: Step 7 one-lap line following from A back to A, with an encoder-distance exit condition first and line-feature/key-point detection later.

## Phase 0 - Confirm Inputs

- Confirm final TI MCU board.
- Confirm available chassis/motors/encoders.
- Confirm target operation type: laser pointing, pen marking, or another action.
- Collect P0 datasheets into `hardware_docs/`.

Exit condition: pin and power budget can be drafted.

## Phase 1 - Chassis And Line Tracking

- Assemble differential-drive chassis.
- Mount line sensor array.
- Implement sensor calibration.
- Implement basic PID line tracking.
- Tune one-lap return-to-A behavior.
- Add buzzer/LED stop prompt.

Exit condition: A-start one-lap tracking and stop within `30 s`.

## Phase 2 - Key Points And Linked Operation

- Add encoder-based distance estimation.
- Detect or schedule B/C/D/A key points.
- Add prompts at each point.
- Stop at B, run work-module action, then continue.

Exit condition: A-B work-C-D-A loop within `40 s`.

## Phase 3 - Static Target Operation

- Mount two-axis gimbal or light actuator.
- Implement servo calibration and angle limits.
- Build static target pointing action.
- Add standalone actuator test mode.

Exit condition: specified static target pointing completes within `5 s`.

## Phase 4 - Advanced Functions

- Dynamic following using known track geometry and odometry.
- Synchronous drawing with target-plane mapping.
- Four-lap continuous run.

Exit condition: advanced demos are stable enough for video submission.

## Phase 5 - Submission Package

- Record required videos.
- Photograph wiring and module layout.
- Write design report.
- Include AI usage explanation.
- Collect final source code, schematics, and calibration parameters.
