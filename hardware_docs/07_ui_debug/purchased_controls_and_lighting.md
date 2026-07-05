# Purchased Controls And Lighting

## `HW-20260621-15` — SS12D10 Slide Switches

- Three-pin, two-position slide switch.
- Selected pack: 10 pieces; displayed price CNY 9.59.
- Use as a logic/mode input unless its manufacturer contact rating is verified for the intended power current.

For GPIO use, connect the centre/common contact deliberately, enable a pull-up/pull-down, and confirm the two outer positions with a continuity meter.

## `HW-20260621-16` — 6 x 6 x 5 mm Tactile Buttons

- Four-pin vertical tactile switch; one cart order at CNY 2.16.
- Product image says one order equals 50 pieces; verify received count.

The two pins on each side are normally common. Verify with a meter, use a pull-up and start firmware debounce around 10-20 ms before tuning from measurements.

## `HW-20260621-17` — 5 V White Spotlight LED

- Selected option: `DC5V小号白光（15MM直径）`.
- Quantity 1; displayed price CNY 5.40.
- Treat as a two-wire DC load, not an addressable LED.

Measure current and polarity, then switch it with a transistor/MOSFET or load switch. Do not power it directly from an MCU GPIO.

## External Buzzer And Indicator LED

Added test firmware: `firmware/ui_buzzer_led_test`.

- Active buzzer module: `GND / I/O / VCC`, marked low-level trigger.
- Test supply: `VCC -> 3.3 V`, `GND -> common GND`, `I/O -> PA2`.
- Firmware polarity: `PA2 high = silent`, `PA2 low = beep`.
- External discrete LED test: `PA14 -> 1 kOhm resistor -> LED anode`, LED cathode -> GND.
- `PB22` onboard LED flashes with the external LED as a known-good reference.

The same PA2/PA14 mapping is now integrated into `firmware/abcd_fresh` as
non-blocking event feedback:

- boot complete: one short beep/flash;
- accepted `A` auto start: one short beep/flash;
- DONE: one longer beep/flash, then PA14 fast-blinks;
- FAIL or `A` rejected because IMU is not ready: three short beeps/flashes,
  then PA14 fast-blinks.

Compatibility note: older classmate-reference notes used `PA14` as a motor
direction candidate. The current `abcd_fresh` motor direction pins are
`PA16 / PA21 / PA22 / PA23`, so `PA14` is available for this external LED in
the current main firmware.

## B21 Button Start Experiment

Current test build: `2026-07-05-abcd-fresh-b21-auto-start-test`.

- Board button/pin under test: `B21 / PB21`.
- Firmware input mapping: `GPIOB.21`, `IOMUX_PINCM49`.
- Electrical assumption: internal pull-up, active-low press, matching TI
  LP-MSPM0G3507 `USER_SWITCH_1` examples.
- Debounce: 30 ms software polling in the main loop.
- Current behavior: press B21 in standby/brake to start a 3 second countdown,
  then run MPU6050 still-car calibration while motors stay stopped. If
  calibration succeeds, the verified A auto mode starts after the two-short OK
  feedback.
- Pressing B21 again during the countdown cancels it.
- Pressing B21 after calibration OK but before A starts cancels the pending
  auto start.
- Pressing B21 while a moving mode is active brakes only and does not start
  calibration.

Do not use the `BSL` button for the run-start workflow until the delivered board
schematic confirms its boot/reset behavior.
