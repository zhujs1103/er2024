# Purchased Motor Driver Modules

## `HW-20260621-09` — TB6612FNG Dual DC Driver Module

- Listing: `TB6612FNG双路四路直流电机驱动`.
- Selected option starts `【焊接排针】TB6612双路...`.
- Product image identifies custom board `D153C` and says MSPM0G3507 example source is available.
- Quantity 1; displayed price CNY 35.70.

Use the official Toshiba TB6612FNG reference in `../official_reference_links.md` for IC truth. Obtain the D153C pin map/schematic because the module may add a regulator and protection or change connector labels.

Critical check: do not place two 520 motors in parallel on one channel until their combined start/stall current is measured and proven safe for this module.

## `HW-20260621-10` — D36A Dual Stepper Driver

- Listing: dual stepper-motor driver module with 5 V regulated output.
- Product image identifies regulated version `D36A` and says MSPM0G3507 example source is available.
- Quantity 1; displayed price CNY 59.00.

Required seller files:

- Schematic and pin map.
- Motor/input voltage range.
- Per-phase continuous/peak current and current-limit method.
- Control mode: STEP/DIR, four-phase sequence, or direct PWM/direction.
- 5 V regulator current rating and whether it may power external loads.
- Matching MS42CG wiring and encoder example.

Do not begin a stepper driver implementation until control mode is confirmed.
