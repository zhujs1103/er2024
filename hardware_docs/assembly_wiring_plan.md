# ER2024 Hardware Assembly Wiring Plan

Status: bench assembly plan for the purchased hardware.

This plan is written for the current purchased set: MSPM0G3507 core board, 12 V 520 encoder motors, TB6612FNG/D153C motor driver, Yahboom 8-channel grayscale sensor, LM2596 buck module, 2-axis PWM servo gimbal, 5 V spotlight, and 405 nm laser.

The first build target is a safe 2WD line-following chassis. Do not connect the laser or all four motors during the first bring-up.

Classmate reference reviewed: `classmate_wiring_reference_review.md` provides a useful first concrete candidate pin map for the 2WD baseline, but bench measurements now override its electrical assumptions. In particular, the received Yahboom sensor `OUT` is 3.3 V-safe and does not need the divider suggested by that reference.

## 1. Assembly Order

1. Mechanical frame only:
   - Mount the chassis plates, wheels, two 520 motors for left/right drive, and leave the other two motors unpowered.
   - Mount the grayscale sensor at the front, low enough for stable readings but high enough not to scrape the field.
   - Leave the gimbal, spotlight, and laser physically disconnected until the car can move and stop reliably.

2. MCU alone:
   - Power the MSPM0G3507 board from USB only.
   - Flash the existing bring-up firmware.
   - Confirm PB22 blinks and UART0 prints through the onboard CH340E.

3. Power tree with no motor load:
   - Verify battery polarity and open-circuit voltage with a meter.
   - Adjust the LM2596 output before attaching any electronics.
   - Build common ground first, then positive branches.

4. Grayscale sensor:
   - Power and measure sensor output high level.
   - Connect `OUT` directly to `PA25`; measured output is 3.3 V-safe.
   - Record left-to-right bit order and black/white active level.

5. One motor through TB6612:
   - Lift wheels off the table.
   - Wire only one TB6612 channel and one 520 motor.
   - Test low PWM, direction, and current.

6. Encoder for the same motor:
   - Verify encoder supply voltage and A/B output voltage.
   - Connect A/B to capture/interrupt-capable MCU inputs only after level safety is known.
   - Record counts per wheel revolution and forward sign.

7. Two-wheel baseline:
   - Add the second motor on the second TB6612 channel.
   - Keep this as 2WD until measured current proves a safer 4WD driver path.

8. Gimbal and light:
   - Use a separate 5-6 V high-current buck/BEC for the two servos.
   - Add the 5 V spotlight through a MOSFET/transistor as a safe stand-in for laser switching.

9. Laser last:
   - Connect only after chassis, gimbal, and emergency-off behavior are stable.
   - Use a MOSFET/load switch, physical disable switch, and beam stop.

## 2. Power Wiring

Recommended first bench power topology:

```text
12 V battery +
  |
  |-- fuse or current-limited bench supply
  |-- main switch
  |-- TB6612 VM / motor power
  |
  |-- LM2596 IN+
        LM2596 OUT+ -> sensor VCC, only after adjusted and measured
        LM2596 OUT- -> common ground

Later, add:

12 V battery + -> separate 5-6 V high-current servo buck/BEC -> servo V+
```

Common ground topology:

```text
12 V battery -
  |-- TB6612 GND
  |-- LM2596 IN-/OUT-
  |-- MSPM0G3507 GND
  |-- sensor GND
  |-- encoder GND
  |-- servo buck/BEC GND, later
  |-- MOSFET source/load-switch ground, later
```

Rules:

- Do not feed 12 V into the MSPM0 board, sensor, encoder, servo signal, or GPIO.
- Do not power motors, servos, spotlight, or laser from MCU pins.
- Treat all MSPM0 GPIO as 3.3 V logic.
- Do not parallel unknown lithium battery packs.
- Do not use the SS12D10 slide switch as the main battery switch unless its contact rating is verified.

## 3. Provisional Wiring Table

This table names the subsystem wiring boundary. The received Yahboom board is
the 6-pin muxed `YB-MYX05-V1.0` variant, so Step 4 uses `AD0/AD1/AD2 + OUT`
rather than eight direct output wires.

