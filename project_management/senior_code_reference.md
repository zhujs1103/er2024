# Senior Code Reference Notes

Source archives:

- `C:/Users/steven/Downloads/25E题底盘开源.zip`
- `C:/Users/steven/Downloads/24H题第四问开源.zip`

Local extracted copy for inspection:

- `tmp/senior_reference_code/25E/25E题底盘国一开源/`
- `tmp/senior_reference_code/24H/24H题第四问代码开源-CTY/`

These archives are prior MSPM0G3507 Keil/SysConfig projects. Treat them as design references, not drop-in source for the 2026 car, because the final chassis, pin map, sensor board, motor driver, wheel geometry and actuator requirements still need bench verification.

## Most Useful Reference

Use the 25E chassis project first. It contains a complete baseline for:

- MSPM0G3507 SysConfig/Keil project organization.
- 8-channel line sensor sampling and bit-field conversion.
- Encoder speed and distance estimation.
- Cascaded control: line-position to turn command, then motor speed PID.
- 200 Hz chassis control loop with slower buzzer/OLED service loops.
- Button-driven lap count/start behavior.

The 24H project appears to share the same chassis framework and is more useful later when adding target-operation or question-specific logic.

## Key Files To Revisit During Firmware Bring-Up

| Area | 25E reference file | What to borrow |
| --- | --- | --- |
| Main loop and task rates | `user/main.c` | `duty_200hz` updates key, line sensor, encoders, task logic and motor PID; `duty_100hz` stops motors when not started; `duty_10hz` services buzzer. |
| Global data model | `user/datatype.h` | One `Car_t` aggregate holding sensor, motor, task and actuator state pointers. This is simple enough for contest firmware. |
| Tunable constants | `user/headfile.h` | PID defaults and geometry constants should become our own config header after hardware measurement. |
| PID implementation | `Algorithm/pid.c`, `Algorithm/pid.h` | Position/delta PID with output and integral limits. Good candidate to reuse after cleanup. |
| Motor control | `FunctionalModule/MotorsCtrl.c` | Turn PID produces left/right differential speed; each side then uses encoder speed PID to produce PWM. |
| Encoder odometry | `FunctionalModule/Encoder.c` | Count-reset per tick, low-pass filtered velocity, integrated wheel distance, helpers for total and differential distance. |
| Line sensor | `FunctionalModule/GraySensor.c` | Sensor normalization, binary packing into 8 bits, weighted line-position estimate. |
| Key point task logic | `ApplicationLayer/Task.c` | Detect right/left-angle events from sensor pattern, use encoder distance to protect turns and count laps. |
| UI/debug | `FunctionalModule/Oled.c`, `FunctionalModule/Buzzer.c`, `FunctionalModule/Key.c` | Minimal onboard feedback and button workflow. Useful for field tuning. |

## Parameter Values Worth Starting From

Reference values from `25E/user/headfile.h`:

| Parameter | Reference value | Notes for our car |
| --- | ---: | --- |
| Chassis control frequency | `200 Hz` | Good initial target if ADC, encoder ISR and PWM updates are stable. |
| Encoder lines | `260` | Must replace with measured encoder CPR and gear/wheel output relationship. |
| Tire radius | `4.0` | Unit is project-local; measure our actual wheel and normalize units before using distance thresholds. |
| Motor speed PID | `Kp=10.0`, `Ki=0.8`, `Kd=5.0` | Treat as only a rough tuning seed. |
| Turn PID | `Kp=0.8`, `Ki=0.0`, `Kd=0.15` | Depends strongly on sensor spacing, speed and chassis width. |
| Straight length | `64.0` | Replace with 2026 track geometry and measured encoder scale. |
| Turn guard distance | `30.0` | Replace after field tests; it is tied to their track and wheel units. |

## Suggested Migration Order

1. Create our own MSPM0G3507 SysConfig/Keil baseline with only clock, UART, one PWM channel and one GPIO output.
2. Bring up motor PWM and direction pins with wheels off the ground.
3. Add encoder interrupts and verify count sign, counts per wheel revolution and velocity at `200 Hz`.
4. Port the PID module and implement a two-side speed loop before attempting line tracking.
5. Add the 8-channel line sensor readout. If the purchased Yahboom board outputs digital signals, use direct GPIO bit sampling instead of the 25E analog multiplexer/ADC path.
6. Convert the 25E weighted line-position method into our `line_error` function.
7. Add the `Task.c` style state machine for A/B/C/D events only after one-lap line tracking is repeatable.
8. Add buzzer/OLED/UART debug from the beginning so calibration values can be checked on the car.

## Do Not Copy Blindly

- The 25E gray sensor code assumes an analog multiplexer and per-channel ADC calibration. Our purchased sensor note currently expects 8 digital GPIO inputs.
- The motor code assumes four motors and maps PWM order in `PWM_Output(pwm[0], pwm[2], pwm[3], pwm[1])`; this must match the actual driver wiring before use.
- Encoder distance helpers average only selected wheels in places. Verify whether our chassis has two or four encoder channels and define left/right odometry explicitly.
- The `headfile.h` constants mix physical and project-local units. Keep our constants in a board/configuration file after measurement.
- The reference projects include large TI SDK/CMSIS/DriverLib copies. Prefer the installed TI SDK/SysConfig source for our new project, and copy only small contest-specific modules after review.
