# Cascade Robot: VEXcode Python handout

This folder is the cascade robot's program in **VEXcode V5 Python**. It has
the same PID setup as the PROS C++ code in the rest of this repo, rewritten so
it can be opened, edited and downloaded straight from VEXcode.

There are **four programs** in this folder now:

1. **Drive live angles** (`Cascade Robot Drive Limits`) — the current driver:
   live angle on every motor, soft limits, exponential stick curve, one-press claw.
   See [Drive live angles](#drive-live-angles).
2. **Drive v2** (`Cascade Robot Drive V2`) — stick feel, strain sensing, a
   software floor on the cascade. See [Drive v2](#drive-v2).
3. **Amps measuring tool** (`Cascade Robot Amps`) — see
   [Measuring the real currents](#measuring-the-real-currents).
4. **Override Auton** (`Override Auton`) — the match program for Override: the
   autonomous, plus the same driver control as drive live angles. See
   [Override Auton](#override-auton) and its own handout,
   [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md).

The match/autonomous program, the motor test and plain drive v1 were **removed
from the folder on 2026-10-10** (the autonomous is back, rebuilt for Override,
as the Override Auton). They are still in git history:

```bash
git show 723ddf1:vexcode-python/cascade_robot_auton.py
git show 723ddf1:vexcode-python/cascade_robot_test.py
git show v1-drive-working-40:vexcode-python/cascade_robot_drive.py
```

> **Status:** so far this has only been tested on a computer simulation of the
> robot, not on the real robot. Go through [Before the first real run](#before-the-first-real-run)
> first.

## Contents

- [Files](#files)
- [Putting it on the robot](#putting-it-on-the-robot)
- [Controls](#controls)
- [Ports](#ports)
- [Drive live angles](#drive-live-angles)
- [Override Auton](#override-auton) - the full handout is [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md)
- [Drive v2](#drive-v2)
- [Measuring the real currents](#measuring-the-real-currents)
- [Common edits](#common-edits)
  - [Change a port or flip a motor](#change-a-port-or-flip-a-motor)
  - [Change driver speeds or buttons](#change-driver-speeds-or-buttons)
- [Keeping the .py and .v5python the same](#keeping-the-py-and-v5python-the-same)
- [Rules for editing VEXcode Python](#rules-for-editing-vexcode-python)
- [Before the first real run](#before-the-first-real-run)
- [Troubleshooting](#troubleshooting)
- [Testing safely](TESTING_SAFETY.md) - read this before the first run
- [Why drive v2 feels the way it does](CONTROL_FEEL.md)
- [How this matches the C++ code](#how-this-matches-the-c-code)
- [Change log](#change-log)

## Files

| File | What it is |
|---|---|
| `Cascade Robot Drive Limits.v5python` | **Drive live angles — the current driver.** Open this in VEXcode. Live angle per motor, soft limits, exponential stick curve, one-press claw. |
| `cascade_robot_drive_limits.py` | Drive live angles as a plain Python file. |
| `Cascade Robot Drive V2.v5python` | **Drive v2.** Open this in VEXcode. Softer sticks, strain sensing, a software floor on the cascade. |
| `cascade_robot_drive_v2.py` | Drive v2 as a plain Python file. |
| `Cascade Robot Amps.v5python` | **Amps measuring tool.** Open this in VEXcode. Reads what each mechanism really draws, so the thresholds stop being guesses. |
| `cascade_robot_amps.py` | The amps tool as a plain Python file. |
| `Override Auton.v5python` | **Override Auton, the match program.** Open this in VEXcode. The autonomous plus drive live angles' driver control. |
| `override_auton.py` | The Override Auton as a plain Python file. |
| `sync_files.py` | Copies changes between each `.py` and its `.v5python` (runs on your computer, not the robot). |
| `README.md` | This handout. |
| `TESTING_SAFETY.md` | Read this before the first run of anything. |
| `CONTROL_FEEL.md` | Why drive v2 feels the way it does: the research, and what each constant does. |
| `OVERRIDE_AUTON.md` | The autonomous handout: field setup, the route, what to measure, how to start it. |

Removed 2026-10-10 (still in git history, commands in the intro above):
the match/autonomous program, the motor test program and plain drive v1.

## Putting it on the robot

1. Open **VEXcode V5**, then **File → Open** and pick a program:
   - `Cascade Robot Drive Limits.v5python` — **drive live angles**, the current
     driver. This is the version the team has been driving and measuring on the
     robot (travel limits, claw angles).
   - `Cascade Robot Drive V2.v5python` — **drive v2**. Softer sticks, a cascade
     that feels its own strain, and a software floor so the arm cannot be driven
     past where it started. Its torque caps are suspect — read
     [Testing safely](TESTING_SAFETY.md) first.
   - `Cascade Robot Amps.v5python` — the amps measuring tool. Not a driving
     program: it spins one mechanism at a time and shows you what it draws.
   - `Override Auton.v5python` — the match program: the Override autonomous
     plus drive live angles' driver control. Not run on the robot yet; see
     [OVERRIDE_AUTON.md](OVERRIDE_AUTON.md) first.
2. Plug in the brain (or the controller, with the brain paired), pick a slot and press **Download**.
   Put each program in a different slot, so all four are on the brain.
3. Before starting any drive program:
   - The **cascade must be all the way down** (and the claw fully open for live
     angles). Every program counts "0 degrees" from wherever the mechanism sits
     at startup, so the measured limits only mean something if it starts at the
     same place every run.
4. Pressing **Run** starts driver control, even in the Override Auton. Its
   autonomous only starts from field control, a competition switch, the
   **Timed Run** option, or holding **A** for 1 second (off the field only). See
   [Running it](OVERRIDE_AUTON.md#running-it).

## Controls

**Drive live angles** (the current driver):

| Control | Does |
|---|---|
| Left stick up/down | Drive forward / back |
| Right stick left/right | Turn |
| L1 / L2 | Cascade up / down (holds its spot when you let go) |
| R1 / R2 | Claw open / close — **one press** turns it `CLAW_TURN_DEGREES` (180°) and it holds there |
| Right / Y | Spin the toggle one way / the other |

**Drive v2** is the same sticks, but R1 / R2 **hold** the claw (close / open),
Up / Down spin the toggle, and **holding B stops every motor** ([FULL STOP](#drive-v2)).

**Amps tool:** A, B, X and Y run its four tests; any d-pad button stops them.

**Override Auton** has the drive live angles controls, plus two off the field
only: **hold A for 1 second** to run the autonomous where the robot stands, and
**B** stops it.

Settings at the top of each file change these: `CLAW_OPEN_BUTTON`,
`CLAW_CLOSE_BUTTON`, `DRIVE_SPEED`, `TURN_SPEED`, `CASCADE_SPEED`, …

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

- The "Reversed" column was measured on the real robot on 2026-10-08 (with the
  motor test program, since removed from the folder — recover it with
  `git show 723ddf1:vexcode-python/cascade_robot_test.py`). It is no longer a
  guess: all four drive wheels were backwards and have been flipped
  (**1, 10, 11, 17**). The **toggles** (18 / 8), the **cascade** (13 / 2) and the
  **claw** (16) were already right.
- **Port 9 is the radio** (confirmed 2026-10-11). It is not an IMU — **the robot
  has no IMU at all.** The PROS project's `imu_port = 9` is a bug: never download
  that code until it is fixed. See `RULES_CONFIGURATION.md`.
- **Port 6 is a communication device, not a motor.** Ignore it.
- **Free ports**: 3, 4, 5, 7, 12, 14, 15, 19, 20, 21 (the brain has 21 Smart
  Ports, numbered 1–21). Room for tracking wheels, a V5 Inertial Sensor, the lot.
- The two **cascade** motors and the two **toggle** motors must always turn the
  same way as each other. Every program sends both motors of a pair the same
  command, which is all you need while they are on one shaft.

## Drive live angles

`cascade_robot_drive_limits.py` (`Cascade Robot Drive Limits.v5python`) is the
current driver. It drives like the old plain drive v1 (still recoverable from
git), with five changes:

- **Exponential stick curve.** A small push gives a small speed, and the top
  speed (`DRIVE_SPEED` / `TURN_SPEED`, 40) only comes with the stick pushed all
  the way to the end. Before, the speed was a straight line from the stick.
  In between it grows exponentially:
  `speed = top speed × (e^(CURVE × push) − 1) / (e^CURVE − 1)`, where push goes
  from 0 just past the deadband to 1 at the end of the stick. `DRIVE_CURVE`
  (forward/back) and `TURN_CURVE` (turning) are both **2**; bigger is gentler
  in the middle, 0 is the old straight line. The left stick drives
  forward / back (axis3) and the right stick turns (axis1):

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
  program (drive v2) left a limit on it. With no limit, pressing open when the
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
from the screens. Drive v1 redrew the brain *and* the controller screen every
time a number on them changed, which is every pass while a stick is moving, and
the controller screen goes over the radio. This version redraws the brain 10
times a second, and sends the controller at most **one changed line** every
100 ms (never the whole screen, never in the same pass as the brain), because the
controller screen shares the radio with the sticks and buttons. It also reads
the controller every 10 ms instead of 20. The drive asks for a speed
exactly like v1 (`DRIVE_USE_VOLTAGE = False`). Voltage drive was the default for
one test and the robot would not drive backward; it now sends the direction
separately, but try `True` again only with the wheels off the floor. The top line of the brain shows the
slowest pass in milliseconds: it should stay near 10.

**Unplugged motors show up.** If the brain loses a motor (a loose cable - the
claw is on the long 1500 mm one), its row says `UNPLUGGED - check cable` and the
bottom line names it, instead of the motor just not responding.

## Override Auton

`override_auton.py` (`Override Auton.v5python`) is the match program for
Override. It is a copy of the old match program (2026-10-10), and its driver
control is the same as drive live angles. In the autonomous it:

1. flips the field Toggle behind the robot to our color;
2. sets the preload on our alliance goal;
3. grabs the yellow pin-in-cup two tiles out and stacks it on the preload;
4. backs off and stops.

Start in the middle of our alliance wall, with the back of the robot against
the wall, the cascade down and the claw closed on the preload. The field map,
the route with every distance, the settings still to measure (only the preload
drop height, 150, is measured so far) and its troubleshooting are in
[OVERRIDE_AUTON.md](OVERRIDE_AUTON.md). It has only run on a computer
simulation so far, and the route probably takes longer than 15 s at 40%.

## Drive v2

`cascade_robot_drive_v2.py` does everything v1 does — same ports, same
directions, same buttons — and adds three things the driver can feel. The long
version, with the research behind it, is in
[Why drive v2 feels the way it does](CONTROL_FEEL.md); this is the summary.

**Hold B and every motor on the robot stops.** This is the first thing the loop
looks at, before the sticks and before anything else, and it is the button to
reach for when something is going wrong. While B is held:

- every ramp — both wheels, the cascade, the claw, the toggle — is pinned to
  zero, so nothing is commanded even if you are still leaning on the sticks;
- the five motors are told to stop **once**, not fifty times a second;
- both strain guards are cleared, because being stopped on purpose is not a
  mechanism in trouble;
- the brain screen says `***  FULL  STOP  ***`.

Let go of B and you drive on from zero. Nothing jumps, because the ramps were
already brought down rather than left where they were. `STOP_BUTTON` in the
settings chooses a different button; if the name is wrong the program prints
which button it could not find and **refuses to drive**, rather than throwing an
error halfway through a run. A, B, X, Y, Left and Right are all free in v1 and
v2 — only L1, L2, R1, R2, Up and Down are used.

**The sticks have two speeds in one.** The first 85% of the stick travel is a
**fine zone**: it only reaches 40% of `DRIVE_SPEED`, and it is bent so that the
middle is especially gentle — half stick asks for about a quarter of the power.
The last 15% of the travel spends the rest, so the full speed you set only
happens **at the stop**. The two halves meet exactly, so there is no step where
they join. `stick_shape()` is the function; `FINE_END`, `FINE_TOP` and
`STICK_EXPO` are the knobs.

**Nothing can jump.** Every output — both sticks, the cascade, the claw, the
toggle — is **rate limited**: it can only change by `RAMP_PER_LOOP` (5) per
20 ms loop, so a motor takes about 0.4 s to go from stopped to full. Each
control has its own ramp value, because sharing one would make turning fight
driving. This is the same idea as WPILib's *slew rate limiter*; why it is not a
smoothing filter is in CONTROL_FEEL.md.

**The cascade watches its own current.** `class StrainGuard` reads
`current(CurrentUnits.AMP)`. If the arm pulls more than `CASCADE_STRAIN_AMPS`
(2.0 A, both motors added together) for three loops in a row, the program eases
the speed down towards 35% and drops the torque ceiling to 1.0 A **per motor**,
then recovers gently when the strain goes away. The mechanism is
`set_max_torque(amps, CurrentUnits.AMP)`. Note that the group's `current()`
adds the motors up while its `set_max_torque()` applies to each motor, and the
constants are commented to say which is which.

**It watches the encoder as well as the amps.** Amps catch a hard hit
instantly, but a chain starting to drag or an element wedged somewhere soft can
hold a motor back while it draws a perfectly ordinary current — that case is
invisible to an amp threshold however well you pick the number. So the same
guard also reads `velocity(VelocityUnits.PERCENT)`, how fast the motor is
really turning as a share of its own top speed, and compares it with what it
was asked for. Anything under `BLOCKED_FRACTION` (25%) of the request, for
`BLOCKED_LOOPS` (6) loops in a row, is "told to spin and not spinning" and
counts as strain by itself. Either symptom starts the easing.

Two details make that safe. The comparison is against the command the motor was
**actually** given, after any easing, so bringing the speed down lowers the bar
too and the guard cannot chase itself into a stall it invented. And
`BLOCKED_LOOPS` is longer than `CASCADE_STRAIN_LOOPS` on purpose, because a
motor takes a moment to spin up and that pause must not read as a fault.
`BLOCKED_MIN_ASK` leaves commands under 5% alone, where the reading is mostly
noise. The brain screen shows `vel` and prints `BLOCKED` when this is what
triggered the easing; set `USE_VELOCITY_CHECK = False` to go back to amps only.

**The claw grips and stops squeezing.** It has a permanent ceiling of
`CLAW_HOLD_AMPS` (1.2 A), and once it has been straining it eases its closing
speed, so it holds an object without cooking the motor. A claw that has closed
on something is also a motor told to turn that has stopped, so the encoder
check catches it even when the current looks normal.

**The cascade cannot be driven past where it started.** The arm has no physical
stop and the worry is the chain coming off the bottom, so the program remembers
where the arm was at startup (`cascade.reset_position()`) and refuses to send a
command that would take it more than `CASCADE_LOWER_LIMIT` degrees below that.

It also **creeps onto the floor instead of stopping a step above it.** Within
`CASCADE_CREEP_BAND` (25°) of the limit the arm stops being asked for
`CASCADE_SPEED` and is asked for `CASCADE_CREEP_SPEED` (10) instead, which gives
the ramp the whole last stretch to bring it down. At the limit itself the
command goes **hard to zero** — the ramp is there to make driving smooth, not to
soften a limit — and the brain screen says `FLOOR`.

At that moment the ramp's own value is pinned to `CASCADE_CREEP_SPEED` rather
than left wherever it was. Leaving it alone let the arm set off again from the
floor at whatever number the ramp happened to be holding, which is a jump bigger
than a crawl; zeroing it made the ramp start over from nothing and nudge the arm
down again, which is a stutter you can see. Pinning it makes the crawl the
ceiling from here, so restarting is bounded by design.

**How far the arm moves in one pass is measured, not assumed.** The floor check
has to predict where the arm will be after the command it is about to send, and
that prediction is a distance per loop. It used to be a fixed
`CASCADE_DEG_PER_LOOP` = 12°, worked out from the gearbox. That is only right if
every pass round the loop really takes 20 ms — and a pass now reads the
position, asks the guard, reads the velocity and the current, and draws the
screen. So the program times each pass itself with `brain.timer.time(MSEC)`,
turns that into degrees with `CASCADE_DEG_PER_SECOND` (600°/s, the output speed
of a red 36:1 cartridge — double it for green, ×6 for blue), and multiplies by
`CASCADE_SAFETY_FACTOR` (1.5) to allow for the arm still moving while the
program decides. A reading shorter than 20 ms is treated as exactly 20 ms,
because under-estimating the time under-estimates the travel, and that is the
unsafe direction to be wrong in. The bench test shows what a difference it
makes: with the loop three times too slow the arm really moved 14.4° in a pass,
and the old fixed 12° sum would have driven it **1.79° past the limit**.

The result is that the arm rests within about a degree of the limit instead of a
full-speed step above it. The exact distance is however far the arm travels in
one pass at `CASCADE_CREEP_SPEED`, so raising the creep speed to get more torque
also means resting slightly further from the limit.

The toggle is deliberately the exception: it gets the ramp like everything
else, but no stick curve and no strain easing.

> **Where v1 went.** `cascade_robot_drive.py` (v1) was removed from the folder
> on 2026-10-10 but is unchanged in git, as the tag `v1-drive-working-40`:
> `git show v1-drive-working-40:vexcode-python/cascade_robot_drive.py`.

## Measuring the real currents

Every current number in this folder was picked by hand. `CASCADE_STRAIN_AMPS`,
`CASCADE_EASE_AMPS`, `TORQUE_FULL_AMPS`, `CLAW_STRAIN_AMPS` and
`CLAW_HOLD_AMPS` are all somebody's guess at what this robot draws. They are
probably in the right region and they are certainly not measurements, and no
amount of reading the code can fix that — the only way to know is to watch the
motor.

**`Cascade Robot Amps.v5python`** (`cascade_robot_amps.py`) is that watch. It is
a measuring tool, not a driving program: **nothing in it drives the robot.**

1. Put the robot **on a stand**, wheels off the floor.
2. Open `Cascade Robot Amps.v5python`, download it, press Run.
3. Hold one button for the whole test:
   - **A** — cascade, lifting up
   - **B** — claw, closing
   - **X** — toggle, turning
   - **Y** — drive, forwards
4. While it runs, **load the mechanism by hand**: press down on the arm as it
   lifts, let the claw close on nothing and then put a game element in it, hold
   the toggle back, hold the robot back on the drive test. The `now` reading
   climbs as you do.
5. **Let go** and the motor stops at once — a measuring tool should never keep
   pushing once your hand has come off. Read the **PEAK** off the screen and
   write it down. `steady` is what it settles at once the motor has spun up.

One press is one test, and it runs for a few seconds. The idle screen keeps a
table of the peaks so far, so you can work through all four without a pen.

**Hold any d-pad button and everything stops.** Up, Down, Left or Right —
whichever finger is free. The stop is checked before the test buttons, so it
wins if you press both at once, and it works from the idle screen too. Pressing
it in the middle of a test **ends** that test rather than letting it finish, and
a test cut short that way does not leave a peak behind: you never remember a
half-finished reading as if it were a real one. It reaches every motor,
including ones that test never switched on, because a stop that only stopped
the thing you were looking at would not be a stop.

**Two warnings, both deliberate.** There is **no torque ceiling** in this
program: a ceiling is a clamp, and once a motor hits it the reading stops
climbing, so "working hard" can no longer be told from "about to stall" — the
whole number you came for would be hidden. That is also why the tests are short.
And **never wedge a mechanism so hard that the motor truly cannot turn, then
walk away.** V5 motors have their own internal protection, but the point of this
tool is to feel the load through your hand, not to test the motor's limits.

### Reading the numbers

The figure on the screen is **everything that test drives, added up**, because
`MotorGroup.current()` sums its motors (while `set_max_torque()` applies to each
one). So:

- The **cascade** reading is a **pair total**. `CASCADE_STRAIN_AMPS` (2.0) is
  written as a pair total too, so it compares directly — but
  `CASCADE_EASE_AMPS` (1.0) is **per motor**, so compare it with the number in
  brackets.
- The **claw** reading is one motor, so it compares directly with
  `CLAW_HOLD_AMPS`.
- The **drive** reading is all four wheels together, which is why it is the
  least useful one — watch the two sides instead if you ever need to.

That is why the screen shows the per-motor figure in brackets as well
(`0.70 A (0.35 each)`). Both are true; they just answer different questions.

Once you have the numbers: a threshold should sit **above** the worst peak the
mechanism hits on purpose and **below** what it draws when something is wrong.
If a peak you measured is already at or over the threshold in the program, the
threshold is too low and the mechanism will be eased off while it is working
normally — raise it. If the peak is far under, the threshold is doing nothing —
lower it until it is just above. Write the numbers in the comments next to the
constants so the next person knows where they came from.

> The encoder check in v2 needs no measuring, on purpose. Its numbers are
> **fractions of the command** rather than amps — "moving at under a quarter of
> what it was asked for" — so they mean the same thing on any motor. That is the
> advantage of measuring a mechanism against itself. It is also the reason the
> encoder half exists at all: an amp threshold is only as good as the number
> somebody guessed, and the fraction is not a guess.

## Wiring checks and the program layout

The **motor test program** and the **autonomous/match program** were removed
from the folder on 2026-10-10. Both are still in git history:

```bash
git show 723ddf1:vexcode-python/cascade_robot_test.py     # motor test
git show 723ddf1:vexcode-python/cascade_robot_auton.py    # match/auton
```

Until the motor test comes back, wiring is checked the same way the drive
programs already do it: every program shows `UNPLUGGED` next to a motor whose
cable is out, and the sign of a live angle tells you whether a motor is
reversed the way the program expects.
## Common edits

### Change a port or flip a motor

Ports are in `# --- Devices`:

```python
drive_right_front_1 = Motor(Ports.PORT1, DRIVE_GEARS, False)
#                           ^ port       ^ cartridge  ^ reversed?
```

- **Change the same block in all four programs**, so they stay the same.
- Moving a cable: change `Ports.PORT1` to the new port. You can rename the
  variable to match, but then you also have to change it everywhere else it's used.
- A motor runs the wrong way: flip `True` / `False`. Drive live angles shows
  each motor's live angle — a sign that points the wrong way finds it.

### Change driver speeds or buttons

- **Speeds:** `CASCADE_SPEED`, `CLAW_SPEED` and `TOGGLE_SPEED` in Settings, in percent.
- **Drive and turn speed (drive live angles):** all five speeds start at **40**
  (`DRIVE_SPEED`, `TURN_SPEED`, `CASCADE_SPEED`, `CLAW_SPEED`, `TOGGLE_SPEED`), so
  full stick gives 40% power and not 100%. `DRIVE_SPEED` is also a hard cap: no
  wheel can be asked for more than it, even when you drive and turn hard at the
  same time. Raise the numbers about 10 at a time.
- **How v2 responds:** everything in drive v2 is tuned by the constants at the
  top of `cascade_robot_drive_v2.py`, each one commented. The ones you will
  actually want are `FINE_TOP`, `STICK_EXPO`, `RAMP_PER_SECOND`,
  `CASCADE_STRAIN_AMPS`, `CLAW_HOLD_AMPS`, `CASCADE_CREEP_SPEED` and
  `BLOCKED_FRACTION`. [Why drive v2 feels the way it
  does](CONTROL_FEEL.md) explains each one and what happens when you move it.
- **Joystick deadzone:** `DEADBAND`. Raise it if the robot creeps when the sticks are let go.
- **Buttons:** in `driver_control()`, change `controller_1.buttonL1` and the
  others. The buttons are `buttonL1`, `buttonL2`, `buttonR1`, `buttonR2`,
  `buttonUp`, `buttonDown`, `buttonLeft`, `buttonRight`, `buttonA`, `buttonB`,
  `buttonX` and `buttonY`.
- **The full stop button:** `STOP_BUTTON = "B"` near the top of
  `cascade_robot_drive_v2.py`, and `STOP_BUTTONS` in `cascade_robot_amps.py`.
  A, B, X, Y, Left and Right are all free in v2. If you misspell the name the
  program prints which button it could not find and **will not drive**, rather
  than failing part-way through a run.
- **Make the toggle spin both ways:** both directions are already wired up —
  Right / Y in drive live angles, Up / Down in v2. To change which button, edit
  the two `buttonRight` / `buttonY` (or `buttonUp` / `buttonDown`) checks.

## Keeping the .py and .v5python the same

VEXcode edits the `.v5python` files. GitHub shows the `.py` files. After
changing one, copy the change into the other before committing. Each
command does all four programs (drive live angles, drive v2, the amps tool
and the Override Auton):

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
- **Every loop needs a `wait(10, MSEC)`** (or longer). Without it, the
  brain screen and the rest of the loop stop updating.
- **Don't put your code inside the `#region VEXcode Generated` block.** VEXcode may overwrite it.
- **Run a quick syntax check before downloading** if you have Python on your
  computer: `python3 -m py_compile cascade_robot_drive_limits.py override_auton.py`.
  It won't catch everything, but it catches typos.

## Before the first real run

- [ ] **Drive gearing.** The design doc says green motors with an 18T gear
  driving a 60T gear on 3.25" wheels. Check the real robot and fix
  `DRIVE_GEARS`. Everything currently runs at 40% and the safety ceiling is
  50% — see `PID_TEST_PLAN.md` before raising anything.
- [ ] **Cascade cartridge.** It's set to red (`RATIO_36_1`) from the design
  doc. Check it, because a wrong cartridge makes all the heights wrong — and in
  **v2** it also sets `CASCADE_DEG_PER_SECOND` (600 for red, 1200 for green,
  3600 for blue), which is how v2 works out how far the arm moves in one pass.
- [ ] **Cascade travel and claw angles.** `CASCADE_TRAVEL` (755) and the claw
  buttons were measured on the real robot 2026-10-10. Re-measure after any
  mechanical change; the ladder in `PID_TEST_PLAN.md` is where to start.
- [ ] **Wiring.** Ports and directions are in [Ports](#ports). Every program
  prints `UNPLUGGED` for a motor with no cable, and a wrong-way motor shows as
  a live angle moving the wrong direction.
- [ ] **Drive v2, on blocks.** Its ramp, stick curve, strain easing and cascade floor have never run on the real robot. Read [Testing safely](TESTING_SAFETY.md) and work through its list. Start with the arm **down**, because that is where the program learns "the bottom" from.
- [ ] **Measure the real currents.** Run the amps tool and write down what each mechanism actually draws, so v2's thresholds stop being guesses. See [Measuring the real currents](#measuring-the-real-currents).
- [ ] **Safety limits.** Start every motor at 10%, nothing above 50% until each
  stage of `PID_TEST_PLAN.md` has been verified.
- [ ] **Override Auton settings.** Measure the toggle flip and the cascade heights, and check the drive distance with `pid_test`. See [Measure these first](OVERRIDE_AUTON.md#measure-these-first).

## Troubleshooting

| Problem | Likely cause / fix |
|---|---|
| Robot spins when you push the stick forward | A drive motor is reversed wrong. Flip its `True`/`False` in the `# --- Devices` block — the live angle shows which one. |
| Cascade or toggle motors buzz or fight | One motor of the pair is reversed wrong. Check the signs of their live angles. |
| The autonomous doesn't run | Plain Run starts driver control. In the Override Auton, hold A for 1 second, or use Timed Run. See [Running it](OVERRIDE_AUTON.md#running-it). |
| A motor's row says `UNPLUGGED` | No motor on that port. Check the cable, or fix the port number in all four programs. |
| Cascade shows negative numbers or the limits are off | It wasn't all the way down when the program started. Restart with the arm down. |
| Claw closes but stops before gripping | `CLAW_STALL_AMPS` is too low. Raise it a little. |
| Claw never stops squeezing until the timeout | `CLAW_STALL_AMPS` is too high. Lower it. |
| Claw motor gets hot holding a piece | Lower `CLAW_HOLD_VOLTS`. |
| Robot creeps with the sticks let go | Raise `DEADBAND`. |
| Drive v2 says `FLOOR` and the arm will not go down | That is the software limit working. The arm was not at its resting place when the program started, so the bottom was learnt from the wrong place. Restart with the arm down. |
| Drive v2 stops with an error about `max_torque` | Set `USE_TORQUE_LIMITS = False` at the top of `cascade_robot_drive_v2.py` and it will run without torque caps. Nothing else changes. |
| Drive v2's sticks feel too soft | Raise `FINE_TOP` towards `1.0` and lower `STICK_EXPO` towards `1.0`. See [Why drive v2 feels the way it does](CONTROL_FEEL.md). |
| Drive v2 eases the cascade down even when it is moving freely | `CASCADE_STRAIN_AMPS` is too low for this arm. Raise it. |
| Drive v2 says `BLOCKED` on a mechanism that is plainly moving | `BLOCKED_FRACTION` is too high for this motor, or `BLOCKED_LOOPS` is too short. Raise the fraction, lengthen the loop count, or set `USE_VELOCITY_CHECK = False` to go back to amps only. |
| Amps tool shows a peak at or over a threshold in v2 | The threshold is too low for this robot — v2 will ease the mechanism off while it is working normally. Raise the threshold just above the peak you measured. |
| Amps tool shows `0.00 A` while a motor is clearly turning | You are reading the wrong group, or the cable is loose. Check the cable and the group name. |
| Amps tool shows a huge peak then the mechanism eases | You loaded it harder than the mechanism ever will be in a match. The useful number is the peak from a *normal* load, not from holding the arm still. |
| Drive v2 no longer eases off a mechanism that has stalled | Check `USE_VELOCITY_CHECK` is `True`. If it is, the motor is still turning faster than 25% of what was asked, so raise `BLOCKED_FRACTION`. |
| The `FULL STOP` screen shows but something still moves | That should be impossible: every ramp is pinned to zero and all nine motors are told to stop. If it happens, something other than this program is writing to a motor. |
| The full stop button does nothing | `STOP_BUTTON` names a button that does not exist, or one the program already uses. The brain screen prints which name it could not find. Use any of A, B, X, Y, Left or Right. |
| The amps tool will not measure anything | The screen says which entry in `STOP_BUTTONS` is wrong. Use any of Up, Down, Left or Right. |

## How this matches the C++ code

The C++ side (repo root `include/`, `src/`) predates this folder and is the
reference implementation. The old Python match program that mirrored it was
removed on 2026-10-10; the Override Auton is a copy of it, so the mapping now
holds in `override_auton.py`:

| C++ (this repo) | Python (`override_auton.py`) |
|---|---|
| `include/pid.hpp` `PIDController` | `class PID` |
| `DriveBase::drive_straight` | `drive_distance()` |
| `DriveBase::turn_to_heading` | `turn_to_heading()` |
| `Cascade` (presets, gravity feedforward) | `cascade_pid = SyncedMechanism(...)` + the `CASCADE_..._DEG` heights |
| `Toggle` (yellow / red / blue) | `toggle_pid = SyncedMechanism(...)` + `TOGGLE_FLIP_DEGREES` |
| `Claw::open` / `Claw::close` (stall detection) | `claw_open()` / `claw_close()` |
| Example auton state machine in `robot.cpp` | `auton_override()` (the Override route) |
| Ports in `src/main.cpp` | `# --- Devices` (real ports for this robot) |

Differences from the C++, still true in the Override Auton:

- **Odometry and `drive_to_point` / `turn_to_point`** were new there, so tracking wheels could be added later.
- **Each move had its own timeout**, and it returned whether it got there.
- **Driving backward worked.** Distance was measured with its sign, so
  `drive_distance(-24)` finished properly.
- **No D-term jump when a new move starts.** The first PID update skips the D term.
- **The cascade and toggle pairs were synced in autonomous too**, not just in driver control.

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
| 2026-10-08 | Added **drive v2**: `cascade_robot_drive_v2.py` / `Cascade Robot Drive V2.v5python`. A two-zone stick curve, per-control rate limiting, strain sensing with torque caps on the cascade and the claw, and a software floor so the cascade cannot be driven below where it started. v1 itself was not touched. Added [Why drive v2 feels the way it does](CONTROL_FEEL.md) and [Testing safely](TESTING_SAFETY.md). |
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
| 2026-10-11 | **Folder cleaned up.** The match/autonomous program, the motor test program and plain drive v1 were removed (the team had already deleted them in commit `f863b31`); they are recoverable from git — commands are at the top of this file. `sync_files.py` now handles the three remaining programs. This handout dropped every section that described the removed programs, and Port 9 is now documented as **the radio** (there is no IMU on the robot). New docs at the repo root: `RULES_CONFIGURATION.md` and `PID_TEST_PLAN.md`. |
| 2026-10-12 | Drive live angles: **claw direction fixed.** On the robot R1 was closing the claw and R2 opening it, so `CLAW_OPEN_DIRECTION` is now `REVERSE` and `CLAW_CLOSE_DIRECTION` `FORWARD`: a plus turn closes this claw, as the 10-08 direction test recorded. The claw's live angle now goes up when it closes. |
| 2026-10-12 | **Added the Override Auton** (`override_auton.py` / `Override Auton.v5python`, handout `OVERRIDE_AUTON.md`): a copy of the old match program that flips the Toggle, sets the preload on our goal and stacks the yellow pin-in-cup on it, with drive live angles' driver control. Hold A for 1 s to test it off the field. Preload drop height measured at 150; the other heights are guesses. Tested on a computer simulation only. `sync_files.py` now handles four programs. |
