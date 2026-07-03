# ER2024 ABCD Fresh Firmware

This project is a clean ABCD experiment branch.

## Source Policy

- `main.c` starts from the older F/G snapshot at
  `tmp/zip1422/er2024/firmware/bringup/main.c`.
- The current `firmware/bringup/main.c` control logic is not copied here.
- SysConfig output, startup, linker, and Keil project shell are reused only for
  the confirmed board pins and build setup.

## Build

Keil project:

```text
firmware/abcd_fresh/keil/abcd_fresh_mspm0g3507_keil.uvprojx
```

Command-line rebuild:

```bat
D:\Keil_v5\UV4\uVision.com -r D:\steven\work\er2024\firmware\abcd_fresh\keil\abcd_fresh_mspm0g3507_keil.uvprojx -o D:\steven\work\er2024\firmware\abcd_fresh\keil\build.log
```

Output hex:

```text
firmware/abcd_fresh/keil/Objects/abcd_fresh_mspm0g3507_keil.hex
```

## Runtime Commands

| Command | Effect |
| --- | --- |
| `A` / `a` | Start the ABCD fresh state machine. |
| `F` | Run the old white-to-black straight test. |
| `G` | Run the old continuous black-line PID test. |
| `g` | Run the old Step7 encoder-distance lap stop. |
| `I` | Initialize MPU6050 on PB2/PB3, keep the car still for gyroZ bias calibration, then print IMU state. |
| `i` | Read one MPU6050 sample after `I` has succeeded, then print IMU state. |
| `Y` / `y` | Run an isolated IMU-assisted straight test after `I`; send `x` to stop. |
| `b` | Short brake. |
| `x` | Coast stop and return to standby. |
| `?` | Print help and parameters. |

## IMU First Test

Wiring for the first MPU6050 / GY-521 bring-up:

```text
GY-521 VCC -> 3V3
GY-521 GND -> GND
GY-521 SCL -> PB2
GY-521 SDA -> PB3
GY-521 AD0 -> open or GND
```

Test sequence:

1. Keep the wheels off the ground or leave motor power disconnected.
2. Flash this project and open UART0 at 9600-8N1.
3. Confirm the startup line includes:

```text
Build: 2026-07-03-abcd-fresh-y-enc-imu
```

4. Send uppercase `I`. Keep the car still for about 2 seconds.
5. A good first result should show `init=OK`, `ready=1`, and usually `who=104`
   for MPU6050 address `0x68`.
6. Send lowercase `i` while gently rotating the car by hand. `yawRate100` and
   `rawZ` should change sign between left and right rotation.

`Y` is the first motion test for the straight controller with IMU correction.
It reuses the proven F-mode encoder balancing with `gapBias=15`, then adds the
measured gyroZ correction. It is still isolated from `F`, `G`, and `A`.

## ABCD State Machine

The auto mode deliberately keeps the logic small:

1. `AB_STRAIGHT`: use the old `F` white-to-black straight behavior.
2. `B_GUARD`: keep driving straight for a short encoder guard distance.
3. `B_ACQUIRE`: keep straight until the center sensor group sees the black line.
4. `BC_LINE`: use the old `G` line PID on the arc.
5. `CD_STRAIGHT`: use the old `F` straight behavior again.
6. `D_GUARD`: same transition guard as `B_GUARD`.
7. `D_ACQUIRE`: same center acquisition as `B_ACQUIRE`.
8. `DA_LINE`: use the old `G` line PID and stop after confirmed line loss.

The guard/acquire split prevents the first edge sensor hit from being fed
directly into line PID.

## First Test Order

1. Flash this project's hex, not the old bringup hex.
2. Confirm the startup line includes:

```text
Build: 2026-07-03-abcd-fresh-y-enc-imu
```

3. Test `I` / `i` if the MPU6050 is connected.
4. Test `Y` on a clear white floor, with one hand ready to send `x`.
5. Test `F`.
6. Test `G`.
7. Only then test `A`.

While testing `A`, watch `auto=... autoOdom=... autoCenter=... point=...` in
telemetry. The useful failure points are usually `B_GUARD`, `B_ACQUIRE`,
`D_GUARD`, or `D_ACQUIRE`.
