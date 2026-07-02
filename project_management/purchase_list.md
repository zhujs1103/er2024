# Purchase List

Date: 2026-06-16

Goal: buy enough parts to build and debug the first reliable version of the line-tracking work-operation car, then keep a path open for dynamic pointing and drawing.

> This file is the purchase plan. Actual purchases and their evidence are tracked in `../hardware_docs/purchased_hardware_registry.md`. Five screenshots supplied on 2026-06-21 now cover all 17 shared-cart lines. Firmware-facing implications are in `../hardware_docs/firmware_integration_matrix.md`.

## Buy First - Core Build

| Priority | Item | Qty | Recommended Spec / Search Keyword | Why |
| --- | ---: | ---: | --- | --- |
| P0 | TI main controller board | 1-2 | `LP-MSPM0G3507 LaunchPad`, `MSPM0G3507 开发板` | Main controller must be TI. Buy 2 if budget allows; one for car, one for experiments/spare. |
| P0 | 2WD wheeled chassis with encoder motors | 1 | `N20 编码器电机 小车底盘 2WD`, must fit within `25 cm x 15 cm` | Differential drive is simplest and legal. Encoders are important for key points. |
| P0 | Spare N20 encoder motors | 2 | Same voltage/reduction ratio as chassis motors, preferably `6V 100-300RPM` | Motors are easy to damage during tuning. |
| P0 | Rubber wheels | 2-4 | Match motor shaft and chassis, around `42-48 mm` diameter | Need stable grip; keep spares. |
| P0 | Ball caster / universal wheel | 1-2 | Low-friction front/rear caster | Differential 2WD needs a support point. |
| P0 | Line sensor array | 2 | Prefer `8路 灰度传感器 模拟输出`, or `QTR-8A`; backup option `QTR-8RC` | The car must track line without camera. Buy 2 for spare and comparison. |
| P0 | Dual DC motor driver | 2 | Prefer `DRV8833 电机驱动模块`; backup `TB6612FNG 双路电机驱动模块` | Drives two brushed DC motors. One spare is worth it. |
| P0 | 2-axis pan-tilt bracket | 1 | `二自由度云台 舵机云台` | Fastest structure for target pointing. |
| P0 | Metal gear micro servos | 4 | `MG90S 金属齿轮舵机`, `4.8-6V`, buy real-ish ones if possible | Two for gimbal, one for pen lift, one spare. |
| P0 | Red laser dot module | 2 | `5V 红光 激光头 模块`, low-power visible dot | Easiest first target-pointing end effector. |
| P0 | Active buzzer | 2 | `有源蜂鸣器 3.3V/5V` | Required for sound prompt at stop/key points. |
| P0 | LEDs + resistors | 1 set | `LED 发光二极管 电阻包 220R/330R` | Required for light prompt and debug states. |
| P0 | Push buttons | 5-10 | `轻触按键 6x6`, or panel buttons | Mode selection, calibration, start/stop. |
| P0 | USB-TTL serial module | 1-2 | `CH340 USB转TTL 3.3V/5V` | Parameter tuning and logs. |

## Buy First - Power And Safety

| Priority | Item | Qty | Recommended Spec / Search Keyword | Why |
| --- | ---: | ---: | --- | --- |
| P0 | Battery | 2 | Option A: `2S 7.4V LiPo 1000-1500mAh 25C`; Option B: `2S 18650 电池盒/电池组 带保护板` | Need enough current for motors and servos. Buy two packs for testing. |
| P0 | Matching charger | 1 | Match chosen battery: `2S LiPo 平衡充` or `2S 锂电池充电器` | Safe charging. |
| P0 | 5V buck regulator | 2-3 | `5V 3A 降压模块`, MP1584/LM2596 class | Servo and sensor rail. Keep spare. |
| P0 | Power switch | 2-3 | `船型开关`, `拨动开关`, current above expected load | Separate main power and actuator power. |
| P0 | Emergency stop switch | 1 | `自锁急停开关 小型` | Engineering completeness and safety. |
| P0 | Fuse / resettable PTC | 3-5 | `自恢复保险丝 1A 2A`, or inline fuse holder | Protect battery and wiring. |
| P0 | Electrolytic capacitors | 1 set | `470uF 1000uF 16V 低ESR` | Suppress motor/servo voltage dips. |
| P0 | Connectors | 1 set | `XT30`, `JST`, `杜邦线`, `XH2.54` | Reliable power and module wiring. |
| P0 | Silicone wires | 1 set | `22AWG`, `24AWG`, red/black | Motor/power wiring. |

## Buy Second - Advanced / Better Debug

