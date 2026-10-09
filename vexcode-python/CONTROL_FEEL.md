# Why drive v2 feels the way it does

The request was: make the controls feel good, look up how people who do this
properly solve it, give me two speeds - the fastest only at full stick and
something precise everywhere else - non-linear but with no sudden jump, and
make every mechanism behave that way. Plus a cascade that notices it is
straining, and a claw that grips without burning out.

This is what the research said and where each idea ended up in the code.

## 1. Do not smooth the signal - limit how fast it can change

The important source is the WPILib documentation for their **slew rate
limiter** (the library FRC teams use for exactly this):

> a simple low-pass filter is poorly-suited for this job; while a low-pass
> filter will soften the response of an input stream to sudden changes, it
> will also wash out fine control detail and introduce phase lag. A better
> solution is to limit the rate-of-change of the control input directly. This
> is performed with a slew rate limiter - a filter that caps the maximum
> rate-of-change of the signal.
>
> <https://docs.wpilib.org/en/stable/docs/software/advanced-controls/filters/slew-rate-limiter.html>

Plain English: **do not average the stick, cap how fast the number is allowed
to climb.** Averaging feels laggy and mushy; rate-limiting feels crisp but
never jerky.

The same page adds a rule that is easy to get wrong:

> each input stream requires its own filter object. Do not attempt to use the
> same filter object for multiple input streams.

So v2 keeps **five separate ramp values** - `ramped_left`, `ramped_right`,
`ramped_cascade`, `ramped_claw`, `ramped_toggle` - one per thing you control.
Sharing one would make turning fight driving.

In v2 this is `ramp_towards()` and `RAMP_PER_SECOND = 250`. At a 20 ms loop
that is `RAMP_PER_LOOP = 5.0` percent per loop, so any motor takes about
0.4 seconds to go from stopped to full. Turning the ramp down makes the robot
calmer; turning it up makes it snappier.

## 2. Two rates and an exponential curve - the radio-control trick

Every hobby transmitter has two knobs for this, and they do the two different
jobs that were asked for:

- **Dual rate** caps how far the channel travels. Low rate = the stick can
  never ask for full power. High rate = full power is available.
- **Exponential** ("D/R & EXP" on a Futaba-style menu) bends the response so
  small stick movements do very little and the last part of the travel does a
  lot. Middle of the stick = precise. End of the stick = fast.

That maps exactly onto "the maximum only at full stick, precise everywhere
else, non-linear but no sudden jump", so v2's stick curve is those two ideas
in one function:

```
stick_shape(value, ceiling):
    the first FINE_END (85%) of the travel is the fine zone,
    and it only reaches FINE_TOP (40%) of the ceiling there,
    bent by STICK_EXPO (2.0) so the middle is especially gentle;

    the last 15% of the travel spends the other 60% of the ceiling.
```

Because both halves meet at exactly `FINE_TOP`, there is **no step** where
they join - that was the "it does not suddenly become the speed that is so
fast" requirement. And because the whole curve is scaled by `ceiling`, the
full ceiling is reached at, and only at, the stop.

Measured from the real function, with the ceiling at 40:

| stick | power | share of the ceiling |
|------:|------:|-----:|
| 0%    | 0.00  | 0% |
| 5%    | 0.00  | 0% (inside the deadband) |
| 10%   | 6.05  | 15% |
| 25%   | 6.83  | 17% |
| 50%   | 10.22 | 26% |
| 75%   | 16.22 | 41% |
| 85%   | 19.35 | 48% (end of the fine zone) |
| 90%   | 25.68 | 64% |
| 95%   | 32.84 | 82% |
| 100%  | 40.00 | 100% |

Half stick giving a quarter of the power is the whole point: that is where
you spend your time when lining the robot up.

`MIN_MOVE_FRACTION = 0.15` is there so the gentlest touch that leaves the
deadband still moves the robot - a curve that starts at zero is technically
smooth and practically useless.

## 3. Magnetic keyboards - respond the instant something changes

Hall-effect keyboards (the magnetic, analog ones) are famous for feel, and
the ideas behind that are worth stealing:

- **Actuation point** - how far you press before it registers at all. Too
  far in and the keyboard feels dead; too far out and it fires when you
  breathe on it.
