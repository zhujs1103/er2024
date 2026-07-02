# Development Environment

Status date: 2026-06-27

This project targets a TI MSPM0G3507-class controller with Keil MDK and TI SysConfig.

## Installed And Verified

| Component | Path | Status |
| --- | --- | --- |
| Keil MDK/uVision 5.32 | `D:\Keil_v5\UV4\UV4.exe` | Installed |
| Arm Compiler 6.14.1 | `D:\Keil_v5\ARM\ARMCLANG\bin\armclang.exe` | Installed |
| Arm Compiler 5.06u7 | `D:\Keil_v5\ARM\ARMCC\bin\armcc.exe` | Installed |
| TI SysConfig 1.28.0+4712 | `D:\ti\sysconfig_1.28.0\sysconfig_cli.bat` | Installed 2026-06-27 |
| MSPM0 SDK 2.10.00.04 | `D:\ti\mspm0_sdk_2_10_00_04` | Installed and verified 2026-06-27 |
| MSPM0G1X0X/G3X0X Keil DFP 1.3.1 | `D:\Users\steven\AppData\Local\Arm\Packs\TexasInstruments\MSPM0G1X0X_G3X0X_DFP\1.3.1` | Installed 2026-06-27 |
| Git | `D:\Git\cmd\git.exe` | Installed |
| Python | `C:\ProgramData\anaconda3\python.exe` | Installed |

## Still Missing

No required Keil-first MSPM0G3507 development components are currently missing.

## Project Environment Scripts

Activate the local tool paths in a PowerShell session:

```powershell
.\tools\env\er2024-dev-env.ps1
```

Run the environment check:

```powershell
.\tools\env\check-dev-env.ps1
```

If PowerShell script execution is disabled on this machine, use the wrappers:

```bat
tools\env\check-dev-env.cmd
tools\env\dev-shell.cmd
```

The activation script sets:

- `ER2024_WORKSPACE`
- `ER2024_KEIL_ROOT`
- `ER2024_SYSCONFIG_ROOT`
- `MSPM0_SDK_ROOT`

It also prepends Keil, Arm compiler and SysConfig paths to the current shell `PATH`. It does not modify the global Windows user PATH.

## Notes

- `cmake`, `ninja`, `make`, `arm-none-eabi-gcc` and `tiarmclang` are not currently on PATH. They are not required for the Keil-first workflow, but may be useful later for CLI builds or CCS/Theia projects.
- The senior reference projects include local copies of TI DriverLib/CMSIS sources. That is enough for reading and partial reuse, but our real firmware project should use the official installed MSPM0 SDK as source of truth.
- The reference `tools/keil/syscfg.bat` expects old SysConfig path `C:\ti\sysconfig_1.20.0`; update it to `D:\ti\sysconfig_1.28.0` when creating or importing a firmware project.
