# AI Usage Log

Use this file as source material for the design report's AI usage section.

## 2026-06-15

Tool: Codex

Usage:

- Read and extracted the official contest PDF.
- Rendered the PDF pages to images and checked the track diagram.
- Built the initial project documentation structure.
- Created first-pass requirement summary, hardware-document checklist, roadmap, and risk register.
- Attempted to read the provided ChatGPT shared conversation through local Chrome.

Benefits:

- Converted scattered contest requirements into engineering checklists.
- Separated hard constraints, basic scoring items, advanced scoring items, and submission requirements.
- Prepared a hardware documentation workflow before part selection begins.

Limitations:

- The ChatGPT share page was visible in Chrome, but automated DOM/screenshot extraction timed out. The conversation title was confirmed as "TI 巡线小车方案"; detailed content still needs to be imported by a working extraction method or manual export.

## 2026-06-16

Tool: Codex

Usage:

- Produced the first purchase list for the contest car.
- Cross-checked key component directions against official documentation for TI MSPM0G3507, TI LP-MSPM0G3507, TI DRV8833, and Pololu QTR-8A style line sensors.
- Organized the list into core build, power/safety, advanced/debug, field materials, and consumables.

Benefits:

- Turned requirements into a concrete buying checklist.
- Marked must-buy components separately from optional advanced parts.
- Added "do not buy" warnings for illegal or risky choices such as non-TI main controllers, mecanum wheels, tracked chassis, and camera-based line tracking.

Limitations:

- Store availability and prices change frequently, so final purchasing should verify stock, seller quality, shipping time, and exact module pinout before checkout.

## 2026-06-29

Tool: Codex

Usage:

- Created and verified the first Keil/SysConfig MSPM0G3507 bring-up firmware under `firmware/bringup`.
- Updated bring-up firmware behavior through the bench flow: PB22 LED health check, UART0 debug output, and current muxed Yahboom line-sensor raw printout.
- Guided J-Link setup for the JLC Tianmengxing MSPM0G3507 board, including SEGGER J-Link update from the old Keil-bundled `V6.86` DLL to `V9.54`.
- Resolved the debugger configuration from the wrong CMSIS-DAP/JTAG path to J-Link SWD at `100 kHz`.
- Helped verify the 20-pin ARM ribbon mapping: `VT/3.3`, `DIO/TMS`, `CLK/TCK`, optional `RST`, and `GND`; left debugger `5V*` unconnected.
- Recorded the successful SWD detection (`ARM CoreSight SW-DP`, `IDCODE 0x6BA02477`) and successful Keil flash output (`Erase Done`, `Programming Done`, `Verify OK`).

Benefits:

- Turned first-board bring-up into a repeatable wiring and Keil setup procedure.
- Preserved the key trap: the 20-pin ribbon mapping must be checked by pin number, not wire color.
- Confirmed that the board CH340 serial port is `COM9`, while the J-Link CDC port is `COM10`.

Limitations:

- PB22 LED was confirmed to run after manual reset; serial output still needs user-side confirmation in a serial assistant at `COM9`, `9600-8N1`.
- Some Keil reset/download behavior may still require manual board reset after flashing, even when programming verifies successfully.
