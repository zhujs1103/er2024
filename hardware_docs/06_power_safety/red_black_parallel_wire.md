# Red/Black Two-Core Parallel Wire

Inventory ID: `HW-20260621-02`

## What Is Confirmed

- Listing text visible in the screenshot: `国标纯铜红黑线2芯双色并线 平行线电源线LED喇叭电子...`.
- Two conductors with red and black insulation are pictured.
- The listing image claims copper conductor.
- Selected option: `纯铜2X0.3平; 1m` (two conductors, nominal `0.3 mm²` each, one metre).

## Unknown

- Strand count and strand diameter.
- Insulation material, temperature rating, and voltage rating.
- Maximum current rating.
- Whether the delivered conductor is bare copper, tinned copper, or copper-clad material.

Because the exact SKU is unknown, this wire must not yet be assigned to the battery, motor, or servo high-current path.

## Project Convention

- Red: positive supply.
- Black: ground/return.
- Do not reuse either color for signal wiring.
- Label both ends when multiple power rails are present.
- Size wire from measured peak current, run length, allowed voltage drop, and insulation temperature rating.

## Receiving Checks

1. Record selected SKU, gauge, and purchased length.
2. Measure conductor diameter or cross-sectional area; do not include insulation.
3. Count strands from a clean cut if the seller specification is ambiguous.
4. Measure resistance of a known length with lead resistance compensated.
5. Inspect a stripped end and record conductor material/plating.
6. Perform a current/temperature test before using it for motors, servos, or the main battery feed.
7. Add the verified gauge and permitted project use to the table below.

| Field | Value |
| --- | --- |
| Cart-selected SKU | `纯铜2X0.3平; 1m` |
| Ordered length | 1 m |
| Claimed conductor area | 2 x 0.3 mm² |
| Verified conductor area/gauge | TODO after receiving |
| Measured resistance per metre | TODO |
| Insulation rating | TODO |
| Approved project use | TODO |

This passive material has no firmware driver or component manual. Its software relevance is indirect: inadequate wire can cause brownouts, noisy ADC readings, servo resets, and motor-control faults that look like firmware bugs.
