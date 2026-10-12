# Cascade Robot: VEXcode Python handout

This folder is the cascade robot's programs in **VEXcode V5 Python**, written
so they can be opened, edited and downloaded straight from VEXcode. There are
two:

1. **Drive live angles** (`Cascade Robot Drive Limits`) is the driver program
   in use. It shows a live angle for every motor, keeps the cascade inside its
   measured limits, has a one-press claw and an exponential stick curve. See
   [Drive live angles](#drive-live-angles).
2. **Override Auton** (`Override Auton`) is the match program for V5RC
   Override: the autonomous, plus the same driver control as Drive Limits. See
   [Override Auton](#override-auton) and its own handout,
   [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md).

The older programs were removed on 2026-10-10 and are still in the git history:
- drive v2, the amps measuring tool and their notes (`CONTROL_FEEL.md`,
  `TESTING_SAFETY.md`): `git show f863b31:vexcode-python/<file>`;
- the old PID match program, drive v1 and the motor test:
  `git show 4bde9ee:vexcode-python/<file>` (drive v1 is also the tag
  `v1-drive-working-40`).

> **Status:** drive live angles has been driven on the robot. The Override Auton
> has only been tested on a computer simulation: measure its settings first
> (see [Measure these first](OVERRIDE_AUTON.md#measure-these-first)).

## Contents

- [Files](#files)
- [Putting it on the robot](#putting-it-on-the-robot)
- [Controls](#controls)
- [Ports](#ports)
- [Drive live angles](#drive-live-angles)
- [Override Auton](#override-auton) - the full handout is [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md)
- [Common edits](#common-edits)
  - [Change a port or flip a motor](#change-a-port-or-flip-a-motor)
  - [Change driver speeds or buttons](#change-driver-speeds-or-buttons)
- [Keeping the .py and .v5python the same](#keeping-the-py-and-v5python-the-same)
- [Rules for editing VEXcode Python](#rules-for-editing-vexcode-python)
- [Before the first real run](#before-the-first-real-run)
- [Troubleshooting](#troubleshooting)
- [Change log](#change-log)

## Files

| File | What it is |
|---|---|
| `Cascade Robot Drive Limits.v5python` | **Drive live angles, the driver program in use.** Open this in VEXcode. A live angle for every motor, cascade limits, toggle on Right / Y, one press of R1 / R2 turns the claw 180° open / closed. |
| `cascade_robot_drive_limits.py` | Drive live angles as a plain Python file, so it can be read and reviewed on GitHub. |
| `Override Auton.v5python` | **Override Auton, the match program.** Open this in VEXcode. The autonomous plus Drive Limits' driver control. |
| `override_auton.py` | The Override Auton as a plain Python file. |
| `sync_files.py` | Copies changes between each `.py` and its `.v5python` (runs on your computer, not the robot). |
| `README.md` | This handout. |
| `OVERRIDE_AUTON.md` | The autonomous handout: field setup, the route, what to measure, how to start it. |

## Putting it on the robot

1. Open **VEXcode V5**, then **File → Open** and pick a program:
   - `Cascade Robot Drive Limits.v5python` — **drive live angles**, for driver
     practice. Nothing to calibrate.
   - `Override Auton.v5python` — for matches and autonomous tests. **Plain Run
     starts driver control, not the autonomous**; see
     [Running it](OVERRIDE_AUTON.md#running-it).
2. Plug in the brain (or the controller, with the brain paired), pick a slot and press **Download**.
   Put each program in a different slot, so both are on the brain.
3. Before pressing **Run**, put the **cascade all the way down.** Both programs
   count "0 degrees" from wherever the lift is at startup. For the Override
   Auton, also close the claw on the preload.

## Controls

Both programs use these controls:

| Control | Does |
|---|---|
| Left stick up/down | Drive forward / back (top speed only at the end of the stick) |
| Right stick left/right | Turn |
| L1 / L2 | Cascade up / down (holds its spot when you let go; stops at 0 and 730°) |
| R1 / R2 | One press opens / closes the claw by 180° |
| Right / Y | Toggle one way / the other way |

The Override Auton adds two, off the field only: **hold A for 1 second** to run
the autonomous where the robot stands, and **B** stops it.

## Ports

Front, back, left and right are as the robot drives forward.

| Port | Part | Motor | Reversed |
|---|---|---|---|
| 11 | Drive left front | 11W | yes |
| 17 | Drive left back | 11W | yes |
| 1 | Drive right front | 11W | no |
| 10 | Drive right back | 11W | no |
| 13 | Cascade left | 11W | no |
| 2 | Cascade right | 11W | yes |
| 18 | Toggle | 5.5W | no |
| 8 | Toggle | 5.5W | yes |
| 16 | Claw (on the long 1500 mm cable) | — | no |

- The "Reversed" column was measured on the real robot with the motor test program (since removed), in **MATCH** direction, on 2026-10-08. It is no longer a guess: all four drive wheels were backwards and have been flipped (**1, 10, 11, 17**). The **toggles** (18 / 8), the **cascade** (13 / 2) and the **claw** (16) were already right.
- **Port 9 is the radio** (checked 2026-10-10). There is no inertial sensor yet.
- **Free ports** for tracking wheels and sensors: 3, 4, 5, 7, 12, 14, 15, 19, 20.
- **Port 6 is a communication device, not a motor.** Ignore it.
- The two **cascade** motors and the two **toggle** motors must always turn the
  same way as each other. Both programs send both motors of a pair the
  same command, which is all you need while they are on one shaft.

## Drive live angles

`cascade_robot_drive_limits.py` (`Cascade Robot Drive Limits.v5python`) is the
driver program in use. It started as a copy of the plain drive program
(drive v1, since removed) and adds five things:

- **Exponential stick curve.** A small push gives a small speed, and the top
  speed (`DRIVE_SPEED` / `TURN_SPEED`, 40) only comes with the stick pushed all
  the way to the end. Before, the speed was a straight line from the stick.
  In between it grows exponentially:
  `speed = top speed × (e^(CURVE × push) − 1) / (e^CURVE − 1)`, where push goes
  from 0 just past the deadband to 1 at the end of the stick. `DRIVE_CURVE`
  (left stick) and `TURN_CURVE` (right stick) are both **2**; bigger is gentler
  in the middle, 0 is the old straight line:

  | CURVE | stick 10% | 25% | 50% | 75% | 100% |
  |---|---|---|---|---|---|
  | 0 | 5% | 21% | 47% | 74% | 100% |
  | 1 | 3% | 14% | 35% | 63% | 100% |
  | **2** | 2% | 8% | 25% | 53% | 100% |
  | 3 | 1% | 5% | 16% | 43% | 100% |
  | 4 | 0% | 2% | 11% | 34% | 100% |

  (percent of the top speed, so half stick at curve 2 is 25% of 40 = 10% power)

- **The toggle is on Right / Y** instead of Up / Down. Right turns it one way,
  Y the other way.
- **The claw is one press, with no limit.** Press **R1 once and it opens**,
  press **R2 once and it closes** - no need to hold the button. Each press
  turns it `CLAW_TURN_DEGREES` (180) by itself, from wherever it is, and it
  holds where it ends up. Everything is a setting at the top of the file:

  | Setting | Now | What it does |
  |---|---|---|
  | `CLAW_OPEN_BUTTON` | `"R1"` | button that opens the claw |
  | `CLAW_CLOSE_BUTTON` | `"R2"` | button that closes the claw |
  | `CLAW_TURN_DEGREES` | `180` | how far one press turns it (fully open to fully closed measured 192°) |
  | `CLAW_OPEN_DIRECTION` / `CLAW_CLOSE_DIRECTION` | `REVERSE` / `FORWARD` | swap them if the open button closes the claw |
  | `CLAW_SPEED` | `55` | how fast it turns (100 = as fast as the motor goes) |

  Button names: `"L1" "L2" "R1" "R2" "Up" "Down" "Left" "Right" "X" "Y" "A" "B"`
  (capitals don't matter) - don't pick one the cascade (L1, L2) or toggle
  (Right, Y) already uses. A name that isn't a button stops the program with a
  message on the brain saying which setting to fix.
  The claw has **no current or torque limit** - it always has the motor's full
  strength, and at startup every motor is set back to 100% in case an earlier
  program left a limit on it. With no limit, pressing open when the
  claw is already open makes it push against its end at full strength until you
  press close, and squeezing a piece for a long time warms the motor up.
- **Every motor's angle is on the brain screen, live.** It refreshes ten times a
  second. The cascade, toggle and claw come first, then the four drive motors:

  ```
  LIVE ANGLES (deg)   slowest pass 10 ms
  Motor         angle   min   max  limit
  Cascade L 13    720     0   720  none
  Cascade R 2     720     0   720  none
  Toggle 18        0     0     0  none
  ...
  ```

  Each angle starts at **0 when the program starts** and goes **up** when the
  motor turns the way its button asks: cascade up (L1), toggle Right, drive
  forward, claw close (R2). `min` and `max` are the lowest and highest it has been
  this run. The controller shows the short version: cascade, toggle and claw
  angles. **Start every run with the cascade all the way down and the claw
  fully open** (and the toggle in the same place each time) so the numbers mean
  the same thing.
- **Soft limits.** Each mechanism motor has a `..._LOW` and `..._HIGH` setting
  (`None` = no limit). The limits come from the measured interval:
  `CASCADE_TRAVEL` (755°, base to top: the left side measured −9 to 746, the
  right −15 to 744) minus `CASCADE_MARGIN` (25°) gives **0 to 730 on both
  cascade motors**. If you measure again, change the travel number, not the
  limits. (The claw has no limit.) The toggle is still
  `None`; set it the same way: move it as far as it may safely go, read its angle,
  and type a number a little inside that. Each part stops at its limit and the
  screen says `AT LIMIT`. A pair stops as soon as either motor reaches its
  limit. Near a limit each part slows to `LIMIT_SLOW_SPEED` (20) over
  its own slow band: `CASCADE_SLOW_BAND` 60° and `TOGGLE_SLOW_BAND` 20°.

**Less delay.** The ~0.3 s lag between the controller and the robot came mostly
from the screens. The old drive v1 redrew the brain *and* the controller screen every
time a number on them changed, which is every pass while a stick is moving, and
the controller screen goes over the radio. This version redraws the brain 10
times a second, and sends the controller at most **one changed line** every
100 ms (never the whole screen, never in the same pass as the brain), because the
controller screen shares the radio with the sticks and buttons. It also reads
the controller every 10 ms instead of 20. The drive asks for a speed,
the way v1 did (`DRIVE_USE_VOLTAGE = False`). Voltage drive was the default for
one test and the robot would not drive backward; it now sends the direction
separately, but try `True` again only with the wheels off the floor. The top line of the brain shows the
slowest pass in milliseconds: it should stay near 10.

**Unplugged motors show up.** If the brain loses a motor (a loose cable - the
claw is on the long 1500 mm one), its row says `UNPLUGGED - check cable` and the
bottom line names it, instead of the motor just not responding.

## Override Auton

`override_auton.py` (`Override Auton.v5python`) is the match program for
Override. It was copied from the old PID match program on 2026-10-10, and its
driver control is the same as drive live angles. In the autonomous it:

1. flips the field Toggle behind the robot to our color;
2. sets the preload on our alliance goal;
3. grabs the yellow pin-in-cup two tiles out and stacks it on the preload;
4. backs off and stops.

Start in the middle of our alliance wall, with the back of the robot against
the wall, the cascade down and the claw closed on the preload. The autonomous
starts from field control, from **Timed Run** on the controller, or by holding
**A** for 1 second (off the field only; **B** stops it).

The field map, the route with every distance, the settings still to measure
and its troubleshooting are in [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md).

## Common edits

### Change a port or flip a motor

Ports are in `# --- Devices`:

```python
drive_right_front_1 = Motor(Ports.PORT1, DRIVE_GEARS, False)
#                           ^ port       ^ cartridge  ^ reversed?
```

- **Change both programs** (drive live angles and the Override Auton), so they stay the same.
- Moving a cable: change `Ports.PORT1` to the new port. You can rename the
  variable to match, but then you also have to change it everywhere else it's used.
- A motor runs the wrong way: flip `True` / `False`. In drive live angles every
  angle goes **up** for drive forward, cascade up (L1) and toggle Right, so the
  one that counts down is the one to flip.
- Two motors of a pair fight (buzzing, stalling, or one side twisting): one of
  them is reversed wrong. Their two angles on the drive live angles screen
  should count the same way.

### Change driver speeds or buttons

- **Speeds:** `CASCADE_SPEED`, `CLAW_SPEED` and `TOGGLE_SPEED` in Settings, in percent.
- **Drive and turn speed (drive live angles):** `DRIVE_SPEED` and `TURN_SPEED` are **40**; the cascade, claw and toggle are 55. They started at **40**
  (`DRIVE_SPEED`, `TURN_SPEED`, `CASCADE_SPEED`, `CLAW_SPEED`, `TOGGLE_SPEED`), so
  full stick gives 40% power and not 100%. `DRIVE_SPEED` is also a hard cap: no
  wheel can be asked for more than it, even when you drive and turn hard at the
  same time. Raise the numbers about 10 at a time.
- **The Override Auton** has the same driver settings at the top of
  `override_auton.py`. The autonomous has its own: `AUTON_DRIVE_SPEED` and
  `AUTON_TURN_SPEED` (they start equal to `DRIVE_SPEED` / `TURN_SPEED`).
- **Joystick deadzone:** `DEADBAND`. Raise it if the robot creeps when the sticks are let go.
- **Buttons:** in the driver loop (`driver()` in drive live angles,
  `driver_control()` in the Override Auton), change `controller_1.buttonL1` and the
  others (the claw's are settings: `CLAW_OPEN_BUTTON`, `CLAW_CLOSE_BUTTON`). The buttons are `buttonL1`, `buttonL2`, `buttonR1`, `buttonR2`,
  `buttonUp`, `buttonDown`, `buttonLeft`, `buttonRight`, `buttonA`, `buttonB`,
  `buttonX` and `buttonY`.

## Keeping the .py and .v5python the same

VEXcode edits the `.v5python` files. GitHub shows the `.py` files. After
changing one, copy the change into the other before committing. Each
command does both programs here (drive live angles and the Override Auton):

```bash
python3 sync_files.py from-vexcode   # you edited and saved in VEXcode
python3 sync_files.py from-py        # you edited the .py in another editor
python3 sync_files.py check          # are they the same?
```

Run these in this folder.

## Rules for editing VEXcode Python

VEXcode runs **MicroPython**, a smaller version of Python. To avoid weird errors on the brain:

- **Use `%` instead of f-strings:** `"X %.1f" % x`, not `f"X {x:.1f}"`.
- **No type hints, no `pip` packages.**
- **Every loop needs a `wait(10, MSEC)`** (or longer). Without it, nothing
  else on the brain gets a turn.
- **Don't put your code inside the `#region VEXcode Generated` block.** VEXcode may overwrite it.
- **Run a quick syntax check before downloading** if you have Python on your
  computer: `python3 -m py_compile cascade_robot_drive_limits.py override_auton.py`.
  It won't catch everything, but it catches typos.

## Before the first real run

- [ ] **Cascade cartridge.** It's set to red (`RATIO_36_1`) from the design
  doc. Check it, because a wrong cartridge makes all the heights wrong.
- [ ] **Directions.** After any wiring change, drive live angles' screen shows every motor's angle: each should go up for drive forward, cascade up and toggle Right.
- [ ] **Override Auton settings.** Measure the toggle flip and the cascade heights, and check the drive distance with `pid_test`. See [Measure these first](OVERRIDE_AUTON.md#measure-these-first).

## Troubleshooting

| Problem | Likely cause / fix |
|---|---|
| Robot spins when you push the stick forward | A drive motor is reversed wrong. Drive forward and watch the four drive angles in drive live angles: the one counting down is the one. |
| Cascade or toggle motors buzz or fight | One motor of the pair is reversed wrong. Their two angles in drive live angles should count the same way. |
| Drive live angles shows `UNPLUGGED` | No motor seen on that port. Check the cable, or fix the port number in both programs. |
| Cascade shows negative numbers, or stops short of the top | It wasn't all the way down when the program started. |
| Robot creeps with the sticks let go | Raise `DEADBAND`. |
| The autonomous doesn't run | Plain Run starts driver control. Hold A for 1 second, or use Timed Run. See [Running it](OVERRIDE_AUTON.md#running-it). |

## Change log

Add a line when you change something important (ports, gearing, gains, routines).

| Date | Change |
|---|---|
| 2026-10-07 | First version: driver control, PID autonomous ported from the C++ code, encoder odometry ready for tracking wheels. Tested in simulation only. |
| 2026-10-08 | Real ports from the team: drive left 11/20, right 1/10, cascade left 12 / right 2, toggle 18 + 8, claw 16. Added the motor test program. |
| 2026-10-08 | Second wiring check, two ports were wrong: cascade left **12 → 13** and drive left back **20 → 17**. Updated the Python programs, the `.v5python` copies, the PROS project and this table. |
| 2026-10-08 | Direction test on the robot. All four drive wheels spun the wrong way in MATCH direction, so **1, 10, 11 and 17** were flipped. The toggles (18 / 8), cascade (13 / 2) and claw (16) were already right. Ports and the PROS project updated to match. |
| 2026-10-08 | Added the plain **drive program**: `cascade_robot_drive.py` / `Cascade Robot Drive.v5python`. Split arcade drive plus cascade, claw and toggle, with no PID and nothing to calibrate. Port 6 noted as a communication device, not a motor. |
| 2026-10-08 | **Fixed a crash in the drive program.** The driver loop used the name `claw`, but only `claw_16` had been created, so pressing Run stopped with `NameError`. Added `claw = MotorGroup(claw_16)`. |
| 2026-10-08 | **Slowed the drive program right down.** It was capped at 100% (`DRIVE_SPEED` / `TURN_SPEED`), much faster than the motor test's 40%. Everything is now 40% (`CLAW_SPEED` 30), so full stick means 40% power. |
| 2026-10-08 | Cascade slowed to **25** (`CASCADE_SPEED`): at 40 the arm was still too quick. Drive, turn and toggle stay at 40, claw at 30. |
| 2026-10-08 | **All five speeds set to 10** for the first real test on the floor: `DRIVE_SPEED`, `TURN_SPEED`, `CASCADE_SPEED`, `CLAW_SPEED`, `TOGGLE_SPEED`. Full stick now means 10% power. |
| 2026-10-08 | Speeds raised to **40** across the board — 10 was too slow for the motors to move the robot. |
| 2026-10-08 | **Version 1 frozen as the backup.** Working version tagged `v1-drive-working-40` (commit `67304de` on GitHub) and copied to `versions/v1-drive-working-40/` in the Desktop folder, with a note on how to restore it. |
| 2026-10-08 | Added **drive v2**: `cascade_robot_drive_v2.py` / `Cascade Robot Drive V2.v5python`. A two-zone stick curve, per-control rate limiting, strain sensing with torque caps on the cascade and the claw, and a software floor so the cascade cannot be driven below where it started. v1 itself was not touched. Added `CONTROL_FEEL.md` and `TESTING_SAFETY.md` (removed with v2 on 2026-10-10). |
| 2026-10-08 | **v2's cascade now creeps onto its floor.** Coming down at `CASCADE_SPEED` it used to stop up to a full step above `CASCADE_LOWER_LIMIT`; within `CASCADE_CREEP_BAND` (25°) it is asked for `CASCADE_CREEP_SPEED` (10) instead, so it rests within about a degree of the limit. The ramp's own value is no longer zeroed at the floor, which removes a small stutter there. (That last part was refined again later — see the row about pinning the ramp.) |
| 2026-10-08 | **The same stick feel ported to the PROS C++.** New `include/control_feel.hpp` holds the deadband, the two-zone curve, the slew rate limiter and the split arcade mix; `DriveBase::manual_control` now shapes the sticks, so the old `turn * 0.7` fudge is gone. The settings live in `config.drive` in `src/main.cpp`. |
| 2026-10-08 | **v2 watches the encoder as well as the amps.** The strain guard now eases off a mechanism that has been told to move and is not moving, even when it is drawing a perfectly ordinary current — the case an amp threshold cannot see. New `USE_VELOCITY_CHECK`, `BLOCKED_FRACTION`, `BLOCKED_LOOPS` and `BLOCKED_MIN_ASK`; the brain screen shows `vel` and prints `BLOCKED` when that is what triggered it. |
| 2026-10-08 | **v2's floor now measures how long each pass takes** instead of assuming 20 ms. The fixed `CASCADE_DEG_PER_LOOP` (12°) is gone; `brain.timer.time(MSEC)` feeds `loop_seconds()` and `cascade_travel()`, with `CASCADE_DEG_PER_SECOND` (600) and `CASCADE_SAFETY_FACTOR` (1.5). A pass that takes longer than 20 ms used to under-predict the arm's travel, which is the direction that could put it through the floor. The bench test now proves it: with the loop made three times too slow, the old arithmetic drives the arm **1.79° below** the limit while the new code stops above it. |
| 2026-10-08 | **v2 pins the ramp at the floor** to `CASCADE_CREEP_SPEED` instead of leaving it at whatever it held. That makes a restart from the floor a crawl by design; leaving it alone had allowed a bigger jump, and zeroing it had caused a stutter. |
| 2026-10-08 | Added the **amps measuring tool**: `cascade_robot_amps.py` / `Cascade Robot Amps.v5python`. Hold A/B/X/Y and it shows what each mechanism really draws (`now` / `peak` / `steady`, plus the per-motor figure in brackets), so the v2 thresholds can stop being guesses. No torque ceiling, on purpose — a ceiling would clamp the very number you are reading. `sync_files.py` now handles five programs. |
| 2026-10-09 | **A full stop button.** In drive v2, hold **B** and every motor on the robot stops at once: every ramp is pinned to zero, the five motors are told to stop once, both strain guards are cleared and the screen says `***  FULL  STOP  ***`. Let go and you drive on from zero, with nothing to jump. A, B, X, Y, Left and Right are free in v1 and v2, so `STOP_BUTTON` can name any of them. Both programs look their button up **once** when they start: a misspelt name now prints a message and **refuses to drive** instead of throwing `AttributeError` part-way through a run. In the amps tool any d-pad button is the stop, it is checked before the test buttons, and a test cut short by it deliberately does not leave a peak behind. |
| 2026-10-09 | **The PROS C++ stopped being able to hurt the robot, and can now be checked.** `include/main.h` was missing — the project had **never compiled at all** — and is now written; `tools/syntax_check.sh` compile-checks every C++ file with clang against stand-in headers. The cascade's `move()` could be handed 135 where a `std::int8_t` (−128..127) was expected, which on the ARM wraps to **−121**: pressing a preset from rest would drive the arm **down** at nearly full power. It is clamped after the feedforward now, with a software floor at the bottom of the travel. The arm also used to keep running at full power for ever if you let go of R1 while a preset was still travelling; the claw's open and close ran each other's branch; the toggle came back to life by itself after `disabled()` was called, because the position PID still had its old target. All three are fixed, and **L2 is a new full stop** in the C++ driver control. |
| 2026-10-10 | Added **drive live angles** (`cascade_robot_drive_limits.py`), the next version of drive v1: toggle moved to **Right / Y**, a live angle for every motor on the brain (10 times a second) and controller, optional soft limits per mechanism motor (all off), and less control delay (screens redraw less often, 10 ms loop, voltage drive). Bench-tested against a simulated robot only. |
| 2026-10-10 | **Cascade limits set from the robot:** the left side travelled 755° from the base (−9 to 746) and the right side 759° (−15 to 744). Drive live angles now limits both sides to **0–730** (25° under the shorter side). Start each run with the cascade at the base. |
| 2026-10-10 | Drive live angles: **cascade, claw and toggle sped up from 40 to 55** (`CASCADE_SPEED`, `CLAW_SPEED`, `TOGGLE_SPEED`). Drive and turn stay at 40. The cascade still slows to 20 over the last 60° before a limit. |
| 2026-10-10 | **Claw limits set from the robot:** −16 fully open to 176 closed (192°). Drive live angles now limits the claw to **0–180**, starting fully open. Each mechanism has its own slow band near its limits (cascade 60°, claw and toggle 20°), so the claw is not slow for a third of its travel. |
| 2026-10-10 | **Drive live angles drives backward again.** With voltage drive on, the robot would not go backward, so the drive is back to v1's speed command (`DRIVE_USE_VOLTAGE = False`), and the voltage option now sends REVERSE with a positive voltage instead of a negative one. Cascade and claw limits now come from `CASCADE_TRAVEL` / `CLAW_TRAVEL` minus a margin. The team starts every run at the cascade's minimum. |
| 2026-10-10 | Drive live angles: **the claw is now one press, with no limit.** R1 opens it and R2 closes it; each press turns it `CLAW_TURN_DEGREES` (180) by itself and it holds there. The buttons (`CLAW_OPEN_BUTTON`, `CLAW_CLOSE_BUTTON`), the turn, its direction and a 1.2 A ceiling (`CLAW_MAX_AMPS`) are settings at the top of the file. The claw limits (`CLAW_TRAVEL`, `CLAW_MARGIN`, `CLAW_SLOW_BAND`) are gone. On the robot a plus speed had been *opening* the claw, so open is FORWARD; the 10-08 direction test recorded plus as closing, so the motor test and the other programs still label it that way. |
| 2026-10-10 | Drive live angles: **exponential stick curve.** The drive used to scale straight from the stick, so it felt like full speed by half stick. Now the speed grows exponentially and the top speed (40) only comes at the end of the stick: `DRIVE_CURVE` and `TURN_CURVE` = 2 (half stick = 25% of the top speed; 0 = the old straight line, bigger = gentler). |
| 2026-10-10 | Drive live angles, **full check:** the claw's 1.2 A current limit (`CLAW_MAX_AMPS`) is **gone** - it was making the claw weak and slow; every motor is set back to full strength at startup in case drive v2 left a limit on it. The controller screen now gets at most one changed line every 100 ms instead of clearing and redrawing all three (less radio traffic next to the sticks and buttons). Unplugged motors show `UNPLUGGED` on the brain. Button names in the claw settings ignore capitals, and a wrong one stops with a clear message instead of a `KeyError`. |
| 2026-10-10 | **Removed the old match program, drive v1 and the motor test** (`cascade_robot_auton.py`, `cascade_robot_drive.py`, `cascade_robot_test.py` and their `.v5python` files), the files last changed on 10-08. They are still in the git history (commit `4bde9ee` and before; drive v1 is also the tag `v1-drive-working-40`). Drive live angles is the driver program in use. The autonomous is being rebuilt for Override as a copy of the old match program, outside this repo for now. `sync_files.py` now handles three programs. Port 9 is the radio. |
| 2026-10-10 | Drive live angles: **claw direction fixed.** On the robot R1 was closing the claw and R2 opening it, so `CLAW_OPEN_DIRECTION` is now `REVERSE` and `CLAW_CLOSE_DIRECTION` `FORWARD`: a plus turn closes this claw, as the 10-08 direction test recorded. The claw's live angle now goes up when it closes. |
| 2026-10-10 | **Added the Override Auton** (`override_auton.py` / `Override Auton.v5python`, handout `OVERRIDE_AUTON.md`), moved here from its own Desktop folder. It flips the Toggle, sets the preload on our goal and stacks the yellow pin-in-cup on it, with Drive Limits' driver control. Hold A for 1 s to test it off the field. Preload drop height measured at 150; the other heights are guesses. Tested on a computer simulation only. |
| 2026-10-10 | **Removed drive v2 and the amps measuring tool** (`cascade_robot_drive_v2.py`, `cascade_robot_amps.py`, their `.v5python` files, `CONTROL_FEEL.md` and this folder's `TESTING_SAFETY.md`), so the folder holds only the two programs in use. They are still in the git history at commit `f863b31`. `sync_files.py` now handles drive live angles and the Override Auton. |