| Subsystem | Module pin/signal | Connect to | Notes |
| --- | --- | --- | --- |
| Debug | USB | PC | Use for first MCU power and flashing. |
| Debug | UART0 TX/RX | Onboard CH340E | Current firmware uses TX PA10, RX PA11. |
| Status | LED1 | Onboard PB22 | Already used by bring-up firmware. |
| Battery | Battery + | Fuse/switch input | Verify polarity before connection. |
| Battery | Battery - | Common ground | Make this the first shared reference. |
| Buck | LM2596 IN+ / IN- | Battery branch | Adjust output with no load. |
| Sensor | VCC | Measured buck output or documented sensor supply | Confirm sensor VCC and output high level. |
| Sensor | GND | Common ground | Must share ground with MCU. |
| Sensor | 5V/GND | Measured 5 V rail and common ground | Confirm supply and polarity first. |
| Sensor | AD0/AD1/AD2 | `PA26 / PA27 / PA24` outputs | 3.3 V address outputs; add buffer if switching is unstable. |
| Sensor | OUT | `PA25` input | Measured 3.3 V logic; direct connection, no divider. |
| TB6612 | VM | Battery motor branch | Through fuse/switch/current limit. |
| TB6612 | VCC | 3.3 V logic or documented logic supply | Confirm D153C board logic requirement. |
| TB6612 | GND | Common ground | Required for PWM/direction reference. |
| TB6612 | STBY | MCU GPIO output | Default low/off at reset. Add pull-down if possible. |
| TB6612 | AIN1/AIN2 | MCU GPIO outputs | Left motor direction in 2WD baseline. |
| TB6612 | PWMA | MCU timer PWM output | Left motor speed. Start at 10-15% duty. |
| TB6612 | A01/A02 | Left 520 motor power wires | Reverse wires or direction bits after testing. |
| TB6612 | BIN1/BIN2 | MCU GPIO outputs | Right motor direction in 2WD baseline. |
| TB6612 | PWMB | MCU timer PWM output | Right motor speed. |
| TB6612 | B01/B02 | Right 520 motor power wires | Test with wheels lifted. |
| Encoder L | VCC/GND | 3.3 V first if supported, otherwise level-shifted 5 V | Do not connect 5 V A/B directly to MCU. |
| Encoder L | A/B | 2 MCU interrupt/capture inputs | Record forward count sign. |
| Encoder R | VCC/GND | Same as left encoder | Verify both motors match. |
| Encoder R | A/B | 2 MCU interrupt/capture inputs | Record CPR at wheel. |
| Servo pan | V+ / GND | Separate 5-6 V high-current supply | Common ground with MCU. |
| Servo pan | PWM signal | MCU timer PWM output | Start at neutral, around 1500 us, then calibrate. |
| Servo tilt | V+ / GND | Separate 5-6 V high-current supply | Do not use MCU 5 V/3.3 V. |
| Servo tilt | PWM signal | MCU timer PWM output | Stop before mechanical hard limits. |
| Spotlight | + | 5 V lighting rail | Confirm polarity/current. |
| Spotlight | - | MOSFET/transistor switched low side | GPIO drives gate/base through resistor. |
| Laser | + / - | Dedicated actuator rail through MOSFET/load switch | Voltage and polarity unknown until measured. |
| Laser enable | MOSFET gate or load-switch enable | MCU GPIO with pull-down | Firmware default off. |

## 3A. First Candidate MSPM0 Pin Map

Use this as the initial 2WD wiring target because the classmate reference was
written for a Tianmengxing MSPM0G3507 + TB6612 + muxed 8-channel grayscale +
dual encoder motor build. The Step 4 firmware already uses the grayscale pins
listed here.

