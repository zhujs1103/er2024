# Purchased Power Components

## Battery Lines — `HW-20260621-03` And `HW-20260621-13`

Both cart lines show a `12 V / 1500 mAh` smart-car lithium pack with an XH-style lead; displayed prices differ (CNY 17.80 and CNY 23.80), so retain them as separate SKUs until order detail confirms the bundle difference.

The product thumbnail also mentions an XH2.54 lead, DC female lead and a 1 A charger bundle, but it is not clear which cart line includes which accessories.

Before use, verify:

- Cell count/chemistry and full-charge voltage (a nominal 3-series lithium pack can reach about 12.6 V).
- BMS presence, continuous/peak current and low-voltage cutoff.
- Connector polarity and charger output.
- Actual capacity and voltage sag under motor/servo load.

Do not assume the pack can supply four 520 motor stall currents plus two 15 kg servos.

## `HW-20260621-07` — LM2596 Power Module

- Adjustable LM2596-based power module; title also contains `dc3.3V`, so the selected board/output configuration needs confirmation.
- Quantity 1; displayed price CNY 17.19.
- Official LM2596 IC references are in `../official_reference_links.md`.

Map every input/output connector and adjust outputs with no load before connecting electronics. Measure temperature and voltage sag at the real servo/sensor load; the IC headline rating is not a guarantee of module continuous current.

## `HW-20260621-14` — XT30 Adapter Leads

- XT30 male-to-female, claimed 16 AWG / 1.26 mm², 10 cm, 10 A.
- Quantity 2; displayed price CNY 4.20 each shown.

Verify male/female orientation, red-positive polarity, solder joints, resistance and heat rise before battery use.

## Power Architecture Implication

Plan separate branches for motors, logic/sensors and servos/actuators. The purchased list does not yet show a proven high-current 5-6 V servo regulator or a fuse/emergency-stop component, so those remain open safety items.
