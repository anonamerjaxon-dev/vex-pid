# Cascade Robot: VEXcode Python handout

This folder is the cascade robot's program in **VEXcode V5 Python**. It has
the same PID setup as the PROS C++ code in the rest of this repo, rewritten so
it can be opened, edited and downloaded straight from VEXcode.

The two main programs are:

1. **Match program** (`Cascade Robot Auton`), used in competition:
   - **Driver control**: split arcade drive, cascade, claw and toggle.
   - **Autonomous**: PID drive and turns, cascade/toggle PID, claw grip.
   - **Position tracking (odometry)**: works now with the drive motor encoders.
     When tracking wheels are added, you change a few settings and nothing else.
2. **Motor test program** (`Cascade Robot Test`), used for checking wiring.
   Each button spins **one** motor, in exactly the direction the match program
   uses, so you can check every port and direction one at a time.

There are also three programs for driver control and for the bench:
**drive v1** (`Cascade Robot Drive`, the known-good one), **drive v2**
(`Cascade Robot Drive V2`, see [Drive v2](#drive-v2)) and the **amps measuring
tool** (`Cascade Robot Amps`, see [Measuring the real currents](#measuring-the-real-currents)).

> **Status:** so far this has only been tested on a computer simulation of the
> robot, not on the real robot. Go through [Before the first real run](#before-the-first-real-run)
> first.

## Contents

- [Files](#files)
- [Putting it on the robot](#putting-it-on-the-robot)
- [Controls](#controls)
- [Ports](#ports)
- [Drive program](#drive-program)
- [Drive live angles](#drive-live-angles)
- [Drive v2](#drive-v2)
- [Measuring the real currents](#measuring-the-real-currents)
- [Motor test program](#motor-test-program)
- [How the program is laid out](#how-the-program-is-laid-out)
- [Common edits](#common-edits)
  - [Pick which autonomous runs](#pick-which-autonomous-runs)
  - [Write a new autonomous routine](#write-a-new-autonomous-routine)
  - [Change a port or flip a motor](#change-a-port-or-flip-a-motor)
  - [Change driver speeds or buttons](#change-driver-speeds-or-buttons)
  - [Set the cascade heights and toggle angles](#set-the-cascade-heights-and-toggle-angles)
  - [Tune the PID](#tune-the-pid)
  - [Add tracking wheels](#add-tracking-wheels)
  - [Add an inertial sensor](#add-an-inertial-sensor)
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
| `Cascade Robot Auton.v5python` | **Match program.** Open this in VEXcode. |
| `cascade_robot_auton.py` | The match program as a plain Python file, so it can be read and reviewed on GitHub. |
| `Cascade Robot Test.v5python` | **Motor test program.** Open this in VEXcode. |
| `cascade_robot_test.py` | The motor test program as a plain Python file. |
| `Cascade Robot Drive.v5python` | **Drive v1 (no PID).** Open this in VEXcode. The known-good version. |
| `cascade_robot_drive.py` | Drive v1 as a plain Python file. |
| `Cascade Robot Drive V2.v5python` | **Drive v2.** Open this in VEXcode. Softer sticks, strain sensing, a software floor on the cascade. |
| `cascade_robot_drive_v2.py` | Drive v2 as a plain Python file. |
| `Cascade Robot Amps.v5python` | **Amps measuring tool.** Open this in VEXcode. Reads what each mechanism really draws, so the thresholds stop being guesses. |
| `cascade_robot_amps.py` | The amps tool as a plain Python file. |
| `Cascade Robot Drive Limits.v5python` | **Drive live angles.** Drive v1 plus a live angle for every motor and optional soft limits. Toggle on Right / Y. |
| `cascade_robot_drive_limits.py` | Drive live angles as a plain Python file. |
| `sync_files.py` | Copies changes between each `.py` and its `.v5python` (runs on your computer, not the robot). |
| `README.md` | This handout. |
| `TESTING_SAFETY.md` | Read this before the first run of anything. |
| `CONTROL_FEEL.md` | Why drive v2 feels the way it does: the research, and what each constant does. |

## Putting it on the robot

1. Open **VEXcode V5**, then **File → Open** and pick a program:
   - `Cascade Robot Drive.v5python` — **drive v1**. No PID, nothing to calibrate.
     This is the version that has already been driven on the robot.
   - `Cascade Robot Drive V2.v5python` — **drive v2**. Softer sticks, a cascade
     that feels its own strain, and a software floor so the arm cannot be driven
     past where it started. **This one has not been on the robot yet** — read
     [Testing safely](TESTING_SAFETY.md) first.
   - `Cascade Robot Auton.v5python` — the match program: driver control plus a
     PID autonomous.
   - `Cascade Robot Test.v5python` — the motor test, for checking wiring.
   - `Cascade Robot Amps.v5python` — the amps measuring tool. Not a driving
     program: it spins one mechanism at a time and shows you what it draws.
2. Plug in the brain (or the controller, with the brain paired), pick a slot and press **Download**.
   Put each program in a different slot, so all five are on the brain.
3. Before starting the **match program**:
   - The **cascade must be all the way down.** The program counts "0 degrees" from wherever the lift is at startup.
   - If an inertial sensor is set up, **don't touch the robot** while the screen says "Calibrating" (about 2 seconds).
4. Pressing **Run** on its own starts **driver control**. To run **autonomous**,
   the robot needs a match start: a competition switch, field control, or the
   **Timed Run** option in the V5 controller's program menu (autonomous, then
   driver control).

The match program shows this on the brain screen the whole time, including
during driver control:

```
X 0.0  Y 0.0  H 0.0        <- position in inches and heading in degrees
Cascade 0  Toggle 0        <- mechanism positions in motor degrees
Auton: pid_test            <- which autonomous routine is selected
```

## Controls

These are the **match program** controls:

| Control | Does |
|---|---|
| Left stick up/down | Drive forward / back |
| Right stick left/right | Turn |
| L1 / L2 | Cascade up / down (holds its spot when you let go) |
| R1 / R2 | Claw close / open |
| Down arrow (hold) | Spin the toggle |

The drive programs are different — they use only the sticks and L1, L2, R1, R2,
Up and Down. **Drive v2 adds one more: hold B and every motor stops**
([FULL STOP](#drive-v2)). The measuring tool uses A, B, X and Y for its four
tests and any d-pad button to stop.

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

- The "Reversed" column was measured on the real robot with the [motor test program](#motor-test-program), in **MATCH** direction, on 2026-10-08. It is no longer a guess: all four drive wheels were backwards and have been flipped (**1, 10, 11, 17**). The **toggles** (18 / 8), the **cascade** (13 / 2) and the **claw** (16) were already right.
- **Port 9** has a single device on it that nobody has identified yet. The brain's
  **Devices** screen shows what it is. If it's an inertial sensor, see [Add an inertial sensor](#add-an-inertial-sensor).
- **Free ports** for tracking wheels and sensors: 3, 4, 5, 7, 12, 14, 15, 19, 20.
- **Port 6 is a communication device, not a motor.** Ignore it.
- The two **cascade** motors and the two **toggle** motors must always turn the
  same way as each other. The match program keeps them exactly in sync:
  whichever one gets ahead is slowed down until the other catches up. The
  [drive program](#drive-program) just sends both motors of a pair the same
  command, which is all you need while they are on one shaft.

## Drive program

`cascade_robot_drive.py` is the plain drive program: **no PID, no odometry and
no sensors to calibrate.** It only uses the controller and the motors, so it is
the quickest way to get the robot moving and the easiest version to read.

| Control | Does |
|---|---|
| Left stick up/down | Drive forward / back |
| Right stick left/right | Turn |
| L1 / L2 | Cascade up / down |
| R1 / R2 | Claw close / open |
| Up / Down | Toggle one way / the other way |

Let go of a button and that part **holds where it is**, so the cascade does not
fall and the claw keeps its grip.

The sticks use **split arcade drive**: the left side is asked for
`forward + turn` and the right side for `forward - turn`. If that would ask a
side for more than 100%, both sides are scaled down together instead of one
side being clipped, so the turn keeps its shape. A small **deadband** (`5`)
ignores tiny stick movements so the robot does not creep.

**Everything starts at 40.** `DRIVE_SPEED`, `TURN_SPEED`, `CASCADE_SPEED`,
`CLAW_SPEED` and `TOGGLE_SPEED` are all **40**, so full stick means 40% power,
not 100%. That is about half throttle: enough for every part to actually move,
slow enough to stop before anything breaks. Raise them about 10 at a time.

It also reads the drive a little differently from the match program: the
cascade and the toggle are plain `MotorGroup`s, so both motors of a pair just
get the same command. The match program does more than that - it compares the
two motors' positions and slows the one that is ahead until the other catches
up.

There is nothing to set up before running it. The brain screen shows the drive
power and what each mechanism is doing, and the controller screen shows the
short version.

> In the VS Code project, the file that gets downloaded to the brain is
> `src/main.py` inside the project folder. There are two project folders,
> `Cascade_Robot_Drive` and `Cascade_Robot_Test`, and both hold a copy. Put
> whichever program you want to run into that file - it is a plain copy of
> the `.py`.

## Drive live angles

`cascade_robot_drive_limits.py` (`Cascade Robot Drive Limits.v5python`) is the
next version of the drive program. It drives exactly like drive v1, with three
changes:

- **The toggle is on Right / Y** instead of Up / Down. Right turns it one way,
  Y the other way.
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
  motor turns the way its button asks: cascade up (L1), claw close (R1), toggle
  Right, drive forward. `min` and `max` are the lowest and highest it has been
  this run. The controller shows the short version: cascade, toggle and claw
  angles. **Start every run with the cascade all the way down and the claw
  fully open** (and the toggle in the same place each time) so the numbers mean
  the same thing.
- **Soft limits.** Each mechanism motor has a `..._LOW` and `..._HIGH` setting
  (`None` = no limit). The limits come from the measured interval:
  `CASCADE_TRAVEL` (755°, base to top: the left side measured −9 to 746, the
  right −15 to 744) minus `CASCADE_MARGIN` (25°) gives **0 to 730 on both
  cascade motors**, and `CLAW_TRAVEL` (192°, −16 open to 176 closed) minus
  `CLAW_MARGIN` (12°) gives **0 (open) to 180 (closed)** for the claw. If you
  measure again, change the travel number, not the limits. The toggle is still
  `None`; set it the same way: move it as far as it may safely go, read its angle,
  and type a number a little inside that. Each part stops at its limit and the
  screen says `AT LIMIT`. A pair stops as soon as either motor reaches its
  limit. Near a limit each part slows to `LIMIT_SLOW_SPEED` (20) over
  its own slow band: `CASCADE_SLOW_BAND` 60°, `CLAW_SLOW_BAND` and
  `TOGGLE_SLOW_BAND` 20°.

**Less delay.** The ~0.3 s lag between the controller and the robot came mostly
from the screens. Drive v1 redrew the brain *and* the controller screen every
time a number on them changed, which is every pass while a stick is moving, and
the controller screen goes over the radio. This version redraws the brain 10
times a second and the controller 4 times a second, never both in one pass. It
also reads the controller every 10 ms instead of 20. The drive asks for a speed
exactly like v1 (`DRIVE_USE_VOLTAGE = False`). Voltage drive was the default for
one test and the robot would not drive backward; it now sends the direction
separately, but try `True` again only with the wheels off the floor. The top line of the brain shows the
slowest pass in milliseconds: it should stay near 10.

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

> **v1 and v2 live side by side.** `cascade_robot_drive.py` is v1, unchanged,
> and `cascade_robot_drive_v2.py` is v2. Neither replaces the other. The v1
> backup also exists as the git tag `v1-drive-working-40` and as a folder in
> `versions/v1-drive-working-40/`.

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

## Motor test program

Use this whenever the wiring changes, before running the match program.
Put the robot **on a stand** so the wheels are off the ground. Then hold one
button at a time:

| Button | Motor | It's right if... |
|---|---|---|
| Up | Drive left front (11) | the wheel rolls forward |
| Down | Drive left back (17) | the wheel rolls forward |
| X | Drive right front (1) | the wheel rolls forward |
| B | Drive right back (10) | the wheel rolls forward |
| L1 | Cascade left (13) | the lift goes up |
| R1 | Cascade right (2) | the lift goes up |
| L2 | Toggle (18) | it spins the same way as 8 |
| R2 | Toggle (8) | it spins the same way as 18 |
| A | Claw (16) | the claw closes |
| Right arrow | — | flips between MATCH direction and REVERSE (to move things back) |

**Why this proves the match program is right:** all three programs use the exact
same `# --- Devices` block (same ports, same `True`/`False`). Each button
spins its motor the same way the match program does for drive forward, cascade
up, claw close and toggle spin. So if L1 makes the left cascade motor go up and
R1 makes the right one go up, the two will work together in a match.

**If a motor goes the wrong way**, flip its `True`/`False` in the
`# --- Devices` block, in **all three** programs. For example:

```python
cascade_right_2 = Motor(Ports.PORT2, CASCADE_GEARS, True)   # True -> False
```

Good to know:

- **Only one motor spins at a time.** Pressing two buttons at once spins nothing.
- **Linked motors move together.** If both wheels on one side are geared
  together, one button turns the whole side. One cascade motor also lifts the
  whole cascade. That's normal: just check the direction.
- **Everything coasts in the test** (no holding), so the other motor of a pair
  doesn't fight the one being tested. Because of that, the cascade can slide
  down when you let go. Test it near the bottom with short presses.
- **The test runs at 40% speed** (`TEST_SPEED`). That's slower than a match,
  but the direction is the same.
- **The brain screen** lists every button, motor and what it should do.
  If a motor isn't found on its port, its row says `MISSING`, so a cable in
  the wrong port shows up right away.

## How the program is laid out

Each part of `cascade_robot_auton.py` starts with a `# --- Name ---` comment, so
you can jump to it with Find (Ctrl/Cmd+F). From top to bottom:

| Section | What's in it | Edit it? |
|---|---|---|
| `#region VEXcode Generated...` | VEXcode's own setup code | **No.** VEXcode may rewrite it. |
| `# --- Settings` | Every number you'd want to change: routine, gearing, ports for sensors, PID gains, presets, claw timing | **Yes, most edits go here** |
| `# --- Devices` | Motor ports and which motors are reversed. The same block is in the motor test program. | When wiring changes (change both programs) |
| `# --- Helpers` | Small functions (`clamp`, sync for motor pairs) | Rarely |
| `# --- PID controller` | The `PID` class | Rarely |
| `# --- Position tracking` | The `Odometry` class (where the robot is) | Rarely |
| `# --- Cascade and toggle in autonomous` | `SyncedMechanism`, PID for a motor pair | Rarely |
| `# --- Autonomous moves` | `drive_distance`, `turn_to_heading`, `claw_close`, and the rest | Rarely |
| `# --- Background loop` | Runs every 10 ms: tracking, cascade/toggle PID, brain screen | Rarely |
| `# --- Autonomous routines` | The routines and the `ROUTINES` list | **Yes, write autons here** |
| `# --- Startup`, `# --- Autonomous`, `# --- Driver control` | What runs when | When changing controls |

## Common edits

### Pick which autonomous runs

At the top of Settings:

```python
AUTON_ROUTINE = "pid_test"
```

| Name | What it does |
|---|---|
| `"pid_test"` | Forward 24", turn to 90°, turn back to 0°, back 24". It should end where it started. Use it to tune the PID. |
| `"odom_test"` | Drives a 24" square and comes back. The screen should read about X 0, Y 0, H 0 at the end. |
| `"example"` | The C++ code's example scoring route. The distances are placeholders. |
| `"none"` | Does nothing. |

### Write a new autonomous routine

1. In `# --- Autonomous routines`, add a function:

   ```python
   def auton_left_side():
       claw_close()                                  # grip the preload
       cascade.move_to(CASCADE_PRESETS["low"])       # lift while driving
       drive_distance(30)
       turn_to_heading(45)
       cascade.move_to(CASCADE_PRESETS["high"], wait_until_done=True)
       claw_open()
       cascade.move_to(CASCADE_PRESETS["down"])
       drive_distance(-12)
   ```

2. Add it to `ROUTINES`:

   ```python
   ROUTINES = {
       "pid_test": auton_pid_test,
       ...
       "left_side": auton_left_side,
   }
   ```

3. Set `AUTON_ROUTINE = "left_side"`.

**Field directions.** The robot starts at X 0, Y 0, facing heading 0.

```
                +Y  (the way the robot faces at the start)
                 ^
                 |
     -X  <---- robot ---->  +X
                 |
                 v
                -Y

Heading: 0 = start direction, 90 = turned right, -90 = turned left
```

If you'd rather use real field positions, call `odom.set_pose(x, y, heading)`
as the first line of the routine.

**Building blocks**

| Call | What it does |
|---|---|
| `drive_distance(inches)` | Drive straight, holding the current heading. Negative = backward. |
| `turn_to_heading(degrees)` | Turn in place to face a heading, the short way round. |
| `turn_to_point(x, y)` | Turn to face a spot on the field (inches). |
| `drive_to_point(x, y)` | Turn to face a spot, then drive forward to it. |
| `cascade.move_to(degrees)` | Start lifting to a height. The robot keeps going while it moves. |
| `cascade.move_to(degrees, wait_until_done=True)` | Lift, and wait until it gets there. |
| `toggle.move_to(degrees)` | Spin the toggle to an angle (same options as the cascade). |
| `claw_open()` | Open for `CLAW_OPEN_MS`. |
| `claw_close()` | Close until it squeezes something, then keep a light grip. |
| `wait(500, MSEC)` | Pause. |

Extra options:

- **Slower moves:** every drive and turn takes `max_volts`. 12 is full speed, so
  `drive_distance(20, max_volts=6)` is about half speed. This helps with gentle
  pushes and lining up.
- **Timeouts:** each move gives up after a while so one bad move can't waste
  the whole autonomous. The default is 1 s plus 0.15 s per inch, or 1 s plus 15 ms per degree.
  To change it for one move: `drive_distance(48, timeout_ms=4000)`.
- **Did it get there?** Moves return `True` if they reached the target and
  `False` if they timed out:

  ```python
  if not drive_distance(24):
      drive_distance(-6)    # back off and carry on
  ```

### Change a port or flip a motor

Ports are in `# --- Devices`:

```python
drive_right_front_1 = Motor(Ports.PORT1, DRIVE_GEARS, False)
#                           ^ port       ^ cartridge  ^ reversed?
```

- **Always change both programs** (match and motor test), so they stay the same.
- Moving a cable: change `Ports.PORT1` to the new port. You can rename the
  variable to match, but then you also have to change it everywhere else it's used.
- A motor runs the wrong way: flip `True` / `False`. The motor test program
  shows which one.
- Two synced motors fight (buzzing, stalling, or one side twisting): one of
  them is reversed wrong. Run the motor test program to find which.

### Change driver speeds or buttons

- **Speeds:** `CASCADE_SPEED`, `CLAW_SPEED` and `TOGGLE_SPEED` in Settings, in percent.
- **Drive and turn speed (drive program):** all five speeds start at **40**
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
- **Make the toggle spin both ways:** under the `buttonDown` check, add
  `elif controller_1.buttonUp.pressing(): toggle_speed = -TOGGLE_SPEED`
  (as two lines, like the cascade buttons).

### Set the cascade heights and toggle angles

The heights in `CASCADE_PRESETS` and the angles in `TOGGLE_ANGLES` are
**placeholders**. To find the real ones:

1. Start the program with the cascade all the way down.
2. In driver control, lift the cascade to the height you want.
3. Read `Cascade ___` on the brain screen and write that number into `CASCADE_PRESETS`.
4. Set `CASCADE_MAX_DEG` a little under the very top, so the PID can't run the lift into its hard stop.

Do the same for the toggle angles.

### Tune the PID

**What the numbers mean.** Each `*_GAINS` set controls one kind of move. The
output is motor volts (−12 to 12).

| Key | Meaning |
|---|---|
| `kp` | Push harder the further away it is. The main one. |
| `kd` | Brakes as it closes in. Stops overshoot. |
| `ki` | Slowly builds up if it's stuck just short of the target. Keep it small. |
| `integral_limit` | Cap on how much `ki` can build up. |
| `output_max` | Volts cap for this PID (only the heading PID uses it). |
| `settle_error` | How close counts as "there" (inches for drive, degrees otherwise). |
| `settle_ticks` | How many 10 ms ticks in a row it has to stay "there" before the move ends. |

| Gains | Used by |
|---|---|
| `DRIVE_GAINS` | Distance in `drive_distance` |
| `HEADING_GAINS` | Keeping straight during `drive_distance` |
| `TURN_GAINS` | `turn_to_heading` and `turn_to_point` |
| `CASCADE_GAINS` (+ `CASCADE_FEEDFORWARD_VOLTS`) | `cascade.move_to` |
| `TOGGLE_GAINS` | `toggle.move_to` |

**How to tune.** Use `AUTON_ROUTINE = "pid_test"` on a field tile.

1. Set `ki` to 0.
2. Raise `kp` until the robot gets there quickly and overshoots a little.
3. Raise `kd` until the overshoot goes away. If the robot shakes at the end, `kd` is too high.
4. Only if it still stops a bit short, add a small `ki`.
5. Change one number at a time, by about 10–25%, and write down what happened.

| What you see | Try |
|---|---|
| Stops short, and the move only ends when it times out | Raise `kp`. If it's still short, add a little `ki`. |
| Overshoots, then comes back | Raise `kd` or lower `kp` |
| Shakes or buzzes at the target | Lower `kd`, then `kp` |
| Drifts to one side when driving straight | Raise `HEADING_GAINS` `kp` |
| Wiggles left and right when driving straight | Lower `HEADING_GAINS` `kp` or raise its `kd` |
| Cascade sags while holding a height | Raise `CASCADE_FEEDFORWARD_VOLTS` |
| Cascade creeps up on its own | Lower `CASCADE_FEEDFORWARD_VOLTS` |

> The current gains were tuned on a computer simulation, so expect to retune
> them. They also depend on the drive gearing. **If you change the gearing,
> retune them.**

### Add tracking wheels

Tracking wheels (coordinate wheels) are unpowered omni wheels on **Rotation
sensors**. They measure how far the robot really moved, even when the drive
wheels slip. The code is already written for them. You just fill in Settings:

```python
TRACKER_WHEEL_DIAMETER_IN = 2.0

VERTICAL_TRACKER_PORT = Ports.PORT3        # wheel that rolls forward/back
VERTICAL_TRACKER_REVERSED = False
VERTICAL_TRACKER_OFFSET_IN = -1.5          # inches RIGHT of the turning center (left = -)

HORIZONTAL_TRACKER_PORT = Ports.PORT4      # wheel that rolls sideways
HORIZONTAL_TRACKER_REVERSED = False
HORIZONTAL_TRACKER_OFFSET_IN = -3.0        # inches IN FRONT of the turning center (behind = -)
```

You can add just the vertical wheel first. Leave the horizontal port `None` until it's on.

**Measuring the offsets.** The turning center is the middle point between the
left and right drive wheels. On most robots that's the middle of the drivetrain.
Measure from there to the tracking wheel's contact point with the floor.

**Checking it** (with the program running, watch the brain screen):

1. Push the robot straight forward by hand. **Y** should go up. If it goes
   down, flip `VERTICAL_TRACKER_REVERSED`.
2. Slide it sideways to the right. **X** should go up. If it goes down, flip
   `HORIZONTAL_TRACKER_REVERSED`.
3. Push it exactly 48 inches with a tape measure. Y should read about 48. If it
   doesn't, adjust `TRACKER_WHEEL_DIAMETER_IN`: new diameter = old × 48 / shown.
4. Spin it in place a few times. X and Y should stay near 0. If they draw a
   circle, the offset is wrong: either the sign is wrong or the measurement is off.
5. Run `AUTON_ROUTINE = "odom_test"`. It should come back to about X 0, Y 0.

The heading still comes from the drive motors, or from the inertial sensor if
there is one. **Tracking wheels plus an inertial sensor** is the most accurate setup.

### Add an inertial sensor

Set `IMU_PORT = Ports.PORT9` (or whichever port it's on) in Settings. Nothing
else changes. Turns get much more accurate, because the heading no longer
depends on wheel slip or `TRACK_WIDTH_IN`. The robot has to stay still for
about 2 seconds while it calibrates at startup.

## Keeping the .py and .v5python the same

VEXcode edits the `.v5python` files. GitHub shows the `.py` files. After
changing one, copy the change into the other before committing. Each
command does all five programs (match, motor test, drive v1, drive v2 and the
amps tool):

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
  background loop that does tracking and the cascade PID stops running.
- **Autonomous loops should check `auton_active`**, the way `drive_distance`
  does. Otherwise they keep running after driver control starts.
- **Don't put your code inside the `#region VEXcode Generated` block.** VEXcode may overwrite it.
- **Run a quick syntax check before downloading** if you have Python on your
  computer: `python3 -m py_compile cascade_robot_auton.py`. It won't catch
  everything, but it catches typos.

## Before the first real run

- [ ] **Drive gearing.** The design doc says green motors with an 18T gear
  driving a 60T gear on 3.25" wheels. That's only about 10 inches per second,
  which doesn't match the "360 RPM wheels" it also mentions. Check the real
  robot and fix `DRIVE_GEARS`, `DRIVE_MOTOR_GEAR_TEETH` and
  `DRIVE_WHEEL_GEAR_TEETH`. For example, blue 600 RPM motors with 36T → 60T
  give 360 RPM. **Then retune the PID.** At the 10 in/s speed, the `example`
  route takes about 18 s, which is too long for a 15 s autonomous.
- [ ] **Check distance.** Push the robot exactly 48" by hand. Y on the screen
  should read about 48. If it doesn't, the gearing or `DRIVE_WHEEL_DIAMETER_IN` is wrong.
- [ ] **Track width.** Measure from the center of the left wheels to the center
  of the right wheels and set `TRACK_WIDTH_IN`. Then spin the robot in place
  exactly 5 times (1800°) and read H. If it isn't about 1800, set
  new width = old width × shown / 1800. Skip this if you have an inertial sensor.
- [ ] **Cascade cartridge.** It's set to red (`RATIO_36_1`) from the design
  doc. Check it, because a wrong cartridge makes all the heights wrong — and in
  **v2** it also sets `CASCADE_DEG_PER_SECOND` (600 for red, 1200 for green,
  3600 for blue), which is how v2 works out how far the arm moves in one pass.
- [ ] **Cascade presets and toggle angles.** These are placeholders. See [above](#set-the-cascade-heights-and-toggle-angles).
- [ ] **Motor test program.** Check every motor's port and direction. See [Motor test program](#motor-test-program).
- [ ] **Drive v2, on blocks.** Its ramp, stick curve, strain easing and cascade floor have never run on the real robot. Read [Testing safely](TESTING_SAFETY.md) and work through its list. Start with the arm **down**, because that is where the program learns "the bottom" from.
- [ ] **Measure the real currents.** Run the amps tool and write down what each mechanism actually draws, so v2's thresholds stop being guesses. See [Measuring the real currents](#measuring-the-real-currents).
- [ ] **Port 9.** Check the brain's Devices screen to see what's plugged in there.
- [ ] **Tune the PID** with `pid_test`.

## Troubleshooting

| Problem | Likely cause / fix |
|---|---|
| Robot spins when you push the stick forward | A drive motor is reversed wrong. Run the motor test program to find it. |
| In autonomous, it turns the wrong way or drives in circles | Same as above: the drive directions are wrong, so the heading is backwards. |
| Cascade or toggle motors buzz or fight | One motor of the pair is reversed wrong. Run the motor test program. |
| Motor test shows `MISSING` | No motor on that port. Check the cable, or fix the port number in both programs. |
| Cascade shows negative numbers or the presets are off | It wasn't all the way down when the program started. |
| Moves end early and short | They're timing out. Raise `kp` (see tuning), or pass a bigger `timeout_ms`. |
| Claw closes but stops before gripping | `CLAW_STALL_AMPS` is too low. Raise it a little. |
| Claw never stops squeezing until the timeout | `CLAW_STALL_AMPS` is too high. Lower it. |
| Claw motor gets hot holding a piece | Lower `CLAW_HOLD_VOLTS`. |
| Robot creeps with the sticks let go | Raise `DEADBAND`. |
| Autonomous doesn't run at all | Plain Run starts driver control. Use a competition switch, field control, or Timed Run. Also check `AUTON_ROUTINE` is spelled the same as in `ROUTINES`. |
| Drive v2 says `FLOOR` and the arm will not go down | That is the software limit working. The arm was not at its resting place when the program started, so the bottom was learnt from the wrong place. Restart with the arm down. |
| Drive v2 stops with an error about `max_torque` | Set `USE_TORQUE_LIMITS = False` at the top of `cascade_robot_drive_v2.py` and it will run without torque caps. Nothing else changes. |
| Drive v2's sticks feel too soft | Raise `FINE_TOP` towards `1.0` and lower `STICK_EXPO` towards `1.0`. See [Why drive v2 feels the way it does](CONTROL_FEEL.md). |
| Drive v2 eases the cascade down even when it is moving freely | `CASCADE_STRAIN_AMPS` is too low for this arm. Raise it. |
| Drive v2 says `BLOCKED` on a mechanism that is plainly moving | `BLOCKED_FRACTION` is too high for this motor, or `BLOCKED_LOOPS` is too short. Raise the fraction, lengthen the loop count, or set `USE_VELOCITY_CHECK = False` to go back to amps only. |
| Amps tool shows a peak at or over a threshold in v2 | The threshold is too low for this robot — v2 will ease the mechanism off while it is working normally. Raise the threshold just above the peak you measured. |
| Amps tool shows `0.00 A` while a motor is clearly turning | You are reading the wrong group, or the cable is loose. Run the motor test program first. |
| Amps tool shows a huge peak then the mechanism eases | You loaded it harder than the mechanism ever will be in a match. The useful number is the peak from a *normal* load, not from holding the arm still. |
| Drive v2 no longer eases off a mechanism that has stalled | Check `USE_VELOCITY_CHECK` is `True`. If it is, the motor is still turning faster than 25% of what was asked, so raise `BLOCKED_FRACTION`. |
| The `FULL STOP` screen shows but something still moves | That should be impossible: every ramp is pinned to zero and all five motors are told to stop. If it happens, something other than this program is writing to a motor. |
| The full stop button does nothing | `STOP_BUTTON` names a button that does not exist, or one the program already uses. The brain screen prints which name it could not find. Use any of A, B, X, Y, Left or Right. |
| The amps tool will not measure anything | The screen says which entry in `STOP_BUTTONS` is wrong. Use any of Up, Down, Left or Right. |

## How this matches the C++ code

| C++ (this repo) | Python (this folder) |
|---|---|
| `include/pid.hpp` `PIDController` | `class PID` |
| `DriveBase::drive_straight` | `drive_distance()` |
| `DriveBase::turn_to_heading` | `turn_to_heading()` |
| `Cascade` (presets, gravity feedforward) | `cascade = SyncedMechanism(...)` + `CASCADE_PRESETS` |
| `Toggle` (yellow / red / blue) | `toggle = SyncedMechanism(...)` + `TOGGLE_ANGLES` |
| `Claw::open` / `Claw::close` (stall detection) | `claw_open()` / `claw_close()` |
| Example auton state machine in `robot.cpp` | `auton_example()` |
| Ports in `src/main.cpp` | `# --- Devices` (real ports for this robot) |

Differences:

- **Odometry and `drive_to_point` / `turn_to_point`** are new here, so tracking wheels can be added later.
- **Each move has its own timeout**, and it returns whether it got there.
- **Driving backward works.** Distance is measured with its sign, so
  `drive_distance(-24)` finishes properly.
- **No D-term jump when a new move starts.** The first PID update skips the D term.
- **The cascade and toggle pairs are synced in autonomous too**, not just in driver control.

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
