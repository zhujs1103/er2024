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
| `b` | Short brake. |
| `x` | Coast stop and return to standby. |
| `?` | Print help and parameters. |

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
Build: 2026-07-03-abcd-fresh-from-fg
```

3. Test `F`.
4. Test `G`.
5. Only then test `A`.

While testing `A`, watch `auto=... autoOdom=... autoCenter=... point=...` in
telemetry. The useful failure points are usually `B_GUARD`, `B_ACQUIRE`,
`D_GUARD`, or `D_ACQUIRE`.
