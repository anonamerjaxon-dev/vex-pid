# Testing safely

Read this before the robot moves for the first time.

Where things actually stand (updated 2026-10-11):

- **Two programs have run on the real robot:** plain drive v1 (ports,
  directions and the four drive signs checked on 2026-10-08) and **drive live
  angles** (measured the cascade travel, 750°, and the claw angles, −16 open to
  176 closed, on 2026-10-10). Everything those runs measured is recorded in the
  code as constants.
- **Drive v2 and the amps tool have never run on the robot.** Their logic was
  tested against a pretend motor on a laptop, which proves the logic is right
  and proves nothing about how the hardware feels.
- **The PROS C++ project has never even been downloaded** to the brain. Its
  compile checks pass (`tools/syntax_check.sh`) and its stick-curve logic has
  45 laptop tests (`tools/feel_test.sh`), and that is all. Its stop button
  (`A`) has never been pressed on a robot.
- **Every current number was a reasonable guess typed at a desk.** Measure
  them with the amps tool before trusting any of them.

So the first run of anything untried is a test, not a match. The staged
procedure, thresholds and abort rules live in
[`PID_TEST_PLAN.md`](PID_TEST_PLAN.md); the hard limits for now are **start
every motor at 10%, and nothing above 50% until each stage is verified.**

---

## The rules

1. **Wheels off the floor first.** Prop the robot up so the drive wheels spin
   free, and keep the arm clear of anything it could hit. A stuck wheel or a
   jammed arm at full power is how parts break.
2. **Never use very high speed while you are still learning what the robot
   does.** The ceilings are deliberately low for this reason: **start every
   motor at 10%, and nothing above 50%** until the matching stage of
   `PID_TEST_PLAN.md` is verified. The drive programs cap at 40%
   (`DRIVE_SPEED` / `TURN_SPEED`), the mechanisms at 55% — treat those as the
   tops, not targets. Raise one number by about ten at a time, never all of
   them at once, and test after each change.
3. **One change at a time.** If you change the stick curve and the ramp on the
   same run, and it feels wrong, you will not know which one did it.
4. **Keep your hands, sleeves and cables clear of anything that can move** when
   you press Run, and keep watching while it runs. A PROS program keeps running
   until it is stopped.
5. **Start with a charged battery.** A tired battery sags under load, the
   motors behave differently, and every number you measure will be wrong.
6. **Ask before anything risky.** If you are about to try something you have
   not seen this robot do — a new mechanism, a faster speed, a longer move —
   that is the moment to stop and ask.

## Learn the stop before anything else moves

There is one stop per program, and they are **not** the same button:

| Program | Stop |
|---|---|
| Drive v2 | **hold B** — every motor stops, latches while held |
| Amps tool | **hold any d-pad button** (Up/Down/Left/Right) |
| Drive live angles | **none.** It has only its soft limits (`AT LIMIT` on the screen). The brain's stop button, or switching the brain off, is the stop — know where your thumb is |
| PROS C++ (never downloaded) | `A`, latched on/off — untested |

Press the stop before you press anything else, with the wheels off the floor,
and watch that the motors really go quiet. Then release it and watch the screen
clear.

> **Drive live angles is the current driver and it has no program stop.** That
> is a known gap: its soft limits stop the cascade/claw/toggle at measured
> angles, but nothing in the loop stops the *drive*. Keep the brain's own stop
> within reach while driving it.

The full control map is in the [README](README.md) **Controls** section: left
stick drives and turns, L1/L2 cascade up and down, R1/R2 claw (one press each
in live angles), Right/Y or Up/Down toggle. Every mechanism button is held, not
latched, so letting go lets the ramp ease the mechanism down.

The screen readouts worth watching are per-program: drive live angles shows
every motor's **live angle** with its limits (`AT LIMIT` when a soft limit is
holding), and drive v2 shows the strain guard's work (`BLOCKED` when a
mechanism is told to move and is not). Watch them when you do step 6.

## Put the arm down first

The cascade lift has **no physical stopper**. Driven far enough down, the chain
can come off the sprocket. Two different guards exist for that, and each one
only works from the right starting place:

- **Drive v2's software floor** refuses to command the arm below where it
  started, so the arm has to be sitting at its natural resting place before you
  press Run. If it is halfway up when the program starts, "the bottom" is
  wherever it happened to be, and the floor is in the wrong place.
