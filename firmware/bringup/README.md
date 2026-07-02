# ER2024 MSPM0G3507 Bringup

Keil MDK bringup project for the JLC Tianmengxing MSPM0G3507 development board. This is a custom third-party board, not the TI LP-MSPM0G3507 LaunchPad.

## Hardware

| Signal | MCU pin | Notes |
| --- | --- | --- |
| LED1 | PB22 | Active high, output, internal pull-down |
| UART0 TX | PA10 | Connected to onboard CH340E |
| UART0 RX | PA11 | Connected to onboard CH340E |
| Line sensor AD0 | PA26 | Mux address output |
| Line sensor AD1 | PA27 | Mux address output |
| Line sensor AD2 | PA24 | Mux address output |
| Line sensor OUT | PA25 | Mux output input; measured 3.3 V logic, direct connection |
| Motor PWM HW-A | PA8 | TIMA0 CCP0; D153C `PWMA`, `AO1/AO2`, physical right wheel |
| Motor PWM HW-B | PA9 | TIMA0 CCP1; D153C `PWMB`, `BO1/BO2`, physical left wheel |
| Motor direction AIN1/AIN2 | PA16 / PA21 | D153C channel A direction |
| Motor direction BIN1/BIN2 | PA22 / PA23 | D153C channel B direction |
| Motor STBY | PB20 | Active high |
| Encoder HW-A A/B | PB17 / PB18 | PB17 rising-edge interrupt, PB18 direction phase |
| Encoder HW-B A/B | PB24 / PB25 | PB24 rising-edge interrupt, PB25 direction phase |

UART settings: 9600 baud, 8 data bits, no parity, 1 stop bit, no flow control.

## Tools

Expected local tool paths:

- Keil MDK: `D:/Keil_v5/UV4/UV4.exe`
- Arm Compiler 6: `D:/Keil_v5/ARM/ARMCLANG/bin/armclang.exe`
- TI SysConfig: `D:/ti/sysconfig_1.28.0/sysconfig_cli.bat`
- MSPM0 SDK: `D:/ti/mspm0_sdk_2_10_00_04`
- DFP pack: `D:/Users/steven/AppData/Local/Arm/Packs/TexasInstruments/MSPM0G1X0X_G3X0X_DFP/1.3.1`
- SEGGER J-Link software: `C:/Program Files/SEGGER/JLink_V954`

The Keil-bundled J-Link DLL originally reported `V6.86` and did not recognize `MSPM0G3507`. Install a newer SEGGER J-Link package and run the DLL updater for Keil before flashing.

## Project Files

- Keil project: `keil/bringup_mspm0g3507_nortos_keil.uvprojx`
- SysConfig file: `empty.syscfg`
- Application source: `main.c`
- Generated DriverLib config: `ti_msp_dl_config.c`, `ti_msp_dl_config.h`

The Keil pre-build step calls `../../tools/keil/syscfg.bat` from the project directory. Because this project is outside the SDK tree, that wrapper is checked in at `firmware/tools/keil/syscfg.bat` and points directly at the SDK product metadata.

## Build

From Keil uVision, open:

```text
firmware/bringup/keil/bringup_mspm0g3507_nortos_keil.uvprojx
```

Select the `bringup_mspm0g3507_nortos_keil` target and build. The project is configured for Arm Compiler 6.

Command-line build:

```bat
D:\Keil_v5\UV4\UV4.exe -b firmware\bringup\keil\bringup_mspm0g3507_nortos_keil.uvprojx -o firmware\bringup\keil\build.log
```

## Flash And Debug

Use J-Link SWD, not ST-LINK.

Validated Keil settings:

```text
Options for Target -> Debug
Use: J-LINK / J-Trace Cortex

Settings:
  Interface: USB
  Port: SW
  Max speed: 100 kHz
  Connect: Normal
  Reset: Normal
```

Successful connection detected:

```text
SW Device: ARM CoreSight SW-DP
IDCODE: 0x6BA02477
```

The board-side SWD header near USB-C is labeled:

```text
GND DIO CLK 3V3
```

Map it to the ARM V9 debugger as `GND`, `DIO/TMS`, `CLK/TCK`, and `VT/3.3`. Leave the debugger `5V*` unconnected. If connect-under-reset is needed, connect debugger `RST` to the board `RST` pin on the `A18 / RST` row.

Successful flash output should include:

