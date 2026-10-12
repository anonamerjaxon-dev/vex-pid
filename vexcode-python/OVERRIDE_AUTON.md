# Override Auton

Autonomous for V5RC **Override** (2026-27), plus driver control. It was
copied from the old `Cascade Robot Auton` program on 2026-10-10. Driver
control is the same as **Cascade Robot Drive Limits**: same buttons, same
stick curve, same cascade limits.

| File | What it is |
|---|---|
| `Override Auton.v5python` | Open this in VEXcode V5 and download it |
| `override_auton.py` | The same program as plain Python (easier to read) |
| `sync_files.py` | Keeps each `.py` and its `.v5python` the same (both programs in this folder): `python3 sync_files.py from-py` after editing the .py, `from-vexcode` after saving in VEXcode, `check` to compare |

The ports, the driver controls and Drive Limits are in this folder's
[README](README.md).

> **Not tested on the robot yet.** The route was checked on a computer
> simulation of the robot and field. Every mechanism number marked GUESS
> has to be measured first (see below).

## What the autonomous does

1. Squeezes the preload in the claw, so it can't slip, and flips the field
   **Toggle** behind the robot to our color, so the yellow pins in our quadrant
   score for us.
2. Lifts the **preload** and drives it to **our alliance goal**, then sets it
   down and lets go.
3. Grabs the **yellow pin-in-cup** two tiles out and stacks it on top of the
   preload.
4. Backs off and stops.

## Setting up on the field

```
            far side (other alliance)
   X:  -24      0       24       48
 Y=48 |  .   red/blue  YELLOW   our goal B
      |        pin     (cup)
 Y=24 | neutral    .   OUR GOAL   (old pin, not used)
      |  goal
 Y=0  |=========[ROBOT]=================  our alliance wall
               (Toggle is on this wall, right behind the robot)
```

- **Where:** in the middle of our alliance wall, centered on the Toggle.
- **Which way:** the **back** of the robot (the toggle arm) touches the wall,
  so the front (cascade and claw) faces into the field.
- **Cascade** all the way **down**.
- **Claw** **closed on the preload pin**.
- **Toggle arm** at rest.
- Don't let any part of the robot touch the field Toggle. The rules don't
  allow it at the start.

Every motor count starts at 0 when the program starts, which is why the
start position matters.

**Blue alliance:** set `ALLIANCE = "blue"` and start the same way in the
middle of the blue wall. The field is the same turned around, so the route
doesn't change. Only the Toggle turns the other way (see
`TOGGLE_FLIP_DEGREES_BLUE`).

## The route

Positions are in inches, measured from the middle of our wall's inside face:
- **X** goes to the right, as the drive team sees it.
- **Y** goes straight out into the field.

The program works out every turn and distance from the positions in Settings,
so if you correct a position, the route follows.

| Step | Move | Distance |
|---|---|---|
| 1 | Squeeze the preload; flip the Toggle (`TOGGLE_FLIP_DEGREES`) | |
| 2 | Straight out, level with the goal; lift the preload to `CASCADE_CARRY_DEG` | 15.25" |
| 3 | Turn right to 90°, drive until the pin is over the goal | 17.25" |
|   | Lower to `CASCADE_SET_DEG`, open the claw | |
| 4 | Back away; lower to `CASCADE_PICKUP_DEG` | 12" |
| 5 | Turn left to 38°, drive until the claw is at the yellow pin, close | 23.7" |
|   | Lift to `CASCADE_STACK_DEG` | |
| 6 | Back out along the same line | 23.7" |
| 7 | Turn right to 90°, drive until the stack is over the goal | 12" |
|   | Lower to `CASCADE_STACK_SET_DEG`, open the claw | |
| 8 | Back off and stop | 8" |

**Where the numbers come from.**
- **The robot** is 35 × 30 studs at 0.5" each: 17.5" front to back, 15" wide.
- **The claw** holds the pin at stud (15, 4), looking at the front: the middle,
  2" behind the front edge. That is **6.75" in front of the robot's center**, so
  every drive stops 6.75" short of the goal or pin.
- **The start:** the robot's center is 8.75" out from the wall.
- **The field:** the goal and the yellow pin sit on tile seams, from the game
  manual's field drawing and VEX's own VR field. Tiles are 24".

Two things in the route are on purpose:
- **Both placements on the goal come in along the same line from the left.**
  If the preload lands right, the stack will too.
- **Every in-place turn happens where the robot's corners clear everything.**
  The corners swing out 11.5" from the center.

## Measure these first

Use the **Drive Limits** program for all of them. It shows every motor's angle
live on the brain.

1. **Toggle flip:** `TOGGLE_FLIP_DEGREES`, a guess of 120.
   - Start Drive Limits with the toggle arm at rest.
   - Hold **Right** until the field Toggle shows red, and read Toggle 18 / 8.
   - Plus is the Right-button way.
2. **Cascade heights.** Start Drive Limits with the cascade down, hold each
   height with L1/L2, and read the C13 / C2 numbers on the controller.

   | Setting | Height | Now |
   |---|---|---|
   | `CASCADE_CARRY_DEG` | preload pin's bottom just above the goal, while driving in | 170 (20 above SET) |
   | `CASCADE_SET_DEG` | the preload drops onto the goal from here | **150, measured** |
   | `CASCADE_PICKUP_DEG` | claw at the height to grab the yellow pin-in-cup | 0, guess |
   | `CASCADE_STACK_DEG` | cup's bottom just above the preload pin on the goal | 380, guess |
   | `CASCADE_STACK_SET_DEG` | the stack drops onto the preload pin from here | 360, guess |

   The stack guesses come from the measured 150: the yellow pin-in-cup has to
   clear the top of the preload pin, about one pin higher.

   Keep them all between 0 and 730 (the cascade limit). The program won't go
   past 730 anyway.