- **Drive live angles** has measured limits instead (`CASCADE_TRAVEL`, 750°,
  with a 25° margin): they count from the position at startup for the same
  reason — start with the arm down.

The same is true of the profile: `initialize()` tares the encoder, so the
starting position is defined as zero on every run, not just the first one after
a power cycle.

## The order to test in

1. **Wheels off the floor, robot on, learn the stop.** With drive v2, hold B —
   nothing moves and the screen says `STOP`; release and you are driving again.
   With drive live angles there is no program stop, so find the brain's own stop
   first. Prove the stop before you need it.
2. **Check each mechanism alone, slowly.** One button at a time, at a distance,
   with nothing in the way. In drive live angles, does the claw open on R1 and
   close on R2 (one press each)? Does the cascade go up on L1 and down on L2?
   Does the toggle go both ways (Right / Y)?
3. **Measure the currents.** Every threshold in both the Python programs and
   `src/main.cpp` was picked by hand. Use `vexcode-python/cascade_robot_amps.py`
   (procedure in [`vexcode-python/README.md`](vexcode-python/README.md) under
   **Measuring the real currents**). Hold a button, load the mechanism gently by
   hand, write down the peak, then set each threshold above the worst normal
   peak and below the fault.
4. **Drive with the wheels still off the floor.** Small stick movements. Does it
   start slowly and only reach its top speed at full stick, or does it jump?
5. **Clear a large empty space and drive on the floor,** at 40% and never above
   the 50% ceiling until each stage in `PID_TEST_PLAN.md` is verified. Keep a
   stop within reach — in drive live angles that is the brain itself.
6. **Test the strain handling by hand** (drive v2 only). Gently hold a mechanism
   back and see that it eases and stops pushing rather than grinding. Never wedge
   a mechanism so hard the motor truly cannot turn, and never walk away from one.

There is no autonomous routine to test — the auton program was removed from the
folder (it is in git history, and its speed presets were wildly unsafe anyway;
see `PID_TEST_PLAN.md` §5).

## If something goes wrong

- **Use that program's stop** (table at the top of this file). If it is drive
  live angles, or the stop does not respond, **turn the brain off.** A motor
  with a shorted or miswired connection can be driven by the motor controller
  itself, so the brain off is the only stop that always works.
- Then **write down what you saw**: which mechanism, which button, what it did,
  and whether the brain screen said anything. "The arm went down when I pressed
  up" is a fixable bug report. "It glitched" is not.

## What has been checked, and what has not

| Checked | How |
|---|---|
| The code parses and type-checks against the real PROS 4.1.0 API | `./tools/syntax_check.sh` — 0 errors, 0 warnings in this project's own files |
| The stick curve, the rate limiter and the strain guard behave | `./tools/feel_test.sh` — 45 checks, 0 failed, including a sweep proving the command never exceeds its ceiling, and the encoder-only guard |
| The Python drive v2 logic | a fake-VEX bench harness: the ramp, the curve, the floor, the strain and encoder guards, and the full stop |
| The Python amps tool | a fake-VEX harness: the readings, the button handling and the stop |
| Ports, directions and drive signs (v1) | measured on the real robot, 2026-10-08 |
| Cascade travel (750°) and claw angles (−16…176) | measured on the real robot, 2026-10-10 |

| **Not** checked | Why it matters |
|---|---|
| That the PROS project **links** | only a build in PROS can do that — and `imu_port = 9` must be fixed first (that is the radio port) |
| Drive v2 / amps tool / drive live angles **logic** on the real robot | v2 and amps have never run there; live angles drove but its limits are one measurement |
| The C++ amps measuring program | it has been compile-checked but never executed; its Python twin is bench-tested |
| The current thresholds in `src/main.cpp` and in v2 | they were picked by hand; measure them |
| The **speed** numbers | 40% / 35% are estimates of what is controllable, not measurements |
| The tuning gains (`PIDGains`, `gravity_feedforward`) | never observed on hardware |

---

## The short version

Wheels off the floor. Learn the stop before anything moves — and remember drive
live angles has no program stop, so the brain's stop is yours. One thing at a
time. Start at 10%, never above 50% until a stage is verified. Put the arm down
before you start. Keep your hands clear. Measure the currents before trusting
them. And ask before anything you have not seen the robot do before.