```text
Erase Done.
Programming Done.
Verify OK.
```

If Keil reports a flash timeout after programming, press the board `RST` button and check whether PB22 starts blinking.

## Runtime Behavior

After flashing, PB22 toggles every 500 ms. After a Step 7 lap stop, PB22 fast
blinks every 100 ms as the current light indication. UART0 prints Step 7
telemetry every 200 ms. The firmware boots in `STBY`; motors do not start until
the serial command `g`, `G`, `F`, `f`, or `v` is received.

UART0 startup and telemetry look like:

```text
=== ER2024 Step7: One-lap Line PID ===
PWM: PA8=HW-A/PWMA/AO/right PA9=HW-B/PWMB/BO/left 4kHz  Dir: PA16/21/22/23  STBY:PB20
Enc: HW-A=PB17/18 HW-B=PB24/25; logical sides use orient flags
Line: black=1; sensorFlip decides logical left/right
Commands: A=seg auto  g=Step7 lap  G=line  F=arm white->black  f=fwd jog  v=rev jog  x=coast  b=brake  +/-=base  p/P=Kp  d/D=Kd
Orient: w=swap motors  1=inv PWMA  2=inv PWMB  s=flip sensor  e=swap enc  3=inv encA  4=inv encB  r=reset enc  ?=help
Lap: [ target-1000  ] target+1000 ticks; PB22 fast blink means lap done
Gap: , bias-5  . bias+5; F stops after 3 confirmed black ticks
base=180 Kp=55 Ki=0 Kd=35
orient mSwap=1 invA=1 invB=1 sensorFlip=1 eSwap=1 eInvA=1 eInvB=1
Boot mode is STBY. Send 'g' from A after wheels are safe.
mode=STBY line=0b00011000 st=OK n=2 err=0 corr=0 base=180 L=0 R=0 encL=0 encR=0 odom=0 tgt=26000 gap=0 lap=OFF gapTest=OFF gapOdom=0 gapRaw=0b00000000 gapBlk=0 gapDL=0 gapDR=0 gapBias=15 auto=OFF seg=OFF segOdom=0 autoReady=0
```

Serial commands:

| Command | Effect |
| --- | --- |
| `A` | Start the current non-closed-field segmented run: AB white straight, BC black arc, CD white straight, DA black arc, then stop near A. |
| `g` | Start Step 7 one-lap line following from A. It clears the encoder counters and stops automatically when `odom >= tgt`. |
| `G` | Arm continuous low-speed line following without the Step 7 lap stop. Use this to re-test Step 6 behavior. |
| `F` | Arm a gap-segment measurement. It waits without moving until the sensor sees all white, clears odometry at that moment, drives straight with encoder closed-loop balancing, then short-brakes and saves `gapOdom` after 3 consecutive black detections. |
| `f` | Jog logical forward at current base speed. Use with wheels lifted. |
| `v` | Jog logical reverse at current base speed. Use with wheels lifted. |
| `x` | Coast stop and return to standby. |
| `b` | TB6612 short brake and return to brake mode. |
| `+` / `-` | Increase/decrease base speed by 20 PWM ticks. |
| `p` / `P` | Increase/decrease steering `Kp` by 5. |
| `d` / `D` | Increase/decrease steering `Kd` by 5. |
| `[` / `]` | Decrease/increase the Step 7 lap target by 1000 encoder ticks. Default target is `26000`. |
| `,` / `.` | Decrease/increase the white-gap straight-run right-wheel bias by 5. Default is `15`; higher values boost the right wheel and reduce right-turn drift. |
| `w` | Toggle logical left/right motor-channel swap. Default is `1`. |
| `1` / `2` | Toggle HW-A/HW-B motor direction inversion. Defaults are both `1`. |
| `s` | Toggle sensor left/right sign. Default is `1`. |
| `e` | Toggle logical left/right encoder swap. Default is `1`. |
| `3` / `4` | Toggle HW-A/HW-B encoder sign inversion. Defaults are both `1`. |
| `r` | Reset left/right encoder counters and displayed odometer. |
| `?` | Print command help and current parameters. |

The Step 7 controller runs at 50 Hz. It converts the muxed sensor byte into a
weighted line error, then applies a PD steering correction around the base
speed. Lowercase `g` also clears the encoder counters and enables a lap-distance
exit condition based on the average of logical `encL` and `encR`. `st=LOST`
(`0x00`) is tolerated for about 300 ms using the previous steering correction
to help with short dashed-line gaps, then commands TB6612 short brake.
`st=FULL` (`0xFF`) still commands immediate short brake.

