# Purchased Hardware Registry

Last updated: 2026-06-21

This is the source of truth for hardware that the team reports as purchased. The earlier `project_management/purchase_list.md` is a planning document, not proof of purchase.

## Evidence Rules

- `Confirmed from image`: text or dimensions are directly visible in the supplied screenshot.
- `Reported purchased`: the user states that the shared cart items were purchased; order quantities are not yet visible.
- `Bench verified`: measured from the received part. No item has reached this state yet.
- Blank or unknown fields must stay unknown until the exact order SKU, seller parameter table, package label, or a bench test resolves them.

## Source Snapshot

- User report: the shared cart contains 17 purchased items.
- Taobao short link: <https://m.tb.cn/h.RLEGtGJ>
- Share ID recovered from the redirect: `3008f686577a7cc209401953e8071084d6e94955`
- Supplied screenshot date: 2026-06-21.
- Five overlapping Taobao screenshots now expose the complete 17-line shared-cart list.
- Prices below are the displayed cart prices and are recorded only to distinguish lines/SKUs; they are not an accounting record.
- Selected option text is sometimes truncated by the Taobao UI. Truncated fields remain explicitly unresolved.

## Confirmed Items

| ID | Item / visible selected option | Qty | Displayed price | Coding or integration relevance | Documentation state |
| --- | --- | ---: | ---: | --- | --- |
| `HW-20260621-01` | Adjustable-focus 405 nm blue-violet dot laser; visible option starts `405nm 蓝紫色激光 10mw...` | 1 | CNY 25.50 | Switched actuator output; must use an external MOSFET/load switch and default off | Generic module; voltage/current/polarity/driver/class still unknown. See `05_servo_gimbal_actuator/405nm_adjustable_laser_module.md`. |
| `HW-20260621-02` | Pure-copper-claimed red/black parallel wire; `纯铜2X0.3平; 1m` | 1 m | CNY 2.55 | Low-current power wiring only until tested | Gauge/length now visible; insulation/current rating and delivered conductor material remain unknown. See `06_power_safety/red_black_parallel_wire.md`. |
| `HW-20260621-03` | `12V大容量锂电池组DIY智能小车`; visible option starts `12V【1500mAh】XH插头...` | 1 | CNY 17.80 | Candidate battery line A | Generic 3-series pack; connector bundle/BMS/charger content and current rating need order detail and receiving verification. |
| `HW-20260621-04` | Yahboom 8-channel grayscale line sensor, white-light version, soldered header; received PCB marked `YB-MYX05-V1.0` | 1 | CNY 59.40 | Muxed sensor interface: `AD2/AD1/AD0` address outputs plus `OUT`; measured `OUT` is 3.3 V-safe, white/bright=`0`, black/dark=`1` | Official product/tutorial links found; received-board photos and bench measurement changed the firmware/electrical interface. See `03_line_sensors/purchased_yahboom_8_channel_sensor.md`. |
| `HW-20260621-05` | 42-size stepper motor, encoder version; image identifies `MS42CG`, 5 mm output shaft | 1 | CNY 57.80 | Alternative motion/actuator path; needs phase and encoder interfaces | Coil current/resistance, step angle, encoder CPR and exact suffix are still hidden. See `02_chassis_motors/purchased_motion_hardware.md`. |
| `HW-20260621-06` | Bluetooth smart-car kit; visible option starts `【升级版】4WD黑色玻...` | 1 | CNY 23.76 | Mechanical chassis candidate | Dimensions, included motors, mounting pattern and final compatibility with 520 motors must be checked. |
| `HW-20260621-07` | `LM2596稳压电源模块`; selected line says adjustable power module; title also contains `dc3.3V` | 1 | CNY 17.19 | Power conversion/distribution board | LM2596 IC datasheet found; exact custom-board outputs and thermal current limit remain unknown. See `06_power_safety/purchased_power_components.md`. |
| `HW-20260621-08` | Simple two-axis gimbal; visible option starts `简易二维云台（15kgPWM...` and product image shows two 15 kg PWM servos | 1 set | CNY 113.00 | Two servo-PWM outputs and independent high-current actuator rail | Exact servo model, voltage, stall current, pulse limits and mechanical limits are unknown. See `05_servo_gimbal_actuator/purchased_2d_servo_gimbal.md`. |
| `HW-20260621-09` | `TB6612FNG双路四路直流电机驱动`; selected line starts `【焊接排针】TB6612双路...`; product image code `D153C` | 1 | CNY 35.70 | Candidate dual brushed-DC driver | Official TB6612FNG reference found; custom board pinout/regulator/current capability still need seller material. See `04_motor_driver/purchased_motor_drivers.md`. |
| `HW-20260621-10` | Dual stepper-motor driver module with 5 V regulated output; product image identifies regulated version `D36A` | 1 | CNY 59.00 | Driver candidate for item 05 | Control mode and pinout are unknown; do not assume STEP/DIR. Seller states MSPM0G3507 example code is available. |
| `HW-20260621-11` | `520编码器电机12V大扭矩PID源码`; visible option starts `520单独编码器电机; 320r/...` | 4 | CNY 48.00 each shown | Main 4WD drive candidate; encoder feedback | 12 V and apparent 320 r/min option are visible, but gear ratio, stall current, encoder voltage/type/CPR remain unknown. |
| `HW-20260621-12` | `MSPM0G3507核心开发板`; visible option starts `MSPM0G3507高配板（已...` | 1 | CNY 75.00 | TI main controller | Official MCU datasheet/TRM/SDK references found; this third-party board still needs its schematic, pin map and exact populated variant. See `01_ti_mcu/purchased_mspm0g3507_core_board.md`. |
| `HW-20260621-13` | Second `12V大容量锂电池组DIY智能小车` line; visible option again starts `12V【1500mAh】XH插头...` | 1 | CNY 23.80 | Candidate battery line B | Kept separate from item 03 because price differs; likely bundle/SKU difference must not be guessed. |
| `HW-20260621-14` | XT30 male-to-female lead; visible option starts `XT30公转母16AWG 1.26...`; product image shows 10 cm and 10 A claim | 2 | CNY 4.20 each shown | Battery/power adapter | Verify connector polarity, crimp/solder quality and conductor area before use. |
| `HW-20260621-15` | `SS12D10` three-pin, two-position slide switch; selected pack `10片装` | 1 pack | CNY 9.59 | Mode input or low-current control; not yet approved as main power switch | Exact contact rating/datasheet and received pin topology need verification. |
| `HW-20260621-16` | 6 x 6 x 5 mm vertical four-pin tactile switch | 1 order | CNY 2.16 | Button inputs with pull-up and software debounce | Product image says one order equals 50 pieces; verify received count and electrical rating. |
| `HW-20260621-17` | Condensing spotlight LED; `DC5V小号白光（15MM直径）` | 1 | CNY 5.40 | Switched illumination/status output; use transistor/MOSFET, not MCU pin power | Current and polarity need measurement; no addressable control interface is indicated. |

