# 405 nm Adjustable-Focus Dot Laser Module

Inventory ID: `HW-20260621-01`

## What Is Confirmed

The 2026-06-21 cart-share screenshot visibly shows:

- Listing title: `聚焦调节405nm蓝紫色激光头 点状`.
- Wavelength marking: `405 nm`.
- Advertised optical power marking: `10 mW`.
- Output: dot.
- Adjustable focus.
- Current full-cart screenshot product image marks about `12 mm` barrel diameter and `36 mm` body length.
- Two flying leads are shown, but their polarity and electrical ratings are not readable.

These are seller-image observations, not bench measurements or a manufacturer datasheet.

## Unknown — Resolve Before Power-On

- Full electrical SKU suffix. Quantity is confirmed as one.
- Rated supply voltage and allowed tolerance.
- Typical/startup current.
- Lead polarity.
- Whether a constant-current driver is built into the barrel.
- Modulation support and maximum switching frequency.
- Laser safety class, certification, and actual optical output.
- Continuous-duty thermal limit.

Do not assume that this module is the earlier planned `5 V red laser module`; it is a different wavelength and its supply voltage is currently unknown.

## Electrical Integration Rule

The firmware-facing abstraction should be a single fail-safe `laser_enable` output, but the MCU must not power the laser directly.

Recommended hardware boundary after the module voltage/current are verified:

```text
MCU GPIO -> gate resistor -> logic-level N-MOSFET or load switch -> laser module
             |                                      |
          pull-down                             actuator rail

MCU ground ------------------------------------ actuator ground
```

- Use a gate pull-down so reset/boot leaves the laser off.
- Put the laser on the independently switched actuator rail.
- Select the MOSFET/load switch only after measuring module current.
- Add local decoupling only after the supply voltage is known.
- No PWM modulation should be used until the seller or a bench test confirms that the internal driver tolerates it.
- Define the output in firmware as `OFF` during reset, fault, emergency stop, loss of control loop, and mode changes.

Suggested future driver contract:

```c
void laser_init(void);        // configures a default-off output
void laser_set(bool enabled); // the only normal control entry point
void laser_force_off(void);   // callable from fault and emergency-stop paths
```

The board-specific GPIO, polarity, timing, and PWM capability must remain configuration data rather than being guessed in application code.

## First-Power And Acceptance Checklist

1. Read the selected SKU and package label; record voltage and polarity in this file.
2. With power disconnected, check whether the metal barrel is connected to either lead.
3. Use a current-limited bench supply at the documented rated voltage; start with the output aimed into a non-reflective beam stop.
4. Record steady current, startup behavior, case temperature after 1/5/10 minutes, and whether focus drifts.
5. Verify GPIO switching through the external MOSFET/load switch, never by wiring the module to an MCU pin.
6. Confirm emergency-stop and reset both force the beam off.

## Safety

An advertised `10 mW` laser must not be treated as a harmless pointer. The actual class cannot be assigned from the screenshot alone.

- Never look into the beam or aim it at people.
- Avoid mirrors, glossy targets, and specular metal during testing.
- Use a fixed matte beam stop and the shortest practical on-time.
- Keep the independent actuator power switch physically accessible.
- Do not rely on the low visibility of 405 nm light as an indication of low optical power.
- Obtain the seller/manufacturer classification and follow the competition venue's laser rules before use.

General safety references (not a substitute for the module datasheet):

- FDA, Laser Products and Instruments: <https://www.fda.gov/radiation-emitting-products/home-business-and-entertainment-products/laser-products-and-instruments>
- IEC 60825-1, Safety of laser products: <https://webstore.iec.ch/en/publication/3587>

## Documentation Status

No trustworthy manufacturer datasheet can be selected from the generic listing title alone. Required next evidence:

- Exact selected SKU from the Taobao order.
- Seller parameter table or manual.
- Clear package/module photos after delivery.
- Bench measurements recorded above.