- **Rapid trigger** - as soon as you release past a small distance, the key
  re-arms, so the next tap needs almost no travel. You never wait through
  dead travel.
- **Per-key curves** - each key can have its own response, because a movement
  key and a lean key want different things.

The transferable lesson is the second one: **never make the driver push
through dead travel.** So v2's `DEADBAND = 5` is small (it exists only to
stop a worn stick from creeping), and everything past it responds
immediately - `apply_deadband()` rescales the stick so the edge of the
deadband is 0 and full stick is still 100. The deadband costs you a few
percent of travel at the very bottom and nothing anywhere else.

The third idea is why the x-axis and y-axis are shaped separately and only
then mixed by `arcade()`: driving straight and turning are different jobs and
deserve their own curves.

## 4. Sensing strain - read the current, cap the torque

VEX gives two tools for this, both on the motor object:

- `motor.current(CurrentUnits.AMP)` - how many amps it is pulling right now.
  On a `MotorGroup` this **adds up every motor in the group** (the SDK stub is
  explicit about it: "Returns the total current all motors are using"), which
  is what you want for a two-motor arm.
- `motor.set_max_torque(amps, CurrentUnits.AMP)` - a ceiling on how hard it
  is allowed to push. On an 11W motor you can also give it NM or a percent;
  on a 5.5W motor it has to be amps. On a `MotorGroup` this one is **per
  motor**, not a group total - it loops over the motors and sets the same
  ceiling on each.

That difference is easy to trip over, so v2's constants say which is which:
`CASCADE_STRAIN_AMPS` is the pair added together, while `CASCADE_EASE_AMPS`
and `TORQUE_FULL_AMPS` are the ceiling for each motor.

v2 wraps those in `class StrainGuard`:

```
watch(asked):
    if it is straining for CASCADE_STRAIN_LOOPS loops in a row:
        ease the speed down towards CASCADE_EASE_SPEED (35%)
        and drop the torque ceiling to CASCADE_EASE_AMPS (1.0 A)
    otherwise:
        let the speed recover, a little at a time
        and go back to the full ceiling (2.5 A)

    return asked * factor
```

Two details that matter:

- The easing is **gradual in both directions** (`CASCADE_EASE_STEP` and
  `CASCADE_RECOVER_STEP`), so the arm does not stutter as it comes in and out
  of a strain.
- `set_cap()` only talks to the motor **when the number actually changes**.
  Setting the same torque limit 50 times a second is pointless work in a loop
  that has to run every 20 ms.

### The other half: watch the encoder

Amps are honest, but they are not complete. A current reading tells you a motor
is working hard; it does not tell you a motor is being **held**. A chain
starting to drag, a game element wedged somewhere soft, or a bearing going dry
can all hold a mechanism back while the current stays perfectly ordinary. That
case is invisible to an amp threshold no matter how carefully the threshold is
chosen.

So the same guard also reads the encoder:

- `motor.velocity(VelocityUnits.PERCENT)` - how fast the motor is **really**
  turning, in percent of its own top speed. Note this is a different unit from
  the `PERCENT` used for power: it is `VelocityUnits.PERCENT`, not
  `PercentUnits.PERCENT`. On a `MotorGroup` it reports the **first motor of the
  group** rather than a sum, which is what lets it be compared straight against
  the commanded percent with no gear maths in the way.

The rule is "told to spin, and not spinning": if the speed is under
`BLOCKED_FRACTION` (25%) of what was asked for, for `BLOCKED_LOOPS` (6) loops
in a row, that counts as strain on its own. Either symptom is enough to start
the easing.

Three details make it safe rather than twitchy:

- The comparison is against the command the motor was **actually given**, after
  any easing has been applied - not the original button value. If it used the
  button value, easing the speed down would make the arm look *more* blocked,
  which would ease it further, and the guard would chase itself into a stall it
  had invented. Comparing against the real command lowers the bar as the speed
  comes down, and the two settle.
- `BLOCKED_LOOPS` is deliberately **longer** than the amp debounce. A motor
  takes a moment to spin up to what it was asked for, and that perfectly normal
  pause must not be read as a fault.
- `BLOCKED_MIN_ASK` skips commands under 5%, where a speed reading is mostly
  noise, so small deliberate crawls are left alone.

