# Cascade Robot: VEXcode Python handout

This folder is the cascade robot's program in **VEXcode V5 Python**. It has
the same PID setup as the PROS C++ code in the rest of this repo, rewritten so
it can be opened, edited and downloaded straight from VEXcode.

There are two programs:

1. **Match program** (`Cascade Robot Auton`), used in competition:
   - **Driver control**: split arcade drive, cascade, claw and toggle.
   - **Autonomous**: PID drive and turns, cascade/toggle PID, claw grip.
   - **Position tracking (odometry)**: works now with the drive motor encoders.
     When tracking wheels are added, you change a few settings and nothing else.
2. **Motor test program** (`Cascade Robot Test`), used for checking wiring.
   Each button spins **one** motor, in exactly the direction the match program
   uses, so you can check every port and direction one at a time.

> **Status:** so far this has only been tested on a computer simulation of the
> robot, not on the real robot. Go through [Before the first real run](#before-the-first-real-run)
> first.

## Contents

- [Files](#files)
- [Putting it on the robot](#putting-it-on-the-robot)
- [Controls](#controls)
- [Ports](#ports)
- [Drive program](#drive-program)
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
- [How this matches the C++ code](#how-this-matches-the-c-code)
- [Change log](#change-log)

## Files

| File | What it is |
|---|---|
| `Cascade Robot Auton.v5python` | **Match program.** Open this in VEXcode. |
| `cascade_robot_auton.py` | The match program as a plain Python file, so it can be read and reviewed on GitHub. |
| `Cascade Robot Test.v5python` | **Motor test program.** Open this in VEXcode. |
| `cascade_robot_test.py` | The motor test program as a plain Python file. |
| `Cascade Robot Drive.v5python` | **Drive program (no PID).** Open this in VEXcode. |
| `cascade_robot_drive.py` | The drive program as a plain Python file. |
| `sync_files.py` | Copies changes between each `.py` and its `.v5python` (runs on your computer, not the robot). |
| `README.md` | This handout. |

## Putting it on the robot

1. Open **VEXcode V5**, then **File → Open** and pick a program:
   - `Cascade Robot Drive.v5python` — the plain drive program. No PID, nothing to
     calibrate, so **start here**.
   - `Cascade Robot Auton.v5python` — the match program: driver control plus a
     PID autonomous.
   - `Cascade Robot Test.v5python` — the motor test, for checking wiring.
2. Plug in the brain (or the controller, with the brain paired), pick a slot and press **Download**.
   Put each program in a different slot, so all three are on the brain.
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

It also reads the drive a little differently from the match program: the
cascade and the toggle are plain `MotorGroup`s, so both motors of a pair just
get the same command. The match program does more than that - it compares the
two motors' positions and slows the one that is ahead until the other catches
up.

There is nothing to set up before running it. The brain screen shows the drive
power and what each mechanism is doing, and the controller screen shows the
short version.

> In the VS Code project, the file that gets downloaded to the brain is
> `Cascade_Robot_Test/src/main.py`. Put whichever program you want to run into
> that file - it is a plain copy of the `.py`.

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
- **Joystick deadzone:** `DEADBAND`. Raise it if the robot creeps when the sticks are let go.
- **Buttons:** in `driver_control()`, change `controller_1.buttonL1` and the
  others. The buttons are `buttonL1`, `buttonL2`, `buttonR1`, `buttonR2`,
  `buttonUp`, `buttonDown`, `buttonLeft`, `buttonRight`, `buttonA`, `buttonB`,
  `buttonX` and `buttonY`.
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
command does both programs:

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
  doc. Check it, because a wrong cartridge makes all the heights wrong.
- [ ] **Cascade presets and toggle angles.** These are placeholders. See [above](#set-the-cascade-heights-and-toggle-angles).
- [ ] **Motor test program.** Check every motor's port and direction. See [Motor test program](#motor-test-program).
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
