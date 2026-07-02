# AUTO State Machine Rewrite Handoff

Date: 2026-07-02

This note captures the useful field-verified facts before rewriting the A/B/C/D
automatic state machine.

## Official Track Labels

- `A`: upper-left black/white boundary
- `B`: upper-right black/white boundary
- `C`: lower-right black/white boundary
- `D`: lower-left black/white boundary
- Route: `A -> B -> C -> D -> A`
- `AB` and `CD` are white/dashed straight segments.
- `BC` and `DA` are black semicircular arcs.
- With the official labels, both `BC` and `DA` enter a right-turn arc from the
  car's frame, but the next rewrite should avoid fixed "turn to find line"
  behavior unless it is deliberately reintroduced after testing.

## Proven Baseline

- Step 6 continuous line PID is the trusted black-line-following baseline.
- Current useful PID values:
  - `base=180`
  - `Kp=55`
  - `Ki=0`
  - `Kd=35`
- F-mode white-gap straight control is the trusted blank-area baseline.
- Current useful white-gap values:
  - `base=180`
  - `gapBias=15`
  - `gapDL/gapDR` typically around `5..6` per status sample
- Measured AB/CD white-gap distance is consistently around `1558..1588` ticks.
- Field examples:
  - `gapOdom=1573`, final `encL=1583 encR=1585`
  - `gapOdom=1576`, final `encL=1582 encR=1591`
  - `gapOdom=1558`, final `encL=1568 encR=1569`
- Wheel diameter is `65 mm`; circumference is about `204.2 mm`.
- Manual encoder check:
  - left wheel forward 10 turns: about `-3350` raw before logical transform
  - right wheel forward 10 turns: about `3301` raw before logical transform

## Current AUTO Failure Summary

Do not treat current `A` mode as reliable.

Last tested build: `2026-07-02-auto-direct-pid`.

Observed field behavior:

1. AB white straight works.
2. B black line is detected.
3. Firmware prints:

```text
[AUTO] transition to=BC code=2 from=AB segOdom=1573
```

4. Immediately after entering BC, the car stops.
5. Telemetry shows a valid B-line reading but broken AUTO state:

```text
mode=STBY line=0b00000011 st=OK n=2 err=-300 corr=0 base=180 L=0 R=0 ... auto=BAD seg=BAD segOdom=16686 autoReady=0
```

Conclusion: the failure is not AB straight control and not line detection at
B. The failure is in the current AUTO transition/state implementation after
AB->BC.

Earlier AUTO failures worth remembering:

- Too-strict BC arc exit caused `arc lost early at segOdom=1888`.
- A later run reached BC exit at `segOdom=2004`, inside the useful BC exit
  window.
- A previous implementation stopped around C because CD straight startup reused
  old odometry after an encoder reset.
- Attempts to add shadow recovery or fixed arc-acquire turns made behavior more
  confusing and should not be carried forward.

## Rewrite Plan Recommendation

Start from stable low-level functions, not from the current patched AUTO logic.

Keep:

- line sensor mux read and weighted error
- motor logical left/right output and orientation flags
- encoder logical transform
- Step 6 line PID behavior
- F-mode straight balancing math
- UART command shell only as a thin control/debug layer

Rewrite:

- all `A` mode state variables
- all A/B/C/D transition code
- all segment odometry handling
- all event logging around segment transitions

Recommended state machine:

```text
AUTO_IDLE
AUTO_AB_WHITE
AUTO_BC_LINE
AUTO_CD_WHITE
AUTO_DA_LINE
AUTO_DONE
AUTO_FAIL
```

Recommended implementation rules:

- Use one dedicated `auto_state_t` struct for all AUTO variables.
- Use monotonic odometry and `segmentStartOdom`; do not reset encoder counters
  inside every segment transition.
- Keep F-mode globals separate from AUTO globals. Do not reuse `g_gapTestState`
  or `g_gapBlackTicks` inside AUTO.
- In AB/CD:
  - drive straight using the proven encoder balancing
  - ignore black before about `1200 ticks`
  - once black is detected after the minimum window, enter line-following state
  - if no black by about `1900 ticks`, brake and report a fail
- In BC/DA:
  - use normal line PID whenever the line is visible
  - tolerate short `LOST` gaps using the last PID correction
  - transition out after confirmed white/lost inside a measured odometry window
- Avoid fixed turn-to-find-line behavior unless a later field test specifically
  asks for it.
- Keep UART logging short. Never let long transition logs run before the new
  motor command for the segment has been issued.

## First Tests After Rewrite

1. Test `F` again to confirm blank straight still gives around `1570..1590`
   ticks.
2. Test `G` on a black arc to confirm plain PID still tracks.
3. Test new `A` only through AB->BC:
   - expected AB odom at transition: about `1550..1600`
   - expected B line reading: active count around `1..2`, error may be near
     `-300` depending on entry offset
   - after transition, mode must remain AUTO and motor commands must not be
     `L=0 R=0`
4. Only after B entry is stable, continue to C/D/A thresholds.