3. **Drive distance:**
   - Set `AUTON_ROUTINE = "pid_test"` and run it.
   - "Forward 24"" must really be 24". If it goes much farther or shorter, the
     gear numbers (`DRIVE_MOTOR_GEAR_TEETH`, `DRIVE_WHEEL_GEAR_TEETH`) or the
     wheel size are wrong.
   - **Time it too.** See the 15-second warning below.
4. **Turning:** `TRACK_WIDTH_IN`, the center of the left wheels to the center of
   the right wheels.
   - If `pid_test`'s turns come up short or overshoot, measure it, then adjust
     it a little at a time.
   - There is no gyro (port 9 is the radio), so turns are worked out from the
     wheels.
5. **Field positions:** `GOAL_X/Y` and `STACK_X/Y`. Check them with a tape
   measure on your field, from the middle of the wall's inside face.

## Running it

**Plain Run starts driver control, not the autonomous.** That's how VEX's
competition template works: the autonomous only starts when something tells
the robot "the match has started". There are three ways to start it:

1. **Hold A for 1 second** (the easy way to test).
   - Start the program with plain Run.
   - Put the robot in its starting spot, cascade down, and close the claw on
     the preload with R2.
   - Hold **A** for 1 second. The controller buzzes and the autonomous runs
     right there.
   - Press **B** at any time to stop it.
   - When it ends, you have driver control again.

   A only does this off the field: with field control or a competition switch
   connected it does nothing, so it can't start in a real match. Set
   `AUTON_TEST_SECONDS = 15` to make the test stop at 15 seconds like a match.
2. **Timed Run** on the V5 controller.
   - Go to the program menu and pick this program.
   - Press the right arrow, choose **Timed Run**, then press **A**.
   - After a 3-second countdown it runs 15 s of autonomous, then 1:45 of
     driver control, like a real match.
3. **Field control** at a tournament (or a competition switch) starts it
   automatically.

What you'll see:
- During the autonomous, the brain shows X / Y / heading, the cascade and toggle
  angles, the time, and which step is running.
- When driver control starts, the bottom line of the brain says
  `Auton override done in X.X s`, `Auton cut off at: <step>` (the 15 s ran
  out) or `Auton stopped at ...` (you pressed B).

After each open, the claw holds where it is. After closing on the yellow pin,
it keeps squeezing so the pin can't drop.

Other routines, chosen with `AUTON_ROUTINE`:
- `"pid_test"`: forward 24", turn 90° and back, back 24".
- `"odom_test"`: a 24" square.
- `"none"`.

## ⚠ Will it fit in 15 seconds?

**Maybe not.** On the computer simulation, with the gearing written in the
vex-pid README (3.25" wheels, 18-tooth gear on the motor, 60 on the wheel,
green cartridge):

| Auton speed | Simulated time |
|---|---|
| 40% (the normal drive speed, the setting now) | **37.9 s** |
| 100% | 20.4 s |

With that gearing the robot can only do about 10 inches per second, and the
route is 112" of driving plus three turns.

If your robot is faster than that, it may fit. The brain shows the real time
at the end of a Timed Run. If it doesn't fit:
- raise `AUTON_DRIVE_SPEED` and `AUTON_TURN_SPEED`;
- or stop after the preload by deleting steps 4-8.

The 15 seconds end the routine wherever it is, and the driver screen says
which step it got to.

## Adding tracking wheels later

The position tracking already supports them:
1. Put the Rotation sensor ports in `VERTICAL_TRACKER_PORT` /
   `HORIZONTAL_TRACKER_PORT`.
2. Set their offsets from the robot's center.
3. Run `odom_test`.

The routine doesn't change. An inertial sensor (`IMU_PORT`) would make the
turns much more accurate.

## Troubleshooting

| Problem | Try |
|---|---|
| The autonomous doesn't run | Plain Run starts driver control. Hold A for 1 second, or use Timed Run (see Running it) |
| Holding A does nothing | A only works off the field. Unplug the competition switch, or use Timed Run |
| The program stops at start with `... is not allowed` | `AUTON_ROUTINE` or `ALLIANCE` is misspelt. The brain lists the words it accepts (capitals don't matter) |
| The claw drops the preload at the start | The claw's directions are swapped: the start squeeze opened it. Swap `CLAW_OPEN_DIRECTION` and `CLAW_CLOSE_DIRECTION` |
| The Toggle turns the wrong way | Flip the sign of `TOGGLE_FLIP_DEGREES` |
| The Toggle doesn't turn far enough, or turns back | Measure `TOGGLE_FLIP_DEGREES`. If driving away drags the Toggle back, tell us: the arm folds back after the first drive |
| The pin misses the goal | Check the `pid_test` distance first, then `GOAL_X/Y` on your field |
| The claw opens when it should close | Swap `CLAW_OPEN_DIRECTION` and `CLAW_CLOSE_DIRECTION` (the same setting as Drive Limits). Since 2026-10-10: open = REVERSE, close = FORWARD, checked on the robot |
| It hits the goal while turning | The start position is off. The back must touch the wall, centered on the Toggle |
| A motor shows UNPLUGGED in driver control | Check that motor's cable (the claw is on the long one) |
