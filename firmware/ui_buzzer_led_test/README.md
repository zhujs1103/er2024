# ER2024 UI Buzzer And LED Test

Purpose: independent smoke-test firmware for the external active buzzer module
and one external LED. This directory is separate from `firmware/abcd_fresh` so
the current A-mode baseline is not changed.

## Pins

Current drivetrain/sensor firmware already uses PA8/PA9, PA16/PA21/PA22/PA23,
PB20, PB17/PB18/PB24/PB25, PA24/PA25/PA26/PA27, PB2/PB3, PA10/PA11, and PB22.
For this UI-only test, use two currently free GPIO pins:

| Function | MSPM0 pin | Notes |
| --- | --- | --- |
| Buzzer `I/O` | `PA2` | Active-low output. High = silent, low = beep. |
| External LED | `PA14` | Active-high output through the external 1 kOhm resistor. |
| Onboard LED | `PB22` | Flashes together with the external LED for confirmation. |

## Wiring

Buzzer module:

```text
Buzzer VCC -> 3.3 V
Buzzer GND -> GND
Buzzer I/O -> PA2
```

LED:

```text
PA14 -> 1 kOhm resistor -> LED long leg/anode
LED short leg/cathode -> GND
```

Keep motor power disconnected for this test. The firmware also forces TB6612
`STBY` low after startup, but this test should be treated as UI-only.

## Build

```text
D:\Keil_v5\UV4\UV4.exe -r D:\steven\work\er2024\firmware\ui_buzzer_led_test\keil\ui_buzzer_led_test_mspm0g3507_keil.uvprojx -o D:\steven\work\er2024\firmware\ui_buzzer_led_test\keil\rebuild.log
```

Expected hex:

```text
firmware/ui_buzzer_led_test/keil/Objects/ui_buzzer_led_test_mspm0g3507_keil.hex
```

UART remains `115200-8N1` on the onboard CH340. The pattern repeats:

1. External LED and PB22 on for about 1 second.
2. Buzzer beeps twice.
3. External LED, PB22, and buzzer pulse together five times.
4. Everything stays off briefly, then repeats.
