# Risk Register

| Risk | Impact | Mitigation |
| --- | --- | --- |
| TI MCU pin count is insufficient | Rework wiring/control board | Confirm sensor, PWM, encoder, UART needs before soldering |
| Third-party MSPM0G3507 board pin map is mistaken for LaunchPad pinout | Wrong pins or damaged board | Obtain the exact board schematic/pin card and create a board-specific pin map |
| Line sensor saturates or has weak contrast | Poor tracking | Add height adjustment, calibration routine, and shield from ambient light |
| Dashed line causes loss of track | Fails basic run | Use prediction from last error and encoder distance during gaps |
| Arc speed too high | Overshoot or oscillation | Speed schedule on curves; reduce base speed before arc |
| Encoder drift affects B/C/D detection | Wrong stop/prompt positions | Combine distance with line feature detection and reset logic at known points |
| Servo power causes MCU brownout | Random resets | Separate servo rail and add current margin/capacitors |
| Gimbal points correctly only in one pose | Static task unreliable | Create calibration routine for target distance and car heading |
| Drawing mechanism drags target or stalls servo | Poor advanced score | Start with non-contact laser drawing demo, then add light pen-lift |
| Four-lap run overheats motors/driver | Advanced failure | Measure current/temperature and reduce speed |
| Two competing drive architectures remain open | Wasted firmware/mechanical effort | Decide between MS42CG stepper path and four-520-motor DC path after fit/current tests |
| Four 520 motors overload a dual TB6612 module | Driver shutdown or damage | Measure each motor start/stall current; do not parallel motor pairs until proven safe |
| 4WD chassis or gimbal exceeds 25 x 15 x 25 cm envelope | Vehicle is ineligible | Measure assembled chassis and gimbal sweep before drilling/mounting |
| 12 V/1500 mAh battery current is insufficient | Brownout, BMS trip, loss of control | Verify BMS rating and test voltage sag against motor/servo peak loads |
| LM2596 module is treated as a guaranteed 3 A supply | Regulator overheating or servo resets | Load-test the exact board and use a separate high-current servo regulator if needed |
| 405 nm 10 mW laser is powered without verified electrical/safety data | Eye hazard or module damage | Verify class, voltage, polarity and driver; use beam stop, independent switch and default-off control |
| Report misses AI usage details | Loses report points | Keep `ai_usage_log.md` updated after each AI-assisted design/debug step |
