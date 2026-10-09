# Testing safely

Read this before the robot moves for the first time.

None of the code in this repository has ever run on a real robot. Not the
autonomous routine, not the driver control, not the strain guard, not the
software floor — nothing. It has been compiled and type-checked against the real
PROS API (`tools/syntax_check.sh`) and the pure logic has been tested on a laptop
(`tools/feel_test.sh`), and that is genuinely all that is known. Everything else
— every speed, every ramp rate, every current limit — was a reasonable number
typed at a desk.

So the first run is a test, not a match.

---

## The rules

1. **Wheels off the floor first.** Prop the robot up so the drive wheels spin
   free, and keep the arm clear of anything it could hit. A stuck wheel or a
   jammed arm at full power is how parts break.
2. **Never use very high speed while you are still learning what the robot
   does.** The ceilings are deliberately low for this reason: drive is 40% of
   full and turn is 35% (`config.drive.drive_speed_max` and `turn_speed_max` in
   `src/main.cpp`). Raise one of them by about ten at a time, never all of them
   at once, and test after each change.
3. **One change at a time.** If you change the stick curve and the ramp on the
   same run, and it feels wrong, you will not know which one did it.
4. **Keep your hands, sleeves and cables clear of anything that can move** when
   you press Run, and keep watching while it runs. A PROS program keeps running
   until it is stopped.
5. **Start with a charged battery.** A tired battery sags under load, the
   motors behave differently, and every number you measure will be wrong.
6. **Ask before anything risky.** If you are about to try something you have
   not seen this robot do — a new mechanism, a faster speed, a longer
   autonomous step — that is the moment to stop and ask.

## Learn the stop before anything else moves

The stop is **button A**, and it stops every motor at once: drive, cascade, claw
and toggle. Press it before you press anything else, with the wheels off the
floor, and watch that the motors really go quiet.

The two other stop controls, for the Python programs, are in
[`vexcode-python/TESTING_SAFETY.md`](vexcode-python/TESTING_SAFETY.md): **B** in
drive v2, and **any d-pad button** in the amps measuring tool.

The full control map is in the **Driver feel** section of the
[README](README.md): left stick throttle, right stick turn, L1/L2 cascade up and
down, R1/R2 claw close and open, B/Y toggle, A stop. Every mechanism button is
held, not latched, so letting go lets the ramp ease the mechanism down.

## Put the arm down first

The cascade lift has **no physical stopper**. Driven far enough down, the chain
can come off the sprocket. The software floor (`config.cascade.floor_inches`) is
what prevents that, and it works by refusing to command the arm below its
starting position — so the arm has to be sitting at its natural resting place
before you press Run. If it is halfway up when the program starts, "the bottom"
is wherever it happened to be, and the floor is in the wrong place.

The same is true of the profile: `initialize()` tares the encoder, so the
starting position is defined as zero on every run, not just the first one after
a power cycle.

## The order to test in

1. **Wheels off the floor, robot on, press A.** Nothing should move. This proves
   the stop before you need it.
2. **Check each mechanism alone, slowly.** One button at a time, at a distance,
   with nothing in the way. Does the claw open when you press R2 and close on R1?
   Does the cascade go up on L1 and down on L2? Does the toggle go both ways?
3. **Measure the currents** before trusting any current threshold. Every number
   in `src/main.cpp` with `_amps` in it was picked by hand. The Python tool
   `vexcode-python/cascade_robot_amps.py` is what turns those guesses into
   measurements; the procedure is in
   [`vexcode-python/README.md`](vexcode-python/README.md) under **Measuring the
   real currents**.
4. **Drive with the wheels still off the floor.** Small stick movements. Does it
   start slowly and only reach its top speed at full stick, or does it jump?
5. **Clear a large empty space and drive on the floor.** Keep a finger over A.
6. **Test the strain handling by hand.** Gently hold a mechanism back and see
   that it eases and stops pushing rather than grinding. Never wedge a mechanism
   so hard the motor truly cannot turn, and never walk away from one.
7. **Only then the autonomous routine**, in as much space as you can find, with
   a finger over A the whole time.

## If something goes wrong

- **Press and hold A.** It stops every motor at once, and it is read before
  anything else in the driver loop, so it works even if the gyro is still
  calibrating.
- If a motor keeps running anyway, the program is not the thing in control —
  turn the brain off. A motor with a shorted or miswired connection can be
  driven by the motor controller itself.
- Then **write down what you saw**: which mechanism, which button, what it did,
  and whether the brain screen said anything. "The arm went down when I pressed
  up" is a fixable bug report. "It glitched" is not.

## What has been checked, and what has not

| Checked | How |
|---|---|
| The code parses and type-checks against the real PROS 4.1.0 API | `./tools/syntax_check.sh` — 0 errors, 0 warnings in this project's own files |
| The stick curve, the rate limiter and the strain guard behave | `./tools/feel_test.sh` — 37 checks, 0 failed |
| The Python drive v2 logic | a fake-VEX bench harness: the ramp, the curve, the floor, the strain and encoder guards, and the full stop |
| The Python amps tool | a fake-VEX harness: the readings, the button handling and the stop |

| **Not** checked | Why it matters |
|---|---|
| That the PROS project **links** | only a build in PROS can do that |
| That anything works on the **real robot** | no version of this code has ever run |
| The current thresholds in `src/main.cpp` | they were picked by hand; measure them |
| The **speed** numbers | 40% / 35% are estimates of what is controllable, not measurements |
| The tuning gains (`PIDGains`, `gravity_feedforward`) | never observed on hardware |

---

## The short version

Wheels off the floor. Press A and confirm nothing moves. One thing at a time.
Never very high speed. Put the arm down before you start. Keep your hands clear.
Measure the currents before trusting them. And ask before anything you have not
seen the robot do before.
