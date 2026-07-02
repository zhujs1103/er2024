# Purchased Controls And Lighting

## `HW-20260621-15` — SS12D10 Slide Switches

- Three-pin, two-position slide switch.
- Selected pack: 10 pieces; displayed price CNY 9.59.
- Use as a logic/mode input unless its manufacturer contact rating is verified for the intended power current.

For GPIO use, connect the centre/common contact deliberately, enable a pull-up/pull-down, and confirm the two outer positions with a continuity meter.

## `HW-20260621-16` — 6 x 6 x 5 mm Tactile Buttons

- Four-pin vertical tactile switch; one cart order at CNY 2.16.
- Product image says one order equals 50 pieces; verify received count.

The two pins on each side are normally common. Verify with a meter, use a pull-up and start firmware debounce around 10-20 ms before tuning from measurements.

## `HW-20260621-17` — 5 V White Spotlight LED

- Selected option: `DC5V小号白光（15MM直径）`.
- Quantity 1; displayed price CNY 5.40.
- Treat as a two-wire DC load, not an addressable LED.

Measure current and polarity, then switch it with a transistor/MOSFET or load switch. Do not power it directly from an MCU GPIO.