The floor needed one more rule. When the arm is resting **on** the limit the
program is deliberately sending zero while the button is still held, so the
guard is told - through `clear_block()` - that this is a chosen stop and not a
mechanism in trouble. Without that, sitting on the floor would slowly ease the
arm for no reason.

`USE_VELOCITY_CHECK = False` turns the whole thing off and leaves amps only.

The claw uses the same class with different numbers: a permanent
`CLAW_HOLD_AMPS = 1.2` ceiling so it can never crush anything or cook itself,
and it eases its closing speed to 35% once amps pass `CLAW_STRAIN_AMPS`,
which is how it decides "I am holding something now". It gets the encoder
check too, and there it is arguably the better of the two signals: a claw that
has closed on an object is a motor that has been told to turn and has stopped,
whatever it happens to be drawing.

**These amp numbers are the least certain part of v2.** They were not
measured on your motor - the VEX knowledge-base page with the real
stall-current figures would not load (403), and the BLRS wiki page on stall
detection timed out. They are guesses that are in the right region, written
as named constants at the top of the file so you can adjust them the first
time you feel the robot. If the easing never comes on, `CASCADE_STRAIN_AMPS`
is too high. If it comes on while the arm is moving freely,
it is too low.

The way out of the guessing is the **amps tool**
(`cascade_robot_amps.py` / `Cascade Robot Amps.v5python`). It spins one
mechanism at a time and shows what it really draws, with no torque ceiling on
purpose - a ceiling is a clamp, and once a motor hits it the reading stops
climbing. Hold a button, load the mechanism by hand, and read the peak off the
screen. Then put that number in the constants: a measured figure cannot be
worse than the one that is there now.
[Measuring the real currents](README.md#measuring-the-real-currents) has the
full instructions, including the one trap worth repeating here: a group's
`current()` adds its motors **up** while `set_max_torque()` applies to **each**
one, so the cascade reading is a pair total, and `CASCADE_EASE_AMPS` - which is
per motor - has to be compared with the per-motor figure the tool shows in
brackets beside it.

The encoder half exists partly because of that uncertainty. Its numbers are
fractions of a command rather than amps, which is a far easier thing to guess
correctly, so it catches the case the amp threshold gets wrong. Between them,
v2 does not stand or fall on one unmeasured figure.

## 5. The stop button is the one thing that beats everything

Everything else in this file is about making the robot feel good. This part is
about making it stop, and it is the only part where "feels nice" is not the
question.

There is a real temptation in a program like this to handle the stop as one more
input: read the sticks, read the buttons, and let the stop fall out of the same
logic. That is the wrong shape. Rank the inputs, and give the stop the top rank:

- it is read **first**, before the sticks and before anything else;
- while it is held, the sticks cannot command anything, however far over they
  are — the ramps are pinned to zero rather than merely not updated;
- the motors are told to stop **once**, on the way in, rather than fifty times a
  second, so the stop is a deliberate event and not a stream;
- it reaches **every** motor, including ones this run has not switched on. A
  stop that only stops what you were looking at is not a stop;
- it clears the strain guards, because being held still on purpose is not a
  mechanism in trouble, and a guard that reports a fault because you asked the
  robot to stop is a guard you will learn to ignore.

Two consequences worth spelling out. **The ramps are brought down, not left
where they were**, which is what makes letting go safe: the robot eases away
from zero instead of jumping back to full stick, so the stop is not itself a
hazard. And **a test that is cut short is not recorded**: in the amps tool a
stopped run leaves no peak, because a half-finished reading that gets written
down as a measurement is worse than no measurement.

The other half of this is the mistake it replaces. The button is looked up once,
when the program starts, by name. Get the name wrong and you get a message on
the brain screen and a robot that will not drive — which is the safe way to be
wrong. Looking it up inside the loop, which is where it used to live, turns the
same typo into an exception thrown in the middle of a run.

## 6. What v2 deliberately does not do

- **No PID.** Asked for, and v1 did not have it either. PID makes a mechanism
  hit an exact position; it does not make a joystick feel nice. It is the
  next step, not this one.
- **No low-pass filter anywhere.** Section 1 is the reason.
- **No auto-tuning.** Every number here is a starting point to be felt and
  adjusted, and they are all at the top of the file in one block with a
  comment each, so that is easy.
