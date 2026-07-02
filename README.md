# BUPT 2026 Electronics Design Contest - Line Tracking Work Platform

This workspace is organized as the project notebook for the BUPT 2026 electronics design contest car-track problem.

## Directory Map

- `competition_requirements/`
  - Official problem statement, extracted requirements, scoring table, and rendered PDF pages.
- `hardware_docs/`
  - Hardware manuals, datasheets, wiring notes, and module decisions.
- `firmware/`
  - Keil/SysConfig firmware projects and board bring-up source.
- `project_management/`
  - Roadmap, risks, AI usage log, and notes from external conversations.
- `tmp/`
  - Temporary extraction/rendering files. Do not treat this as source-of-truth documentation.

## Current Baseline Direction

Build the first reliable version around:

- TI MCU as the main controller.
- Wheeled differential-drive chassis.
- Non-camera line tracking for driving.
- Encoder/IMU-assisted key-point handling.
- Two-axis servo gimbal or lightweight actuator for target pointing/marking.
- Independent power control for the actuator module.

The first milestone is not maximum score. It is a repeatable 60-point basic-function car that can pass: one-lap tracking, static target action, and A-B-C-D-A linked operation.
