# Hardware Documents

Store datasheets, user manuals, wiring diagrams, calibration notes, and vendor links here.

## Folder Rules

- Keep one hardware category per folder.
- Put official PDFs or manuals in the relevant folder.
- Add a short `README.md` or note when a module is chosen, replaced, or rejected.
- Preserve original downloaded filenames when useful, but add a short local note with the exact module name used on the car.

## Inventory And Evidence

- `purchased_hardware_registry.md` is the source of truth for hardware reported as purchased.
- A shopping title or screenshot is not a datasheet. Keep seller-confirmed, image-inferred, and bench-verified values separate.
- Do not assign MCU pins or power rails from an unverified product variant.
- Category notes should contain the electrical interface and the checks required before firmware integration.

## Categories

- `01_ti_mcu/` - TI main controller, LaunchPad/user guide, SDK notes.
- `02_chassis_motors/` - chassis, motor, wheel, encoder, mechanical dimensions.
- `03_line_sensors/` - grayscale/IR line sensors and calibration notes.
- `04_motor_driver/` - motor driver datasheets and wiring.
- `05_servo_gimbal_actuator/` - servos, gimbal, marker, laser pointer, pen-lift mechanism.
- `06_power_safety/` - batteries, regulators, fuses, switches, emergency stop.
- `07_ui_debug/` - buttons, buzzer, LEDs, OLED, UART/Bluetooth/debug adapters.

## Purchased Hardware Notes

- `purchased_hardware_registry.md` - complete 17-line screenshot-derived inventory.
- `official_reference_links.md` - vendor/official MCU, driver, regulator and sensor references.
- `firmware_integration_matrix.md` - provisional software interfaces and decision gates.
- `hardware_bringup_wiring_checklist.md` - step-by-step first wiring and bench bring-up sequence.
- `step5_motor_encoder_bringup_record.md` - 2026-06-30 bench-verified D153C motor/encoder wiring, observations, and firmware sign/brake decisions.
- `step6_pid_line_following_bringup_record.md` - 2026-07-01 accepted low-speed line PID baseline and orientation flags.
- `step7_one_lap_encoder_stop_bringup_record.md` - Step 7 one-lap encoder stop firmware behavior and field-test sequence.
- `01_ti_mcu/purchased_mspm0g3507_core_board.md` - purchased third-party MSPM0G3507 board intake.
- `01_ti_mcu/mspm0g3507_bringup_notes.md` - verified Keil/J-Link/SWD bring-up wiring, settings and serial-port notes.
- `02_chassis_motors/purchased_motion_hardware.md` - MS42CG, 4WD chassis and four 520 encoder motors.
- `03_line_sensors/purchased_yahboom_8_channel_sensor.md` - white-light 8-channel muxed line sensor.
- `04_motor_driver/purchased_motor_drivers.md` - D153C TB6612 and D36A stepper modules.
- `05_servo_gimbal_actuator/405nm_adjustable_laser_module.md` - 405 nm adjustable-focus dot laser module visible in the 2026-06-21 cart-share screenshot.
- `05_servo_gimbal_actuator/purchased_2d_servo_gimbal.md` - two-axis 15 kg-class servo gimbal intake.
- `06_power_safety/red_black_parallel_wire.md` - two-core red/black copper parallel wire visible in the same screenshot.
- `06_power_safety/purchased_power_components.md` - duplicate battery lines, LM2596 board and XT30 leads.
- `07_ui_debug/purchased_controls_and_lighting.md` - slide switches, buttons and 5 V spotlight.

## Baseline Candidate Set

The current recommended direction is conservative:

- TI MCU: MSPM0G3507-class LaunchPad or another TI MCU board with enough ADC/PWM/UART.
- Drive: differential two-wheel drive with encoders.
- Line detection: 8-channel or more IR/grayscale sensor array, non-camera.
- Motor driver: dual H-bridge driver sized for the selected DC motors.
- Attitude/odometry: wheel encoders first, IMU as drift/stability assist.
- Actuator: two-axis servo gimbal with laser pointer or marker/pen end effector.
- Power: separate motor/actuator logic rails, common ground by design, emergency stop.

Final part numbers should be chosen after checking what is already available in the lab and what can be purchased quickly.
