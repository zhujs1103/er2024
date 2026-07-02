# Purchased Two-Axis Servo Gimbal

Inventory ID: `HW-20260621-08`

- Listing: `二维电动舵机云台送图纸源码TI杯`.
- Visible option starts `简易二维云台（15kgPWM...`.
- Product image shows the bracket kit and two 15 kg-class PWM servos.
- Quantity 1 set; displayed price CNY 113.00.

## Required Seller Data

- Exact servo model and quantity included.
- Supply range, idle/running/stall current.
- PWM period and safe pulse-width range.
- Neutral pulse, direction and achievable angle.
- Bracket assembly drawing, hard stops and payload limits.
- TI/MSPM0 example source and its pulse assumptions.

## Integration Rules

- Power servos from a separate high-current actuator rail with common signal ground.
- Do not power them from the MSPM0 board or the unverified D36A 5 V auxiliary output.
- Characterize each axis without the laser fitted, then set conservative software limits.
- Start PWM at a verified neutral only after actuator power is stable.
- Store calibration as per-axis pulse/angle data; do not assume both servos match.
