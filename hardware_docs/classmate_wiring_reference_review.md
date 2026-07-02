# Classmate Wiring Reference Review

Status: reviewed from local WeChat reference files on 2026-06-29; use cautiously.
Bench measurements and received-board photos override this reference.

Reference files:

- `D:\weixin_documents\xwechat_files\wxid_78stqy4xzggs22_ad2f\msg\file\2026-06\main.c`
- `D:\weixin_documents\xwechat_files\wxid_78stqy4xzggs22_ad2f\temp\RWTemp\2026-06\7f8a4a42e9bf2fd2691ea73f841e1523\README_接线与SysConfig.md`
- `D:\weixin_documents\xwechat_files\wxid_78stqy4xzggs22_ad2f\temp\RWTemp\2026-06\7f8a4a42e9bf2fd2691ea73f841e1523\说明.md`

## What It Implements

The reference code is a one-lap line-following demo for:

- Tianmengxing MSPM0G3507 board.
- TB6612FNG motor driver.
- 8-channel muxed/selectable digital grayscale sensor.
- Two GM37/520 encoder motors.

Implemented behavior:

- Wait 2 seconds after power-on.
- Follow line with 8-channel grayscale sensor.
- Hold last steering briefly through short dashed/lost-line sections.
- Accumulate left/right encoder distance.
- Stop after an estimated one-lap distance.

Not implemented:

- B/C/D key-point state machine.
- Sound/light prompts.
- Servo gimbal or laser.
- Wheel-speed closed-loop PID.
- Emergency stop and recovery.

## Candidate Pin Map From Reference

This remains a useful candidate pin map because it was written for the same board family and the same main hardware path. Its electrical notes are not fully reliable; verify against received hardware before copying.

| Function | Module signal | MSPM0 pin |
| --- | --- | --- |
| Motor PWM left | TB6612 `PWMA` | `PA21 / TIMA0_C0` |
| Motor PWM right | TB6612 `PWMB` | `PB20 / TIMA0_C1` |
| Left direction 1 | TB6612 `AIN1` | `PA23` |
| Left direction 2 | TB6612 `AIN2` | `PA22` |
| Right direction 1 | TB6612 `BIN1` | `PA14` |
| Right direction 2 | TB6612 `BIN2` | `PA16` |
| Motor standby | TB6612 `STBY` | `PA02` |
| Grayscale address bit 0 | Sensor `AD0` | `PA26` |
| Grayscale address bit 1 | Sensor `AD1` | `PA27` |
| Grayscale address bit 2 | Sensor `AD2` | `PA24` |
| Grayscale output | Sensor `OUT` | `PA25`, direct; received board measured about 3.3 V high |
| Left encoder A | Encoder `LA` | `PB24` |
| Left encoder B | Encoder `LB` | `PB25` |
| Right encoder A | Encoder `RA` | `PB18` |
| Right encoder B | Encoder `RB` | `PB17` |

Existing confirmed bring-up pins remain:

| Function | MSPM0 pin |
| --- | --- |
| Onboard LED | `PB22` |
| UART0 TX | `PA10` |
| UART0 RX | `PA11` |

## SysConfig Requirements

The reference `main.c` assumes SysConfig generates these symbols:

- `PWM_0_INST`
- `PWM_0_CC_0_IDX`
- `PWM_0_CC_1_IDX`

PWM settings used by the code:

- Instance: `PWM_0`.
- Peripheral: `TIMA0`.
- `PA21 = TIMA0_C0` for `PWMA`.
- `PB20 = TIMA0_C1` for `PWMB`.
- 20 kHz PWM.
- Period: `4000` ticks when MCLK is 80 MHz and divider is 1.
- Edge-aligned.
- Output high at counter start, low at compare match.
- Initial compare value: `0`.

GPIO settings:

- Direction outputs: `PA23`, `PA22`, `PA14`, `PA16`, `PA02`.
- Grayscale address outputs: `PA26`, `PA27`, `PA24`.
- Grayscale input: `PA25`, no pull.
- Encoder inputs: `PB24`, `PB25`, `PB18`, `PB17`.
- Encoder inputs need both rising-edge and falling-edge interrupts.
- GPIOB interrupt should route through the generated `GROUP1_IRQHandler` path.

## Electrical Notes To Keep

1. Do not use the reference's grayscale `OUT` divider for the received Yahboom
   `YB-MYX05-V1.0` sensor. Bench measurement with 5 V sensor supply showed
   `OUT` is 3.3 V-safe and the observed raw polarity is white/bright=`0`,
   black/dark=`1`, so `OUT` can connect directly to `PA25`.

2. Grayscale `AD0..AD2` are 3.3 V MCU outputs driving a 5 V sensor board. The reference says short bench wires may work directly, but the formal reliable solution is a 3.3 V to 5 V logic buffer such as `74HCT125` if channel switching is unstable.

3. Encoder VCC should be tried from the MSPM0 board `3V3` first so encoder A/B outputs are MCU-safe. Do not use the TB6612 board's encoder 5 V rail unless A/B are level shifted.

4. TB6612 is suitable for light-load testing only. The reference warns that a GM37/520 stall current around 3.5 A can overheat or trip the driver. First tests must be wheels-up, low PWM, and short duration.

## Firmware Compatibility Risks

The checked-in bring-up project currently only confirms LED/UART and uses a simpler generated config. The classmate `main.c` should not be dropped into that project until SysConfig is rebuilt to match the pin map above.

Specific mismatches to resolve:

- The reference code assumes `SysTick_Config(80000000U / 1000U)`.
- The current bring-up generated header records `CPUCLK_FREQ` as 32 MHz.
- `GRAY_SETTLE_CYCLES` is written for about 10 us at 80 MHz.
- PWM period `4000` ticks gives 20 kHz only under the expected timer clock.
- Encoder interrupt handler names and group routing must match the generated SysConfig output.

Therefore choose one of these paths before compiling:

1. Set the board clock/timer clock to match the reference 80 MHz assumptions.
2. Keep the current 32 MHz clock and adjust SysTick, PWM period, and grayscale settle cycles.

## Useful Constants From Reference

| Constant | Reference value | Action |
| --- | ---: | --- |
| Encoder counts per wheel revolution | `1320.0` | Treat as starting estimate, then measure by hand-turning 10 wheel revolutions. |
| Wheel diameter | `0.065 m` | Replace with measured wheel diameter. |
| Lap distance | `4.50 m` | Calibrate on the actual contest track. |
| Control period | `5 ms` | Reasonable starting point. |
| Base PWM | `300 / 1000` | Start lower, for example 150, during wheels-up checks. |
| Max PWM | `620 / 1000` | Keep conservative until current/heat are known. |

## Recommended Adoption

Use this reference as the first concrete wiring/SysConfig target for the 2WD baseline:

1. Wire power and common ground exactly as in `assembly_wiring_plan.md`.
2. Use the candidate pin map above for TB6612, grayscale, and two encoders.
3. Connect grayscale `OUT` directly to `PA25`; no divider is needed for the measured sensor.
4. Power encoders from `3V3` first.
5. Rebuild SysConfig for the full pin map.
6. Bring up in this order: PWM disabled, one motor, one encoder, two motors, grayscale readout, low-speed line following.

Do not add gimbal, spotlight, laser, or 4WD until the 2WD baseline is stable.
