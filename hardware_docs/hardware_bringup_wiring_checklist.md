# Hardware Bring-Up And Wiring Checklist

Status: first bench-wiring guide for the purchased hardware.

This checklist is intentionally conservative. The goal is to avoid damaging the MSPM0G3507 board, motor driver, battery pack, servos, and laser while collecting the constants needed for firmware.

## Golden Rules

1. Do not connect every module at once. Bring up one subsystem at a time.
2. Keep one common ground: battery negative, motor driver GND, regulator GND, sensor GND, servo supply GND, and MSPM0 GND must meet.
3. Never feed 12 V into the MSPM0 board, sensor VCC, encoder VCC, servo signal, or any GPIO.
4. MSPM0 GPIO should be treated as 3.3 V only. If any module outputs 5 V logic, add a level shifter or resistor divider before the MCU pin.
5. Do not power motors, servos, spotlight, or laser directly from MCU pins.
6. Do not power the 405 nm laser during chassis bring-up. Treat it as the last actuator.
7. Do not parallel unknown lithium packs. Use one pack at a time until polarity, charger, BMS, and current capability are verified.
8. Do not drive four 520 motors from one dual TB6612 module until single-motor current and two-motor branch current are measured.

## Tools Required Before Wiring

- Multimeter.
- Current-limited bench supply if available; otherwise use battery only after polarity checks.
- USB cable for MSPM0 debug/programming.
- Small labels or tape for every connector.
- Heat-shrink or tape for exposed battery/motor joints.
- Optional but strongly recommended: inline fuse or resettable fuse on the battery positive line.

## Step 0 — Physical Intake

Before plugging anything in:

| Item | Check |
| --- | --- |
| MSPM0G3507 core board | Photograph both sides and save the seller pin map/schematic if available. |
| Battery packs | Measure open-circuit voltage, confirm red is positive and black is negative, identify charger connector. |
| LM2596 board | Identify input side and all output rails; do not trust printed labels until measured. |
| TB6612 D153C board | Identify `VM/Motor power`, `VCC/logic power`, `GND`, `STBY`, `PWMA/PWMB`, direction pins and motor outputs. |
| 520 motors | Separate the two motor power wires from encoder power/A/B wires. |
| Yahboom 8-channel sensor | Identify VCC, GND, and channel order from left to right. |
| 2-axis gimbal | Identify servo power positive, servo ground, and PWM signal on each servo lead. |

## Step 1 — MCU Alone

Use only USB power first.

1. Connect MSPM0G3507 board to PC by USB.
2. Confirm the board powers normally and can be programmed/debugged.
3. Run a minimal firmware test:
   - blink onboard LED if available;
   - print UART debug if already wired by the board;
   - keep all external output pins configured as safe low/off.

Do not connect battery, motors, servos, laser, or sensor yet.

## Step 2 — Regulator Test With No Load

Use the LM2596 before connecting electronics.

1. Connect battery positive to LM2596 `IN+`, battery negative to `IN-`.
2. Measure battery voltage. A nominal 12 V lithium pack may be around 12.6 V when full.
3. Adjust one output rail with no load:
   - for logic/sensor experiments: 5.0 V only if the sensor output is made 3.3 V-safe;
   - for MSPM0 logic: prefer the board's documented USB/5V/VIN input, not random 3.3 V back-feeding.
4. Power off, wait, then power on again and confirm the voltage is still correct.

Do not use this LM2596 as the proven supply for two 15 kg servos until it passes a real load and temperature test. A separate high-current 5-6 V BEC/buck is safer for servos.

## Step 3 — Common Ground Backbone

Build the ground first, with power still off.

Recommended first wiring backbone:

```text
Battery -  -------- TB6612 GND
             |----- LM2596 IN-
             |----- MSPM0 GND
             |----- Sensor GND
             |----- Servo supply GND, later
```

The positive branches should stay separate:

```text
Battery +  -> motor driver VM / motor power
Battery +  -> LM2596 IN+
LM2596 5V -> sensor VCC only after logic-level check
Separate 5-6V high-current buck -> servos later
```

## Step 4 — Grayscale Sensor First

This is the lowest-risk useful module.

1. Power the sensor from the documented VCC only. The received board is marked
   `YB-MYX05-V1.0` and uses a 6-pin mux header: `5V AD2 AD1 AD0 OUT GND`.
2. Recorded measurement: with 5 V sensor supply, `OUT` is logic `0` on a
   white/bright surface and logic `1` on a black/dark surface. Connect `OUT`
   directly to `PA25`; no divider is needed.
3. Connect the mux pins for the current Step 4 firmware:
   - `AD0 -> PA26`;
   - `AD1 -> PA27`;
   - `AD2 -> PA24`;
   - `OUT -> PA25` direct.
4. Write down physical order as `X1 ... X8` after mounting. In the received
   top-side photo, `X8` is on the left and `X1` is on the right.
