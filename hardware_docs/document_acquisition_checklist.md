# Document Acquisition Checklist

Use this file to track manuals and datasheets that should be collected.

| Priority | Item | Folder | Status | Notes |
| --- | --- | --- | --- | --- |
| P0 | MSPM0G3507 MCU datasheet/TRM/SDK | `01_ti_mcu/` | LINKED | Official TI references are in `official_reference_links.md` |
| P0 | Purchased MSPM0G3507 core-board schematic/pin map | `01_ti_mcu/` | BLOCKED | Third-party board; obtain seller files or photograph the included pin card and PCB revision |
| P0 | TB6612FNG IC datasheet | `04_motor_driver/` | LINKED | Official Toshiba references are in `official_reference_links.md` |
| P0 | D153C TB6612 module schematic/pin map | `04_motor_driver/` | BLOCKED | Needed before motor wiring and current-limit decisions |
| P0 | D36A stepper driver schematic/pin map/example | `04_motor_driver/` | BLOCKED | Must determine STEP/DIR versus direct phase control and current limit |
| P0 | 520 motor/encoder exact specification | `02_chassis_motors/` | BLOCKED | Need gear ratio, stall current, encoder voltage/type/CPR and mounting dimensions |
| P0 | MS42CG stepper/encoder exact suffix and specification | `02_chassis_motors/` | BLOCKED | Need phase current/resistance, step angle, wiring and encoder CPR |
| P0 | 4WD chassis dimensions and mounting drawing | `02_chassis_motors/` | BLOCKED | Must prove final vehicle stays within 25 x 15 x 25 cm and motors fit |
| P0 | Yahboom 8-channel sensor product/tutorial | `03_line_sensors/` | LINKED | Official vendor links found; confirm delivered revision, VCC, pin order and active level |
| P1 | 15 kg servo exact datasheet | `05_servo_gimbal_actuator/` | BLOCKED | Need model, voltage, stall current, angle range and PWM limits |
| P0 | Purchased 405 nm adjustable laser exact SKU / seller parameter table | `05_servo_gimbal_actuator/` | BLOCKED | Screenshot confirms 405 nm, 10 mW, dot, adjustable focus and approximate 12 x 36 mm body; voltage/current/polarity/driver/class remain unknown |
| P1 | LM2596 IC datasheet | `06_power_safety/` | LINKED | Official TI reference found; custom module schematic/thermal limit still unknown |
| P0 | Two battery SKU/BMS/charger specifications | `06_power_safety/` | BLOCKED | Two 12 V/1500 mAh lines have different prices; confirm full-charge voltage, current limit, polarity and bundle contents |
| P1 | Purchased red/black wire receiving verification | `06_power_safety/` | PARTIAL | Cart confirms 2 x 0.3 mm², 1 m; verify conductor and insulation/current suitability |
| P1 | XT30 lead polarity/current verification | `06_power_safety/` | PARTIAL | Cart claims 16 AWG, 10 cm, 10 A; inspect and load-test |
| P1 | IMU module datasheet | `02_chassis_motors/` | Optional | Useful for arc and recovery, not mandatory |
| P2 | SS12D10/tactile-switch exact ratings | `07_ui_debug/` | BLOCKED | Generic variants; continuity, contact rating and received pack count need verification |
| P1 | 5 V spotlight current/polarity | `07_ui_debug/` | BLOCKED | Measure before transistor/MOSFET selection |

## Naming Convention

Recommended filename pattern:

`<part-number>_<doc-type>_<vendor-or-source>.pdf`

Examples:

- `MSPM0G3507_datasheet_TI.pdf`
- `TB6612FNG_datasheet_Toshiba.pdf`
- `QTR-8RC_user-guide_vendor.pdf`

## Purchased Hardware Intake

See `purchased_hardware_registry.md` and `official_reference_links.md` before looking for manuals. A reliable manual search requires the exact selected SKU, board revision, or IC marking; a generic shopping title is not enough to bind firmware to electrical limits.
