# Power And Safety

Store battery, regulator, switch, fuse, and emergency-stop documents here.

Required by problem statement:

- Actuator/gimbal module should have independent power control.
- Basic safety requirements must be met.

Recommended structure:

- Logic rail for MCU and sensors.
- Motor rail for drive motors.
- Servo/actuator rail with independent switch.
- Common ground designed deliberately.
- Emergency stop cuts motor/servo power while keeping debug power if possible.