## Duplicate And Bundle Notes

- Items 03 and 13 are intentionally separate. Both display `12 V / 1500 mAh` text, but their prices differ (`17.80` vs `23.80` CNY), so a charger/connector bundle difference may exist. Confirm from the order-detail page.
- Item 11 is one cart line with quantity four.
- Item 14 is one cart line with quantity two.
- Item 15 is one line for a 10-piece pack. Item 16 may be a 50-piece pack based on the product image, not the truncated selected-option text.

## Minimum Fields Needed Before Firmware Pin Assignment

For every active module, collect:

1. Exact product title and selected SKU/variant.
2. Quantity received.
3. Supply voltage range and typical/maximum current.
4. Logic voltage, interface type, pin names, and whether signals are active-high or active-low.
5. Mechanical dimensions and connector type.
6. Manufacturer/board revision or clear front/back photographs.
7. Datasheet, schematic, seller parameter table, or a local bench-test record.

## Receiving Workflow

1. Photograph the package label and both sides of each board/module before wiring.
2. Match the item to its inventory ID.
3. Record silkscreen, IC markings, selected SKU, and quantity.
4. Check continuity and polarity with power disconnected.
5. Use a current-limited bench supply for first power-on.
6. Record measured idle/current peaks and logic thresholds.
7. Only then mark the module `Bench verified` and use its values in firmware configuration.
