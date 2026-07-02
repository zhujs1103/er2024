# MSPM0G3507 Bring-Up Notes

Status date: 2026-06-29

These notes capture the first successful bench bring-up of the JLC Tianmengxing MSPM0G3507 board with Keil, SysConfig, and an ARM V9 / J-Link-compatible debugger.

## Confirmed Hardware Connections

Use the board USB-C connector for board power and onboard CH340 serial. Do not power the board from the debugger `5V*` pin during this bring-up.

Board SWD header near USB-C, by board silkscreen:

| Board pin | Debugger signal |
| --- | --- |
| `GND` | Any 20-pin ARM GND |
| `DIO` | `DIO/TMS` |
| `CLK` | `CLK/TCK` |
| `3V3` | `VT/3.3` |

Optional reset wire:

| Board pin | Debugger signal |
| --- | --- |
| Right pin of the `A18 / RST` row on the left double-row header | `RST` |

ARM V9 debugger 20-pin mapping from the debugger case label, with USB on the right and the 20-pin cable on the left:

| 20-pin ARM pin | Signal | Board connection |
| --- | --- | --- |
| 1 | `VT/3.3` | `3V3` |
| 7 | `DIO/TMS` | `DIO` |
| 9 | `CLK/TCK` | `CLK` |
| 15 | `RST` | `RST`, optional but useful for connect-under-reset |
| 4/6/8/10/12/14/16/18/20 | `GND` | `GND` |
| 19 | `5V*` | Leave unconnected |

Important lesson from bench debugging: the board-side 4-pin SWD header was correct, but the 20-pin ribbon-to-Dupont mapping is easy to get wrong. Use the 20-pin numbers and the debugger label, not Dupont wire colors.

## Debugger And Keil Settings

SEGGER J-Link software was updated to `V9.54`. The Keil-bundled `D:\Keil_v5\ARM\Segger\JLinkARM.dll` was originally old (`V6.86`) and did not know `MSPM0G3507`.

Keil settings that connected successfully:

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

Successful SWD detection showed:

```text
SW Device: ARM CoreSight SW-DP
IDCODE: 0x6BA02477
```

Successful flash output showed:

```text
Erase Done.
Programming Done.
Verify OK.
Flash Load finished
```

If Keil loops on factory reset / mass erase prompts, check the 20-pin ribbon mapping first, then lower SWD speed, then try `Connect: Under Reset` with the optional `RST` wire.

## Serial Console

The board's onboard CH340 enumerated as:

```text
USB-SERIAL CH340 (COM9)
```

The debugger also enumerated a virtual serial port:

```text
JLink CDC UART Port (COM10)
```

Use `COM9` for this board bring-up, not `COM10`.

Serial settings:

```text
9600 baud
8 data bits
1 stop bit
No parity
No flow control
```

The first LED/UART smoke test printed `Hello ER2024`. The current bring-up firmware has advanced to line-sensor raw output and should print:

```text
ER2024 line sensor mux bringup: AD0 PA26 AD1 PA27 AD2 PA24 OUT PA25
bit0=CH1/X1 bit7=CH8/X8, confirm physical left/right on the car
line mux=0b00011000 hex=0x18
```

Raw polarity observed on the serial monitor: black/dark or fully covered across
all channels prints `0xFF`; white/bright across all channels prints `0x00`.

## Current Status

- Keil project built with `0 Error(s), 0 Warning(s)`.
- J-Link/SWD connection works after correcting the 20-pin mapping and using `Port: SW`.
- Flash download reached `Verify OK`.
- PB22 LED started after manual board reset.
- Current serial validation target: open `COM9` at `9600-8N1` and check for line-sensor raw printouts.