| Function | Module signal | Candidate MSPM0 pin |
| --- | --- | --- |
| Left motor PWM | TB6612 `PWMA` | `PA21 / TIMA0_C0` |
| Right motor PWM | TB6612 `PWMB` | `PB20 / TIMA0_C1` |
| Left motor direction | TB6612 `AIN1` | `PA23` |
| Left motor direction | TB6612 `AIN2` | `PA22` |
| Right motor direction | TB6612 `BIN1` | `PA14` |
| Right motor direction | TB6612 `BIN2` | `PA16` |
| Motor standby | TB6612 `STBY` | `PA02`, default low |
| Grayscale address | Sensor `AD0` | `PA26` |
| Grayscale address | Sensor `AD1` | `PA27` |
| Grayscale address | Sensor `AD2` | `PA24` |
| Grayscale output | Sensor `OUT` | `PA25`, direct; measured high level about 3.3 V |
| Left encoder A/B | Encoder `A/B` | `PB24 / PB25` |
| Right encoder A/B | Encoder `A/B` | `PB18 / PB17` |

Keep the already-confirmed board functions unchanged:

| Function | MSPM0 pin |
| --- | --- |
| Onboard LED | `PB22` |
| UART0 TX/RX | `PA10 / PA11` |

Important compatibility note: the classmate firmware assumes an 80 MHz clock, 20 kHz PWM with a 4000-tick period, and SysTick configured from 80 MHz. The current checked-in bring-up project only proves LED/UART and records a 32 MHz CPU clock, so rebuild SysConfig or adjust those constants before compiling the classmate `main.c`.

## 4. Bench Test Sequence

### 4.1 MCU

- USB only.
- Confirm blink/UART.
- All external outputs should be safe low/off before connecting modules.

### 4.2 Power

- Measure battery voltage.
- Measure LM2596 output twice: no load, then with a small dummy load if available.
- Label every rail: battery, logic/sensor, servo, actuator.

### 4.3 Sensor

- Power sensor alone.
- Recorded measurement: `OUT` is logic `0` on white/bright and logic `1` on black/dark with 5 V supply.
- Confirm `AD2 AD1 AD0 = 000` selects `CH1/X1` and `111` selects `CH8/X8`.
- Test black tape and white surface.
- Write down:
  - black active level;
  - leftmost mounted channel name;
  - stable mounting height;
  - all-white and all-black bit patterns.

### 4.4 One Motor

- Connect TB6612 logic only first.
- Keep STBY low, PWM 0.
- Connect VM and one motor.
- Set STBY high, command 10-15% PWM.
- Measure current and check heating.
- Record command direction as forward or reverse.

### 4.5 Encoder

- Spin wheel by hand and print counts.
- Drive slowly and compare encoder count direction with actual forward motion.
- Measure counts per wheel revolution after wheel installation.

### 4.6 Two Motors

- Repeat one-motor test for the other side.
- Lift wheels for first closed-loop test.
- Only then test on the floor at low duty.

### 4.7 Servo And Spotlight

- Power servos from independent 5-6 V supply.
- Signal ground must be common with MCU.
- Start both axes at neutral pulse.
- Add spotlight through MOSFET and test on/off before laser.

### 4.8 Laser

- Do not power until voltage, current, polarity, and driver presence are known.
- Use a matte beam stop and physical disable switch.
- Verify reset and fault states force laser off.

## 5. Measurements To Record

| Item | Record |
| --- | --- |
| Battery | Full voltage, loaded voltage sag, connector polarity. |
| LM2596 | Set voltage, load voltage, temperature after several minutes. |
| Sensor | VCC, output high voltage, active level, bit order, mounting height. |
| Motors | No-load current, start current, stall/block current if safely measured. |
| Encoders | Supply voltage, A/B high level, CPR, forward sign. |
| Chassis | Wheel diameter, track width, motor side orientation. |
| Servos | Neutral pulse, min/max safe pulse, direction, stall current estimate. |
| Spotlight | Voltage, current, polarity. |
| Laser | Voltage, current, polarity, duty/thermal behavior, emergency-off result. |

## 6. Recommended First Wiring Milestone

Stop after this milestone before adding the gimbal and laser:

```text
USB-powered MSPM0
+ common ground
+ adjusted LM2596
+ 8-channel grayscale sensor
+ TB6612 with one left motor and one right motor
+ left/right encoder A/B
```

At that point the car can support:

- sensor byte printing;
- two-motor low-speed PWM;
- encoder counting;
- first line-following PID tests.

That is the safest point to lock the final MCU pin map and update SysConfig.
