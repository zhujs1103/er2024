# Official Reference Links

Checked: 2026-06-21

These references are tied to identifiable chips or branded modules. They do not replace the schematic/pinout of the third-party boards actually purchased.

## TI MSPM0G3507

- Product page: <https://www.ti.com/product/MSPM0G3507>
- MCU datasheet: <https://www.ti.com/lit/ds/symlink/mspm0g3507.pdf>
- MSPM0 G-Series 80-MHz MCU technical reference manual (latest revision redirect): <https://www.ti.com/lit/pdf/slau846>
- LP-MSPM0G3507 LaunchPad user guide (reference design only; this is not the purchased board): <https://www.ti.com/lit/pdf/slau873>
- MSPM0 SDK product/download page: <https://www.ti.com/tool/MSPM0-SDK>
- MSPM0 SDK documentation overview: <https://software-dl.ti.com/msp430/esd/MSPM0-SDK/latest/docs/english/MSPM0_SDK_Documentation_Overview.html>
- TI Resource Explorer, latest MSPM0 SDK: <https://dev.ti.com/tirex/global?id=MSPM0-SDK>

Use the MCU datasheet/TRM for peripheral behavior and the SDK examples for code structure. Use the purchased board's own pin map/schematic for connector-to-pin mapping.

## Toshiba TB6612FNG

- Official product page: <https://toshiba.semicon-storage.com/ap-en/semiconductor/product/motor-driver-ics/brushed-dc-motor-driver-ics/detail.TB6612FNG.html>
- Official datasheet endpoint: <https://toshiba.semicon-storage.com/info/docget.jsp?did=10660&prodName=TB6612FNG>

The purchased `D153C` module may add regulators, connectors, protection, or changed labels. Confirm its board schematic/pin map before reusing a generic TB6612 driver.

## TI LM2596

- Product page: <https://www.ti.com/product/LM2596>
- IC datasheet: <https://www.ti.com/lit/ds/symlink/lm2596.pdf>

The IC datasheet does not establish the continuous current of the purchased module. Inductor, diode, PCB copper and cooling usually set a lower practical limit than the headline IC rating.

## Yahboom 8-Channel Grayscale Sensor

- Official product page: <https://category.yahboom.net/products/yahboom-8-gs>
- Official tutorial folder linked by Yahboom: <https://drive.google.com/drive/folders/1DZYiSlijtCnp9gtIbcY1Ww1SV_gZWtgN?usp=sharing>

The official product page confirms digital outputs, MSPM0 compatibility, and that the white-light variant is intended for black tracks on a white background (or the inverse). Pin order, active level and supply voltage still need to be checked against the delivered board/tutorial revision.

## Modules With No Safe Datasheet Match Yet

The following need seller documentation or received-part markings before a specific manual can be bound:

- `MS42CG` encoder stepper motor exact suffix.
- `D36A` dual stepper driver module.
- 12 V 520 geared encoder motor exact ratio/encoder variant.
- Third-party MSPM0G3507 high-configuration core board schematic/pin map.
- 15 kg PWM servo exact model.
- 12 V / 1500 mAh battery packs and charger/BMS variants.
- 405 nm / 10 mW laser module electrical variant.
- SS12D10 and 6 x 6 x 5 mm switch manufacturer variants.

Do not substitute a visually similar manual for these parts.