5. Test black tape and white floor:
   - recorded polarity: black/dark gives `1`, white/bright gives `0`;
   - record sensor height where readings are stable.

Firmware target: scan all 8 muxed channels into one byte and print it continuously before any motor test.

## Step 5 — One 520 Motor + TB6612 Only

Start with one motor, wheels lifted off the table.

Typical TB6612 signal set:

| TB6612 signal | Connect to |
| --- | --- |
| `VM` / motor power | Battery positive through switch/fuse |
| `VCC` / logic power | 3.3 V or documented logic supply |
| `GND` | Common ground |
| `STBY` | MSPM0 GPIO, default low/off |
| `AIN1`, `AIN2` | MSPM0 GPIO direction pins |
| `PWMA` | MSPM0 timer PWM pin |
| `A01`, `A02` | One 520 motor's two motor-power wires |

Bring-up order:

1. Power MCU by USB.
2. Keep `STBY = 0`, PWM = 0.
3. Connect TB6612 logic VCC/GND only; confirm no heat.
4. Connect motor supply and one motor, with wheel off the table.
5. Set `STBY = 1`.
6. Try low PWM, about 10-15%.
7. Reverse direction and record which command is forward.
8. Measure motor current at low, medium, and blocked/stall conditions if possible.

Stop if the TB6612 board, wires, or battery connector heats up quickly.

## Step 6 — Encoder Test For The Same Motor

Most 520 encoder motors have motor power plus encoder VCC/GND/A/B. Verify by seller pinout or meter before connecting.

1. Power encoder from 3.3 V first if it works reliably.
2. If encoder requires 5 V, confirm A/B output voltage; use level shifting if high is 5 V.
3. Connect encoder A/B to MSPM0 interrupt or timer-capture capable GPIOs.
4. Spin the wheel by hand and print counts.
5. Drive slowly and verify:
   - forward count sign;
   - counts per output shaft revolution;
   - counts per wheel revolution after wheel is installed.

Firmware target: closed-loop speed control for one motor before adding the second motor.

## Step 7 — Two-Wheel Baseline

Recommended first vehicle baseline is 2WD:

| Side | Driver channel | Motor count |
| --- | --- | --- |
| Left | TB6612 A | 1 x 520 encoder motor |
| Right | TB6612 B | 1 x 520 encoder motor |

Keep the remaining two 520 motors as spares or mechanical experiments until current capacity is proven.

Only after single-motor current is known may you evaluate 4WD. If using 4WD, prefer a second suitable motor driver or a higher-current driver rather than simply paralleling left-pair and right-pair motors on one TB6612.

## Step 8 — Servo Gimbal

Do this after chassis motors are controllable.

1. Use a separate 5-6 V high-current supply/BEC for servos.
2. Connect servo supply GND to MSPM0 GND.
3. Connect only the servo signal wire to MSPM0 PWM output.
4. Start at neutral pulse, typically around 1500 us.
5. Slowly sweep within safe limits, for example 1000-2000 us, but stop earlier if the bracket hits a mechanical end.
6. Record each servo's neutral, minimum safe pulse, maximum safe pulse, and direction.

Do not power servos from MSPM0 3.3 V, USB 5 V, or an untested LM2596 rail.

## Step 9 — Spotlight LED

The 5 V white spotlight is a safer stand-in for the laser.

1. Identify polarity and current.
2. Drive it through a transistor/MOSFET, not from GPIO directly.
3. Firmware default must be off.
4. Use it to test gimbal pointing and enable/disable logic.

## Step 10 — 405 nm Laser Last

Only connect after the car and gimbal are stable.

1. Confirm voltage, current, polarity, and whether a driver is built in.
2. Use a MOSFET/load switch controlled by `laser_enable`; never drive directly from MCU.
3. Add a physical switch or removable connector in series for safety.
4. Keep firmware default off during reset, fault, line loss, and mode changes.
5. Use a beam stop and avoid eye exposure/reflections.

## First Firmware Milestones

1. MCU LED/UART alive.
2. 8-channel sensor byte printout stable on black/white.
3. TB6612 one motor low-speed forward/reverse.
4. One encoder count direction and CPR measured.
5. Two motors closed-loop speed control with wheels lifted.
6. Chassis straight-line test on floor.
7. Line-following PID at low speed.
8. Servo neutral/sweep test.
9. Spotlight on/off through MOSFET.
10. Laser enable last.

## Values To Record During Bring-Up

| Constant | Where firmware will need it |
| --- | --- |
| Sensor active level and bit order | Line position calculation |
| Sensor spacing and mounting height | PID tuning and line-loss handling |
| Encoder counts per wheel revolution | Speed PID and odometry |
| Wheel diameter | Distance and speed conversion |
| Track width | Turning and odometry |
| Motor PWM sign and minimum moving duty | Motor driver abstraction |
| Battery voltage sag under motor load | Low-voltage protection |
| Servo neutral/min/max pulse | Gimbal control |
| Laser voltage/current/polarity | Actuator driver safety |
