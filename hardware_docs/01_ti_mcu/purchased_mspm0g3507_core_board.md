# Purchased MSPM0G3507 Core Board

Inventory ID: `HW-20260621-12`

## Screenshot Record

- Product: `MSPM0G3507核心开发板`.
- Visible option: `MSPM0G3507高配板（已...` (truncated; likely a populated/soldered variant, but do not infer the missing text).
- Quantity: 1.
- Displayed price: CNY 75.00.
- Photo shows a third-party white core board, USB cable and a printed pin map/reference card.

## Documentation

Use the TI MCU datasheet, technical reference manual and MSPM0 SDK links in `../official_reference_links.md` for firmware development.

Before creating the board support package, obtain from the seller or delivered kit:

- Full front/back pin map.
- Board schematic and exact PCB revision.
- Crystal/clock population.
- USB/debug circuit and boot/reset behavior.
- Regulator input/output limits.
- LED/button connections and active levels.
- Exact MCU package fitted to the board.

The LP-MSPM0G3507 LaunchPad guide is useful as a TI reference design but is not the schematic of this board.

## Code Setup Rule

Start from an MSPM0 SDK example for the exact MCU, then create a board-specific pin map. Do not use LaunchPad pin names in application code unless the delivered board actually matches them.
