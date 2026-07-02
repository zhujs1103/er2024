# GY-521 / MPU-6050 IMU Notes

Source archive inspected:

```text
D:/weixin_documents/xwechat_files/wxid_78stqy4xzggs22_ad2f/temp/RWTemp/2026-07/7ffceeef92133a76f15808f7d36e72ea/GY521mpu-6050资料-.rar
```

## Contents

- `RM-MPU-6000A.pdf`: register map.
- `PS-MPU-6000A.pdf`: product specification.
- `MPU6050-V1-SCH.jpg`: GY-521 module schematic.
- `MPU6050-V1.jpg`: module outline and pin order.
- Arduino and 8051 example programs.

## Module Pins

Pin order from the board image:

```text
VCC
GND
SCL
SDA
XDA
XCL
AD0
INT
```

Only `VCC`, `GND`, `SCL`, and `SDA` are needed for first bring-up.

## Electrical Notes

- The GY-521 board schematic shows a 5 V input feeding a 3.3 V LDO.
- The MPU-6050 itself runs at 3.3 V on this module.
- SDA and SCL have 4.7 k pull-ups to `VCC_3.3V`, not to 5 V.
- `AD0` has a 4.7 k pull-down, so the default I2C address is `0x68`.
- If `AD0` is tied high, the address becomes `0x69`.

This is compatible with the MSPM0 3.3 V I2C pins. Do not add external pull-ups
to 5 V on SDA or SCL.

## Useful Registers

From the included Arduino `MPU6050.h` and register map:

| Register | Address | Use |
| --- | --- | --- |
| `SMPLRT_DIV` | `0x19` | Sample-rate divider |
| `CONFIG` | `0x1A` | DLPF / filtering |
| `GYRO_CONFIG` | `0x1B` | Gyro range |
| `ACCEL_CONFIG` | `0x1C` | Accel range |
| `ACCEL_XOUT_H` | `0x3B` | Start of 14-byte accel/temp/gyro data block |
| `GYRO_ZOUT_H` | `0x47` | Z-axis gyro high byte |
| `PWR_MGMT_1` | `0x6B` | Wake/sleep and clock source |
| `WHO_AM_I` | `0x75` | Device identity check |

Minimum first bring-up sequence:

1. I2C read `WHO_AM_I` at address `0x68`.
2. Write `PWR_MGMT_1 = 0x00` or select a gyro clock source.
3. Read 14 bytes from `0x3B`.
4. Print raw `gyroZ` first. Do not use it for control until bias is measured.

## How It Can Help This Car

The current car already has encoder feedback for white-gap straight driving.
The IMU can add heading information:

- White gap: hold yaw angle while encoder balancing keeps wheel speeds matched.
- Black arc: estimate turn progress and avoid relying only on line loss near C/D/A.
- Recovery: detect unexpected spin if one wheel stalls.

The first useful signal is yaw rate around the vertical axis, usually `gyroZ`.
Because gyro integration drifts, use it as a short-term correction, not as the
only long-term position source.

## Recommended Project Plan

1. Do not integrate IMU into `A` automatic mode yet.
2. Add SysConfig I2C pins on currently unused MSPM0 pins.
3. Bring up a standalone UART command such as `I`:
   - initialize MPU-6050;
   - print `who`, raw accel, raw gyro, and `gyroZ` bias.
4. With the car stationary for 2-3 seconds, average `gyroZ` to estimate bias.
5. Only after stable raw readings are confirmed, add yaw-hold correction to the
   white-gap straight controller.

## Wiring To Decide

Current firmware does not configure I2C. Choose an MSPM0 I2C SCL/SDA pair in
SysConfig before wiring the module.

Temporary IMU wiring once pins are chosen:

```text
GY-521 VCC -> MCU 3.3 V or 5 V supply available on the board
GY-521 GND -> common GND
GY-521 SCL -> selected MSPM0 I2C SCL
GY-521 SDA -> selected MSPM0 I2C SDA
GY-521 AD0 -> leave open for address 0x68
GY-521 INT -> leave open for first bring-up
```

Prefer powering it from 3.3 V if convenient. The module schematic accepts 5 V on
`VCC`, but keeping sensor power at 3.3 V reduces mixed-power surprises.
