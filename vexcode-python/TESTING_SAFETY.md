# Testing safely

Written 2026-10-08, before the first run of **drive v2**.

Be honest with yourself about where things stand:

- **v1 was measured on the real robot.** The ports, the directions and the
  four drive signs in this folder were checked against the actual machine.
- **v2 has never run on the robot.** Its ramp, its stick curve, its strain
  easing, its torque limits and its software floor are all new. They were
  tested against a pretend motor on the laptop, which proves the *logic* is
  right - it proves nothing about how the real hardware feels.
- **The current numbers are guesses.** `CASCADE_STRAIN_AMPS`,
  `CASCADE_EASE_AMPS`, `TORQUE_FULL_AMPS`, `CLAW_STRAIN_AMPS` and
  `CLAW_HOLD_AMPS` were all picked by hand. Nobody has measured what this robot
  draws. Run the amps tool (below) before you rely on any of them.

So treat the first v2 run as a first run, not as a formality.

**Learn the stop before anything else moves.** In drive v2, **hold B**: every
motor on the robot stops at once, whichever way the sticks are leaning. In the
amps tool, hold any d-pad button for the same thing. Both are the first thing
the program looks at, so they work even while something else is happening. Press
it once with the wheels off the floor, before you need it, so you already know
it works.

## The rules

1. **Wheels off the floor first.** Blocks, a box, an upside-down crate -
   anything that lets the wheels spin free. Lift the arm clear of the bench
   and of its own chain. Go to the floor only once the controls do what you
   expect on the bench.
2. **Start slow and stay slow for a while.** Every speed in the drive
   programs starts at 40, and 40 is a *cap*, not a target. Raise **one**
   number, by about 10, drive it, and only then raise another.
3. **One change at a time.** Change one constant, re-download, test, then
   change the next one. If you change two things and it misbehaves, you have
   no way to tell which one did it.
4. **Keep hands clear of the arm and the claw** whenever the battery is
   plugged in, even when nothing is moving. A stalled motor is silent.
5. **Charge the battery.** A tired battery makes the motors weak and the
   robot unpredictable, and it is the single most common cause of "my code
   got worse on its own".
6. **Ask before anything risky.** Cutting, drilling, unplugging a port,
   changing a gear, deleting a program that works - ask first, and take a
   backup first. The `versions/` folder and the git tags are there for this.

## v2 has one clever thing and it needs one thing from you

v2 has a **software floor** on the cascade. The arm has no physical stop, so
the program remembers where the arm started and refuses to drive it down past
that point (plus a couple of degrees of slack). Over the last 25 degrees it
comes down at a crawl instead of at full speed, so it can rest right on the
limit rather than a full-speed step above it, and the screen says `FLOOR` when
it gets there.

That only works if the arm is **already sitting at its resting place when you
press Run**, because pressing Run is what teaches the program where "the
bottom" is (`cascade.reset_position()` on startup).

> Put the arm down at the bottom before you press Run. If you start with the
> arm half way up, the program will treat *that* as the bottom, and the arm
> will not be allowed to come down any further.

## If something goes wrong

- **Stop it:** **hold B** (drive v2) or any d-pad button (amps tool). Every motor
  stops while it is held, and letting go puts you back where you were without a
  jump. **Press the brain's stop button, or turn the robot off**, if that is
  closer - the controller does nothing once the program stops, so those always
  work too.
- **It says `FLOOR` and will not go further down** - that is the floor doing
  its job, not a fault.
- **The arm is hot, or a motor is whining and not moving** - stop, and lower
  `CASCADE_SPEED`, or lower `CASCADE_STRAIN_AMPS` so the easing comes on
  sooner.
- **It says `BLOCKED` on the screen** - the program has decided that mechanism
  is being held back: the motor is being told to turn and is not turning, even
  if the current looks normal. That is usually right. If the mechanism really
  is moving fine, raise `BLOCKED_FRACTION` (25% by default) or set
  `USE_VELOCITY_CHECK = False` to go back to watching amps only.
- **It stops with an error about `max_torque`** - some motors may not accept
  the torque ceiling. Open `cascade_robot_drive_v2.py`, set
  `USE_TORQUE_LIMITS = False`, and it will run without torque caps. Nothing
  else changes.
- **The claw squeezes too hard, or not hard enough** - that is
  `CLAW_HOLD_AMPS`. Lower it to grip more gently, raise it to grip harder.
  `CLAW_STRAIN_AMPS` is the point where it decides it is holding something.

## Measure the currents first

Every current number in v2 was picked by hand, so run
`Cascade Robot Amps.v5python` on the bench before you trust one. It is a
measuring tool, not a driving program: hold a button and it spins one mechanism
at a time and shows what it really draws. The full instructions are in
[Measuring the real currents](README.md#measuring-the-real-currents).

It has **no torque ceiling, on purpose** - a ceiling is a clamp, and once a
motor hits it the reading stops climbing, which would hide the very number you
came for. That is also why each test is only a few seconds. Two things to keep
safe while you use it:

- keep the runs short (the program does this for you) and let the motor rest
  between them;
- never wedge a mechanism so hard that the motor truly cannot turn, then walk
  away. Load it with your hand until the reading climbs, then let go - the
  motor stops the moment you release the button.

If a peak you measure is already at or above one of v2's thresholds, that
threshold is too low and v2 will ease the mechanism off while it is working
normally. Raise it just above the peak you saw.

## The order to test in

1. On blocks, with the amps tool: measure what each mechanism really draws, and
   fix any threshold that is below the peak you saw.
2. On blocks: does the drive go forwards when you push the stick forwards?
3. On blocks: does it turn the way you expect?
4. On blocks: claw open and close, then grip a soft object and watch that it
   holds without the motor getting hot.
5. On blocks: toggle up and down.
6. On blocks: drive the arm all the way down with L2 and confirm it stops
   with `FLOOR` on the screen and does **not** climb past its start.
7. On blocks: hold L1 against something (a hand on the arm is enough) and
   confirm the speed eases and the arm does not fight you.
8. On the floor, at 40, in a clear space. Then start raising numbers.

Write down what you saw after each step. If it is not in a note, it did not
happen.