| Priority | Item | Qty | Recommended Spec / Search Keyword | Why |
| --- | ---: | ---: | --- | --- |
| P1 | IMU module | 1-2 | `MPU6050 模块`, better: `ICM42688 模块` | Helps arc stability and yaw estimation. Not mandatory for first run. |
| P1 | OLED display | 1 | `0.96寸 OLED I2C SSD1306` | Shows mode, battery, sensor values, PID state. |
| P1 | Small marker/pen mechanism parts | 1 set | `水性笔`, `舵机 笔架`, springs, rubber band | For synchronous drawing and marking. |
| P1 | Stronger servo | 1-2 | `20g/25g 金属齿轮数字舵机`, torque above `3kg.cm` | Use if pen pressure is too high for MG90S. |
| P1 | Bluetooth serial module | 1 | `HC-05`, `BLE 串口模块` | Wireless tuning during track tests. Optional. |
| P1 | VL53L0X/VL53L1X ToF sensor | 1 | `VL53L0X 激光测距模块` | Optional distance check for target/gimbal calibration. Do not rely on it for line navigation. |
| P2 | K230/camera module | 0-1 | Buy only later | Allowed only as end-effector auxiliary positioning, not for required driving/path recognition. Skip for first build. |

## Field And Submission Materials

| Priority | Item | Qty | Recommended Spec / Search Keyword | Why |
| --- | ---: | ---: | --- | --- |
| P0 | White matte field surface | Enough for `220 cm x 120 cm` | `白色哑光 PVC板`, `白色KT板`, or white matte roll material | Need test field close to problem statement. |
| P0 | Black track tape | 2-3 rolls | `18mm 黑色 电工胶带`, `18mm 黑色美纹纸胶带`, matte preferred | Problem line width is `1.8 cm ± 0.2 cm`. |
| P0 | A4 target board | 5-10 sheets | A4 paper/card + back board | Target face and repeated tests. |
| P0 | Target stand | 1 | `A4展示架`, DIY foam board stand, height <= `50 cm` | Target must stand 50 cm outside AB and parallel to AB. |
| P1 | Printed calibration target | Several | A4 concentric circles + center dot | For gimbal calibration and scoring evidence. |

## Prototyping Tools And Consumables

| Priority | Item | Qty | Recommended Spec / Search Keyword | Why |
| --- | ---: | ---: | --- | --- |
| P0 | Perfboard / prototype board | 3-5 | `洞洞板`, `万用板` | Mount connectors, buttons, power distribution. |
| P0 | Header pins and sockets | 1 set | `排针 排母 2.54mm` | Module connections. |
| P0 | Heat shrink tube | 1 set | Mixed sizes | Wiring protection. |
| P0 | Zip ties / Velcro / double-sided tape | 1 set | Small size | Cable management and quick mounting. |
| P0 | M2/M3 screw kit and standoffs | 1 set | `M2 M3 螺丝 铜柱 尼龙柱` | Mechanical assembly. |
| P0 | Solder and flux | 1 set | If not already available | Assembly and repair. |
| P0 | Multimeter | 1 | If not already available | Required for power debugging. |

## Do Not Buy For Main Solution

- Arduino/STM32/ESP32 as the main controller. The problem requires a TI main controller.
- Mecanum wheels or tracked chassis. Both violate the problem statement.
- Camera-based line tracking. The car must complete path recognition without a camera.
- Oversized 4WD chassis wider than `15 cm`.
- High-power laser. Use only low-power visible dot modules for aiming tests.

## My Recommended First Order

If you want the shortest shopping list that can start work:

1. `LP-MSPM0G3507 LaunchPad` x 1 or 2.
2. `N20 编码器电机 2WD 小车底盘` x 1.
3. Matching spare `N20 编码器电机` x 2.
4. `8路灰度传感器 模拟输出` x 2.
5. `DRV8833 电机驱动模块` x 2.
6. `MG90S 金属齿轮舵机` x 4.
7. `二自由度舵机云台` x 1.
8. `5V 红光激光模块` x 2.
9. `2S 7.4V 电池 + 充电器` x 1 set, extra battery x 1.
10. `5V 3A 降压模块` x 3.
11. Buttons, buzzer, LEDs, USB-TTL, wires, connectors, switches, emergency stop.
12. Field materials: white matte base, 18 mm black tape, A4 target stand.

## Reference Specs Checked

- TI LP-MSPM0G3507 is the LaunchPad development kit for the 80-MHz Arm Cortex-M0+ MSPM0G3507 MCU and includes onboard debug, buttons, LEDs, light/temperature sensors.
- MSPM0G3507 has up to 128KB flash, 32KB SRAM, 80MHz Arm Cortex-M0+, and 1.62V to 3.6V MCU supply range.
- DRV8833 is a dual H-bridge driver for two brushed DC motors or one stepper; motor supply range is 2.7V to 10.8V and output current is up to 1.5A RMS per bridge in suitable packages.
- Pololu QTR-8A/QTR-8RC style arrays have 8 IR reflectance channels, operate at 3.3V to 5V, and are designed for line-following robots.
