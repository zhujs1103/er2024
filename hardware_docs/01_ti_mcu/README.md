# TI MCU

Store the selected TI MCU datasheet, launch board user guide, schematic, SDK notes, and pin-assignment notes here.

Current board notes:

- `purchased_mspm0g3507_core_board.md` - purchased MSPM0G3507 board intake.
- `mspm0g3507_bringup_notes.md` - first successful Keil/J-Link/SWD bring-up wiring and settings.

Selection must satisfy:

- Main controller is a TI chip.
- Enough ADC or digital inputs for the line sensor.
- Enough PWM outputs for motors and servos.
- UART available for debug/configuration.
- Timer/capture support preferred for encoders.

Create `pinout_plan.md` after the board is chosen.
