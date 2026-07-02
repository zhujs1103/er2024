# Purchased Yahboom 8-Channel Grayscale Sensor

Inventory ID: `HW-20260621-04`

## Screenshot Record

- Product: `8路灰度传感器 无MCU 电赛专用`.
- Selected option: white-light version with headers soldered.
- Quantity 1; displayed price CNY 59.40.

## Vendor Documentation

- Product page: <https://category.yahboom.net/products/yahboom-8-gs>
- Tutorial folder: <https://drive.google.com/drive/folders/1DZYiSlijtCnp9gtIbcY1Ww1SV_gZWtgN?usp=sharing>

The vendor page confirms digital signal output and explicitly lists the white-light variant for black tracks on white backgrounds, matching the contest field concept.

## Received Board Photos

User photos from `2026-06-29` show the received PCB is marked
`YB-MYX05-V1.0`. It is not an 8-wire direct-output header. The 6-pin header is
silkscreened as:

```text
5V AD2 AD1 AD0 OUT GND
```

The board also prints a select table: `AD2 AD1 AD0 = 000` selects `CH1`, and
`111` selects `CH8`. The sensor side labels are `X1..X8`; in the top-side
photo, `X8` is on the left and `X1` is on the right.

## Bench Measurement

Measured on `2026-06-29`:

| Item | Result |
| --- | --- |
| Sensor supply | `5 V` |
| `OUT` on white/bright surface | logic `0`, about `0 V` |
| `OUT` on black/dark surface | logic `1`, about `3.3 V` |
| Logic polarity | white/bright = `0`, black/dark = `1` |
| Serial output, all channels black/dark or covered | `0xFF` |
| Serial output, all channels white/bright | `0x00` |
| MCU connection | `OUT` can connect directly to `PA25`; no resistor divider is needed |

Conclusion: this board uses 5 V for sensor/module power, but its data output is
already MSPM0-safe 3.3 V logic. Previous classmate/reference notes that asked
for a `10k + 20k` divider on `OUT` should not be followed for this received
sensor.

## Firmware Intake Checklist

- Confirmed: `5 V` supply and 3.3 V-safe `OUT`.
- Record left-to-right channel/pin order.
- Recorded from raw serial monitor: all black/dark or fully covered produces
  `0xFF`; all white/bright produces `0x00`.
- Measure switching stability versus mounting height and ambient light.
- Capture the eight muxed channel reads into one byte and keep any physical reversal in the board configuration.
- Implement lost-line and dashed-line states; do not treat `0x00` or `0xFF` as an ordinary weighted position.

## Step 4 Bring-Up Pin Map

Initial firmware in `firmware/bringup` now expects the received 6-pin muxed
board:

| Sensor pin | MSPM0 connection | Notes |
| --- | --- | --- |
| `5V` | 5 V sensor supply | Use a measured 5 V rail. |
| `AD2` | `PA24` output | Mux address bit 2. |
| `AD1` | `PA27` output | Mux address bit 1. |
| `AD0` | `PA26` output | Mux address bit 0. |
| `OUT` | `PA25` input | Direct connection is OK; measured high level is about 3.3 V. |
| `GND` | Common ground | Must share ground with MSPM0. |

Channel packing used by the debug firmware:

| Raw bit | Board channel | `AD2 AD1 AD0` |
| --- | --- | --- |
| bit 0 | `CH1` / `X1` | `000` |
| bit 1 | `CH2` / `X2` | `001` |
| bit 2 | `CH3` / `X3` | `010` |
| bit 3 | `CH4` / `X4` | `011` |
| bit 4 | `CH5` / `X5` | `100` |
| bit 5 | `CH6` / `X6` | `101` |
| bit 6 | `CH7` / `X7` | `110` |
| bit 7 | `CH8` / `X8` | `111` |

`OUT` has been measured as 3.3 V-safe, so the Step 4 wiring is a direct
connection to `PA25`.

UART debug target:

```text
line mux=0b00011000 hex=0x18
```

Record the observed black/white polarity here after bench testing:

| Surface | Expected record |
| --- | --- |
| White/bright background | `0` / about `0 V` |
| Black/dark track | `1` / about `3.3 V` |

Whole-array sanity check:

| Whole sensor state | Expected serial byte |
| --- | --- |
| All channels covered or over black/dark surface | `0xFF` |
| All channels over white/bright surface | `0x00` |

One unit was purchased, so mechanical protection and a spare plan remain advisable.
