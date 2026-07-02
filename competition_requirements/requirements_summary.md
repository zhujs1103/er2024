# Requirements Summary

## Problem

Design a "line-tracking and work-operation integrated platform". The device consists of:

- A wheeled vehicle chassis.
- A line-tracking control module.
- A gimbal, light robotic arm, or equivalent work module.

The car must automatically follow the specified black-line track and perform target-pointing or marking actions at required positions.

## Field And Track

- Field size: at least `220 cm x 120 cm`.
- Field surface: white matte planar material.
- Track: black line, closed or non-closed, consisting of two straight/dashed segments and two symmetric semicircular arcs.
- Line width: `1.8 cm ± 0.2 cm`.
- Key points: `A`, `B`, `C`, `D`.
- Diagram reference:
  - Overall reference field shown as `220 cm x 120 cm`.
  - `AB` reference length shown as `100 cm`.
  - Right semicircle radius shown as `40 cm`.
  - `B-C` vertical distance shown as `60 cm`.

## Target

- Target board is placed `50 cm` outside the `AB` segment.
- Target face is parallel to `AB`.
- Target height: no more than `50 cm`.
- Target material: A4 record sheet.
- Target center has an obvious center point and concentric circles for judging.

## Hard Constraints

- Overall vehicle size: no more than `25 cm x 15 cm x 25 cm`.
- Chassis must be wheeled.
- No tracks, no mecanum wheels.
- No camera may be used for vehicle path recognition.
- The main controller must be a TI chip.
- A camera or K230 vision module may be used only as end-effector auxiliary positioning, not for vehicle line tracking or required target operation.
- The car must complete path recognition and target operation without a camera.
- No human interference while driving.
- No external navigation device.
- Actuator/gimbal module needs independent power control and basic safety.

## Basic Requirements

1. Automatic line tracking
   - Start from `A`.
   - Follow the track for one lap and stop at `A`.
   - Give sound/light indication when stopped.
   - Time limit: no more than `30 s`.

2. Static target pointing
   - Place the car at a specified point on the track, with arbitrary initial pose.
   - After start, automatically adjust gimbal/arm pose within `5 s`.
   - End effector points to the target center and performs one required pointing action.

3. Linked line tracking and target operation
   - Start from `A`.
   - Track to `B`, stop, and complete one target operation while stationary.
   - Continue through `C`, `D`, and back to `A`.
   - Give sound/light indication at every key point.
   - Total one-lap time limit: no more than `40 s`.

4. Engineering training item
   - Chassis, gimbal/arm, and human-machine interface modules should support independent power-up and standalone testing.
   - Modes and parameters should be settable by buttons or serial port.

## Advanced Requirements

1. Dynamic pointing
   - Start from `A`.
   - While driving along the track, keep the gimbal/arm continuously following the target.
   - Short deviations during turns are allowed, but the system should not lose effective working capability.

2. Synchronous drawing
   - With the car stationary, draw a specified pattern on the target surface.
   - Example patterns: simple geometry or letters.
   - Pattern should be clear and match the preset design.

3. Four-lap continuous operation
   - Run four continuous laps and complete one full pointing task.
   - Shorter time is better after meeting functional requirements.

4. Engineering completeness
   - Wiring quality.
   - Structural strength.
   - Maintainability.
   - Power isolation.
   - Emergency stop logic.
   - Fault recovery.

## Submission Notes

All review materials are submitted online. Prepare clear videos covering:

- Overall appearance.
- Automatic line tracking.
- Static target operation.
- Linked track-to-target operation.
- Dynamic following, if implemented.
- Synchronous drawing, if implemented.
- Four-lap continuous operation, if implemented.
- Engineering completeness details.

The report should include an AI usage section describing tools, usage method, benefits, and limitations.