Uppercase `F` is for the current non-closed field layout. It is safe to send
`F` before moving the car to the all-white gap: the firmware waits with motors
off while it sees black, then resets odometry and starts driving only after it
sees `st=LOST` / `line=0b00000000`. It ignores one- or two-tick black glitches
in the white area and stops only after 3 consecutive control ticks with
`activeCount > 0`. Telemetry `gapBlk` shows this confirmation counter. After
stopping it keeps printing the saved result as
`gapTest=DONE gapOdom=... gapRaw=...`, so the USB cable can be unplugged during
the run and reconnected afterward for readback. While running on white, `F`
uses encoder speed-plus-position balancing: `gapDL` and `gapDR` show the latest
left/right encoder increments, and the firmware boosts the slower side on the
next control tick. If the car still turns right, send `.` one or more times
before the next test; if it turns left, send `,` to reduce the bias. If `gapDR`
stays much smaller than `gapDL` even while telemetry shows `R` much larger than
`L`, inspect the right wheel, motor wiring, gearbox, and TB6612 output path.

Uppercase `A` is the first full-field automation for the current non-closed
track. It does not use the old pure `g` lap target. For AB and CD it now uses
the same start condition as `F`: wait until the sensor sees all white, reset the
straight controller at that exact white start point, then drive forward. In
telemetry, `autoReady=0` means the segment is waiting for all-white;
`autoReady=1` means the straight controller has been initialized. Once running,
AB/CD ignore black detections before 1200 ticks, then switch segment after 3
confirmed black ticks. It runs BC and DA using the PD line follower, switching
out of the arc after it is inside the measured exit window and then sees
confirmed line loss. Telemetry fields `auto`, `seg`, `segOdom`, and
`autoReady` show the current automatic state. Segment changes also print an
explicit `[AUTO] transition to=... code=...` line before entering the next
segment; AB should transition to `BC code=2`, then CD should transition to
`DA code=4`. During `A`, serial commands other than `b/B` are ignored so stray
UART bytes cannot cancel the automatic run.

AUTO telemetry is intentionally slower than standby telemetry because UART0 is
only 9600 baud and every blocking status line pauses the cooperative control
loop. The command byte is no longer echoed from the UART ISR; the serial
monitor already shows sent commands, and this avoids TX re-entry while the
controller is printing. AB/CD use a one-sample black trigger after the minimum
odometer window. On AB->BC and CD->DA transitions, the controller immediately
uses the current black-line sensor error to run the normal line PID; there is
no fixed arc-acquire turn. If the black line is missed and the white-gap
odometer exceeds the guard window, AUTO brakes with `gap missed black` instead
of forcing a blind transition.

2026-07-01 orientation correction: `AO1/AO2` controls the physical right wheel,
`BO1/BO2` controls the physical left wheel, and the current forward direction is
opposite the previous Step 5 check. The default orientation flags therefore
swap motor sides and invert both motor channels. Telemetry `L/R` and
`encL/encR` are logical car-left/car-right after this transform.

`bit0` is channel `CH1` / board mark `X1`, and `bit7` is channel `CH8` /
board mark `X8`. Raw sensor polarity is black/dark=`1`, white/bright=`0`.
Whole-array sanity check: all channels covered or on black/dark should print
`0xFF`; all channels on white/bright should print `0x00`.
Received UART bytes are echoed back from the UART0 RX interrupt.

SysConfig 1.28 did not accept the earlier PA2/PA14 PWM choice in this project,
so the hardware PWM wires must stay on PA8 and PA9 for the current firmware.

The 2026-06-30 bench wiring and observations are recorded in
`hardware_docs/step5_motor_encoder_bringup_record.md`. The Step 6 line PID
first-test procedure is recorded in
`hardware_docs/step6_pid_line_following_bringup_record.md`. The Step 7 one-lap
test sequence is recorded in
`hardware_docs/step7_one_lap_encoder_stop_bringup_record.md`.

Open the onboard CH340 serial port at `9600-8N1`. Windows has assigned this
board `COM9` in earlier tests and `COM11` in the 2026-07-01 Step 6 validation
log, so use the current `USB-SERIAL CH340` port shown on the PC. The J-Link
virtual serial port is separate; do not use it for this board UART check.
