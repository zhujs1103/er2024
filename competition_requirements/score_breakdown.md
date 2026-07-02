# Score Breakdown And Strategy

## Scoring Table

| Item | Points |
| --- | ---: |
| Automatic line tracking and stop | 20 |
| Static target operation | 20 |
| Linked line tracking to target position | 20 |
| Dynamic following | 15 |
| Synchronous drawing | 15 |
| Four-lap continuous running | 10 |
| Engineering completeness | 10 |
| Design report AI usage statement | 10 |

## Strategy

1. Secure the 60-point basic-function base first:
   - One-lap tracking and stop.
   - Static target pointing.
   - A-to-B stop and work, then C-D-A indication.

2. Build engineering score into the car from the beginning:
   - Clear wiring.
   - Separate actuator power control.
   - Debug ports.
   - Emergency stop.
   - Parameter storage and quick calibration.

3. Attempt advanced items only after the basic path is repeatable:
   - Dynamic pointing can reuse a geometric target-angle model.
   - Drawing can reuse gimbal inverse mapping after target calibration.
   - Four-lap continuous running depends mainly on thermal, battery, and drift stability.

