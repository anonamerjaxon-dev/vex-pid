# vex-pid

Control software for a VEX V5 Robotics Competition robot, plus the ESP32-based
IMU instrumentation used to characterise it.

The robot code is a [PROS 4](https://pros.cs.purdue.edu/) C++20 project built
around a single reusable PID controller driving four subsystems. The IMU test
rig is a separate ESP32-C3 firmware that fuses an MPU-6050 at 200 Hz and streams
telemetry to a browser dashboard — it exists to measure what the robot actually
does, independent of the V5 brain.

> **Before anything moves: read [TESTING_SAFETY.md](TESTING_SAFETY.md).** Every
> number in this robot — speeds, ramps, current limits — was picked at a desk. It
> has never been run. The note is the wheels-off-the-floor, raise-one-thing-at-a-
> time procedure, and it is the point where you should stop and ask.

---

## Repository layout

```
include/            PROS robot — headers
  main.h              The include every PROS source starts with (see Building)
  pid.hpp             PIDController: P/I/D/F, clamped integral, settle timer
  robot.hpp           Robot — owns subsystems, IMU, logger, stop screen
  amps_tool.hpp       The amps measuring program (see Measuring the currents)
  control_feel.hpp    Stick deadband, two-zone curve, slew rate limit, arcade mix
  strain_guard.hpp    Current + encoder strain sensing, easing and current limits
  data_logger.hpp     CSV telemetry logger
  subsystems/
    drive_base.hpp      Differential drive + heading correction
    cascade.hpp         Linear cascade lift with gravity feedforward
    claw.hpp            Single-motor claw with stall detection
    toggle.hpp          3-position toggle (yellow / red / blue)

src/                PROS robot — implementation
  main.cpp            PROS entry points, the amps-tool switch + all port/pin
                      configuration
  robot.cpp           Subsystem ticks, driver control, current readout,
                      emergency stop
  amps_tool.cpp       The amps measuring program
  data_logger.cpp     Per-sample CSV writer
  subsystems/*.cpp    Subsystem implementations

tools/              Build-time helpers (not part of the robot)
  get_pros_headers.sh Downloads the real PROS headers into .pros_headers/
  syntax_check.sh     Compile-checks every C++ file with clang, no PROS needed
  feel_test.sh        Builds and runs feel_test.cpp on this machine
  feel_test.cpp       40 checks of the stick curve and the strain guard
  pros_stub/pros/     Fallback stand-in headers, copied from the real API

TESTING_SAFETY.md     Read before the first run of anything

mpu6050_test/       ESP32-C3 + MPU-6050 IMU dashboard (PlatformIO)
README.md             Wiring, CSV format, pattern-detection thresholds
dashboard.html        Browser UI: Web Bluetooth / Web Serial, Three.js orientation
src/motion_detect.h   Pattern detection engine — portable C++, no Arduino deps
test/test_motion.cpp  Host test harness (10 scenarios, 38 assertions)

VEX_2026_2027_RESEARCH.md   Design notes: hardware, encoder math, PID tuning

vexcode-python/     The same robot in VEXcode V5 Python (the team's handout)
  cascade_robot_auton.py      Match program: driver control + PID autonomous
  cascade_robot_test.py       Motor test — one button per motor, for wiring
  cascade_robot_drive.py      Drive v1, the known-good backup
  cascade_robot_drive_v2.py   Drive v2 — stick feel, strain sensing, soft floor
  cascade_robot_amps.py       Amps measuring tool — what each mechanism really draws
  *.v5python                  The files VEXcode opens (same programs)
  sync_files.py               Copies each .py into its .v5python
  README.md                   The handout: controls, ports, edits, troubleshooting
  TESTING_SAFETY.md           Read before the first run of anything
  CONTROL_FEEL.md             Why drive v2 feels the way it does
```

---

## The robot (PROS)

### Architecture

Everything hangs off a `Robot` object configured in `src/main.cpp` and ticked
from the PROS entry points:

| PROS entry point | What it ticks |
|---|---|
| `initialize()` | Builds `RobotConfig`, calls `robot.initialize(config)` |
| `opcontrol()` | `driver_tick()` — controller input — plus `subsystems_tick()`, or the amps measuring program when `kRunAmpsTool` is set |
| `autonomous()` | Nothing. This project is driver control only — see below |
| `disabled()` | `disabled_tick()` — stops every subsystem, clears the stop latch |

All configuration lives in one place. `src/main.cpp` is the only file that
mentions port numbers, so changing wiring never means editing a subsystem.

### There is no autonomous routine

`autonomous()` is deliberately empty. The entry point still exists because PROS
needs it to link, but nothing in this project runs a timed sequence — it is
driver control only.

What survives from the autonomous work is the subsystems' own positional moves
(`move_to_preset()`, `home()`, the claw's one-shot `open()`/`close()`), because
they are part of the mechanism APIs. Nothing calls them from the main loop, and
the ten-state `AutonState` machine, its 15-second whole-routine watchdog and
`auton_tick()` have all been deleted.


### PID controller

`include/pid.hpp` is a single `PIDController` class shared by every subsystem:

- **P/I/D/F** terms, with `kF` applied to the *target* rather than the error
  (the standard velocity/gravity feedforward shape).
- **Clamped integral** — `integral_limit` is a symmetric bound on the
  accumulator, which is what stops integral windup during a long approach.
- **Output clamping** between `output_min` and `output_max` (default ±127).
- **Settle detection** — `is_settled()` returns true once the error has stayed
  inside `settle_error` for `settle_ticks` consecutive updates. The subsystem
  positional moves use this signal instead of a fixed sleep, so a move
  self-adjusts to how the robot is actually moving.

Gains are plain `PIDGains` structs, so they can be tuned per subsystem without
touching the controller.

### Subsystems

**Drive base** (`drive_base`) — differential drive over a `pros::MotorGroup`
per side, using the V5 IMU for heading.

- `drive_straight(inches)` runs the straight PID on average wheel position
  while a second PID holds heading, with the correction clamped to
  `max_heading_correction`. The two are mixed and renormalised so neither side
  ever exceeds full power.
- `turn_to_heading(degrees)` runs a turn PID on IMU heading with wrap-aware
  error, i.e. turning from 350° to 10° takes the 20° path, not the 340° one.
- `manual_control(throttle, turn)` bypasses PID entirely. The raw sticks go
  through the stick curve in `include/control_feel.hpp`, then split arcade
  mixing, then a slew rate limiter — see **Driver feel** below. The old
  `turn * 0.7` scaling that used to sit in `robot.cpp` is gone; the curve does
  that job now, and does it in the right place.

Encoder conversion is explicit: wheel circumference from diameter, then
`revolutions × 360 × gear_ratio`. With a 3.25" wheel and the 60/18 gear ratio
that is a known 117.55 encoder degrees per inch, which is where the drive
tuning starts from.

**Cascade lift** (`cascade`) — a linear cascade driven through a position PID
on extension in inches.

- Extension is clamped to `max_extension_inches`.
- `gravity_feedforward` is added to the PID output so the controller does not
  have to build integral error just to hold the lift up against gravity.
- Four named presets (`presets[4]`). They are **autonomous only** now — the
  driver drives the arm with L1 and L2 instead, so as far as driver control is
  concerned they no longer exist, but `move_to_preset()` is still the shortest
  way to say "go to the scoring height" in a routine.
- The output is clamped **after** `gravity_feedforward` is added. The PID's own
  output is limited to ±127, so adding the 8.0 feedforward could reach 135.
- A **software floor** at the bottom of the travel (`config.cascade.floor_inches`,
  −0.02 in, matching the Python side's `CASCADE_LOWER_LIMIT` of −2.0° at 0.01
  in/degree) refuses a downward command once the arm is there. The arm has no
  physical stopper: driven past the bottom the chain can come off the sprocket,
  and that is not something a match can recover from.
- Approaching the floor the arm is asked for `creep_speed_percent` (10) instead
  of `manual_speed_percent` (40), over the last `creep_band_inches` (0.25 in), so
  it can settle onto the floor rather than stopping a full-speed step above it.
  When the floor does stop it, the ramp is **pinned to the crawl** rather than
  left where it was or zeroed: leaving it let the arm set off again at whatever
  number it happened to be holding, and zeroing it made the ramp start over from
  nothing and push the arm down again, which stutters.
- How far the arm will move in one pass is **measured, not assumed**: the driver
  loop reads `pros::millis()` and passes the real elapsed seconds in, and
  `CASCADE_DEG_PER_SECOND`-style arithmetic turns the command into inches of
  travel. A pass that overruns — the brain is busy, the screen is being redrawn —
  would otherwise move the arm further than the ramp believed, which is exactly
  how the floor gets overshot.
- A **strain guard** (`strain_guard.hpp`, shared with the claw) watches the
  current *and* the encoder on every manual pass. Over `guard.strain_amps` for
  `strain_loops` passes, or told to move and not moving for `blocked_loops`
  passes, it eases the command back (down to `guard.ease_speed`, never to zero)
  and lowers the per-motor current limit to `guard.ease_amps`. It recovers more
  slowly than it eases. It judges the *eased* command, not the raw one, so it
  cannot chase itself into a stall it invented; and parking on the floor calls
  `clear_block()`, because being stopped on purpose is not a mechanism in
  trouble.
- `initialize()` **tares the encoder**, so "the bottom" means the same thing on
  every run and not just the first run after a power cycle. The arm has to be
  resting at its bottom when the program starts.
- `home()` drives down until sustained current draw indicates a hard stop, then
  tares the encoder — the zero point is found, not assumed. It is bounded by a
  `kHomeTimeoutMs` wall-clock timeout as well as by the stall detector, because a
  slipping chain or an unplugged motor never registers the stall and the loop
  would otherwise run for ever.
- `stop()` re-points the position PID at wherever the arm actually is. Setting
  only the target field was not enough: `update()` runs the PID, and the PID
  still held its old target, so the next tick drove the arm back towards it.

**Claw** (`claw`) — one motor, two end states, and a held-button grip.

For the driver: **R1 closes, R2 opens, both held**, through the same
manual path as the cascade — a ramp, a gentle percentage, and the strain guard.
A claw closed on a game object is a motor that has been told to turn and has
stopped, which is exactly what the guard's encoder half is for: it eases the
squeeze back and caps the current, so the claw holds firmly without cooking the
motor over a two-minute match. That is the "limit the grip but do not burn the
motors" behaviour, and it needs no threshold picked by hand — the encoder half is
a fraction of what was asked for, not an amp figure.

For autonomous, `open()` and `close()` are one-shot and not symmetric. Opening is
timed (`open_time_ms`). Closing watches current draw: once past
`stall_check_after_ms`, a draw above `stall_current_ma` means the claw has closed
on something, so it backs off to `hold_power` and reports done. `close_timeout_ms`
(2 s) falls back to the same holding power, so a missed detection cannot stall the
motor indefinitely.

`open()` and `close()` also **set `m_is_open` themselves**. `update()` branches
on that flag, so leaving it over from the previous action ran the wrong branch:
the first press after boot ran the *closing* code, and a claw asked to open could
end up commanded at full power into its hard stop. The flag records where the
claw is, and it is deliberately **not** flipped when an action finishes.

Elapsed time comes from `pros::millis()`. It used to be a count of `update()`
calls multiplied by 10, which is only the same thing if the loop always runs at
exactly 100 Hz — and it does not, so a slow pass stretched both the open time and
the squeeze timeout in real time.

**Toggle** (`toggle`) — a 3-position mechanism (yellow / red / blue) positioned
by a PID on motor encoder angle, with a `ToggleState` enum so callers ask for a
colour rather than a raw angle.

For the driver it is **B one way, Y the other, held** — a plain ramped
percentage, deliberately with no stick curve and no strain guard. It is a "go
that way" button rather than a speed control, and it has hard stops at both ends
of its travel. Entering the manual path adopts the current angle as the PID's
target, so letting go does not snap it back to wherever the PID last aimed.

`stop()` re-points the position PID at the current angle for the same reason the
cascade does: zeroing the motors alone left the PID aiming at its old target, so
the very next `update()` drove the toggle straight back — which is why it came
back to life on its own after `disabled()` was called.

### Driver feel

`include/control_feel.hpp` holds the whole feel of manual driving. It has no
PROS dependency at all, so it can be read and compiled on a laptop, and it is
ported number-for-number from the Python drive program
(`vexcode-python/cascade_robot_drive_v2.py`). `vexcode-python/CONTROL_FEEL.md`
is the write-up of why each piece is there.

- **Deadband** — `6.35`, which is 5% of a stick, with the value *rescaled* past
  it so that the edge of the deadband reads 0 and full stick reads 127. Without
  the rescale there would be a step at the edge, which is the jolt the deadband
  was meant to avoid.
- **A two-zone curve** (`stick_shape`) — gentle over the first `fine_end` (85%)
  of the travel, reaching only `fine_top` (40%) of the ceiling, then straight
  out to the ceiling over the last 15%. The two halves meet exactly, so there
  is no step anywhere. `min_move_fraction` (15%) is the slowest a moving stick
  ever asks for, so the robot always starts moving.
- **Split arcade** (`arcade`) — forward ± turn, with anything past the ceiling
  shared across both wheels rather than clipped off one. Clipping one wheel
  would make the robot drag sideways.
- **A slew rate limiter** (`rate_limit`) — every output may only change by
  `ramp_percent_per_second` (250% of full scale per second), so nothing can
  lurch. There is one limiter per wheel: sharing a single limiter between them
  would let a hard turn ration the other wheel. This is rate limiting, not
  smoothing — averaging the stick would soften it but also wash out the fine
  detail and add lag.

The throttle and the turn stick go through the **same** curve and the same
limiter. Only the ceiling is separate (`drive_speed_max` = 51, 40% of 127, and
`turn_speed_max` = 44, 35%), so turning can be made slower than driving without
touching the shape. All the numbers live in `config.drive` in `src/main.cpp`.

**Why this is the shape the driver asked for.** Press the stick a little and the
robot moves slowly and precisely; push it all the way to the stop and only then
does it reach the ceiling. That is the two-zone curve above, not the ramp: the
ramp only limits how fast the command may *change*, so it decides how quickly the
robot accelerates for a given stick position. The curve decides which stick
position means which speed. A lightness of touch that used to mean 100% of the
ceiling now means well under half of it.

**The ceiling is structural, not a clamp bolted on.** `stick_shape()` ends in
`return sign * power * ceiling`, with `power` built from a normalized magnitude
that is pinned to 1.0, so the answer *cannot* exceed the ceiling. `arcade()` then
shares any excess between the wheels rather than clipping one, which would drag
the robot sideways. `tools/feel_test.sh` proves both on this machine: every one
of 601 stick positions from well past one end to well past the other, at both
ceilings (51 and 44), and the biggest answer anywhere is exactly the ceiling —
plus a printed table of the whole curve. The same test measures how evenly the
travel is used. Over the first **85% of the stick** the command rises from 15% to
48% of the ceiling — that is the precise part — and the last 15% spends the
remaining half, reaching 64% of the ceiling at 90% of the stick. If that top end
turns out to be too sudden to drive, `stick.fine_end` is the knob: raising it
moves where the fast zone starts.

**The controls.** The drive project and the Python drive v2 now have the same
map:

| Input | Action |
|---|---|
| Left stick up/down | Throttle |
| Right stick left/right | Turn |
| L1 / L2 | Cascade up / down, held |
| R1 / R2 | Claw close / open, held |
| B / Y | Toggle one way / the other, held |
| **A** | **Full stop — press once to stop, press again to drive** |
| X, Up, Down, Left | Unused |

Every one of the mechanism buttons is **held, not latched**: letting go passes a
demand of zero, and the ramp eases the mechanism down from wherever it was rather
than stopping it dead. The arm, the claw and the toggle hold their position when
the button comes off, because their motors are in brake-hold mode.

**Pressing A stops every motor, and it stays stopped when you let go. Press it
again to drive.** A is deliberately the one button that is not next to a
mechanism control, and it can be hit with a thumb without letting go of a stick.
It is a **latched toggle**, not a hold, so the driver does not have to keep a
finger on it while they sort out whatever went wrong. Four details make it a
stop rather than a suggestion:

- it is read with `get_digital_new_press()`, which is **edge triggered**: holding
  A produces exactly one press, so the robot cannot flicker between stopped and
  driving under a resting thumb;
- it is read **before** the sticks and before the IMU check, so it works from the
  moment the program starts, not only once the gyro has finished calibrating;
- `subsystems_tick()` is gated on it as well as `driver_tick()`. That matters
  because the PIDs are what actually write the motors: a stop that only zeroed
  the driver's own outputs would be overwritten by `m_cascade.update()` on the
  very next pass;
- `stop_all()` is called on **both** edges — stopping and handing control back —
  so every ramp is zeroed and the robot eases up from nothing instead of jumping
  straight back to whatever the sticks were asking for during the stop.

The brain says `FULL STOP` in red, and `press A to drive`, for as long as the
latch is on. Going into `disabled()` clears the latch, so being disabled never
leaves the robot stopped for the next enable.

`stop_all()` — drive, cascade, claw, toggle — is the single definition of
"stopped", used by the stop button and by `disabled()`.

Both the cascade and the claw also watch themselves while the driver is holding a
button, through `strain_guard.hpp`: if a mechanism draws too much current, or is
told to move and does not, the command is eased back and the per-motor current
limit is lowered. The top two lines of the brain screen show what each guarded
mechanism is drawing and how far it has been eased (`casc 12.34 A  ease  35%`,
and `BLOCKED` when it is being told to move and not moving), so the automatic
slow-down can be watched rather than taken on trust.

The Python drive v2 does the same thing with the same numbers;
`vexcode-python/CONTROL_FEEL.md` is the write-up of the reasoning.

### Measuring the currents

Every current threshold in the project — `guard.strain_amps`, `guard.ease_amps`,
`guard.relaxed_amps`, the claw's `stall_current_ma` — was picked by hand. To
measure what the mechanisms really draw, set `kRunAmpsTool = true` in
`src/main.cpp` and download: `opcontrol()` then runs `amps_tool()` instead of
the driver program.

| Button | Test |
|---|---|
| hold A | cascade, lifting up |
| hold B | claw, closing |
| hold X | toggle, turning |
| hold Y | drive, forwards |
| hold any d-pad | stop everything, at once |

Let go and that mechanism stops at once; hold past five seconds and the test
ends by itself. The screen shows now / peak / steady, the peak is kept in the
table on the idle screen, and a per-motor figure is bracketed after the total
because `MotorGroup::current()` **adds its motors up** — the pair total is what
compares with `guard.strain_amps`, while `guard.ease_amps` is per motor. The
tool deliberately sets no current ceiling on the motors: a ceiling is a clamp,
so once a motor reaches it the reading stops climbing and "working hard" cannot
be told apart from "about to stall". It opens the claw's and the cascade's own
limit to 2500 mA — the motor's maximum — for the measurement and puts the
configured value back afterwards.

The Python half of this is `vexcode-python/cascade_robot_amps.py`, which does
the same thing on the same ports; `vexcode-python/README.md` has the fuller
write-up.

### Telemetry

`data_logger` writes a 14-column CSV to `/usd/vex_log.csv` at a configurable
rate (50 Hz here). It samples drive position/velocity/current, cascade
position/current, claw current, toggle position, IMU heading/accel/gyro, motor
temperature and battery voltage. `start_session()` opens the file and defers the
header until the first sample, so a session that never starts leaves no empty
file with a misleading header.

### Building

This is a standard PROS 4 project (`project.pros`, kernel 4.1.0, target
`cortex-a9`, C++20). Open the folder in the PROS editor and build, or with
`PROS_DIR` set:

```bash
make
```

The `Makefile` compiles `src/*.cpp` and `src/**/*.cpp` with `-std=c++20
-Wall -Wextra -O2`.

> **`include/main.h` was missing, so this project had never compiled.** Every
> PROS source starts with `#include "main.h"`, and the file did not exist here,
> in the old Desktop copy, or on GitHub. The compiler stopped on
> `src/main.cpp:1` with `fatal error: 'main.h' file not found` before reading a
> line of robot code. It has been written and now gets past that point. It also
> had the wrong include in it at first: it said `#include "pros/api.h"`, and no
> such header exists. The kernel ships a **top-level** `api.h` — its own
> `main.h` template is `#define PROS_USE_LITERALS` then `#include "api.h"` — and
> the file here is now the same shape.

**Checking it without the PROS toolchain.** There is no VEX kernel and no ARM
toolchain on a Mac, so the check works in two steps.

First, get the real headers. `tools/get_pros_headers.sh` downloads the PROS
4.1.0 source and keeps its `include/`:

```bash
./tools/get_pros_headers.sh      # default tag 4.1.0; pass another if you like
```

They land in `tools/.pros_headers/` and are git-ignored — they are somebody
else's source, not ours.

Then `tools/syntax_check.sh` compiles every C++ file in `src/` with clang against
**those real headers**:

```bash
./tools/syntax_check.sh
```

It falls back to the stand-in headers in `tools/pros_stub/` if you have not
fetched the real ones, and says so loudly when it does:
`NOTE: this was NOT checked against the real PROS API.` The current state is
**0 errors, 0 warnings in this project's own files against the real PROS 4.1.0
headers** (the only output is warnings inside the kernel's own headers, e.g. an
unused parameter in `llemu.h`).

> **A correction to an earlier claim.** The first version of this check used only
> hand-written stand-in headers, and one of them had been written to match what
> this project *called* — it declared `class Motor_Group` and
> `move(std::int8_t)`. A check written from the code it is checking cannot catch
> the code being wrong, and this one did not: the real kernel has **no**
> `Motor_Group` (it is `MotorGroup`, in `pros/motor_group.hpp`) and
> `move()` takes a `std::int32_t`. Every use of the old name was a compile error
> on the real robot. The stand-in headers have been rewritten from the real API
> and the script now prefers the real ones, so this cannot happen silently again.
>
> The same mistake produced a wrong story about an `int8_t` overflow: a
> saturating 135 into `move()` does **not** wrap round to −121. `move()` takes an
> `int32_t` and the kernel clamps the value to ±127, so it means "full speed",
> as intended. The explicit `std::clamp` in the PID paths is worth keeping as
> hygiene, but it was never the bug it was described as.

`tools/syntax_check.sh` proves this project's own code parses, includes what it
uses, and type-checks against the real API. It does **not** link, and it does not
run on the brain: only a build in PROS can tell you that.

`tools/feel_test.sh` is the other half — a host test of the pure-logic headers
(the stick curve and the strain guard) with 37 assertions, which is how the
curve's numbers below were measured:

```bash
./tools/feel_test.sh
# → 37 checks, 0 failed
```

> **Note on port numbers.** `src/main.cpp` is the only file that mentions ports.
> The assignment there is the one measured on the real robot: drive left 11/17,
> drive right 1/10, cascade 13/2, claw 16, toggle 18/8. Port 6 is a
> communication device rather than a motor, and port 9 is where an unidentified
> module sits, so the IMU is not confirmed and `has_imu` stays off for now.
> Section 9 of `VEX_2026_2027_RESEARCH.md` tracks the assignment.

---

## The IMU rig (ESP32-C3 + MPU-6050)

See **[`mpu6050_test/README.md`](mpu6050_test/README.md)** for the full write-up:
wiring, the CSV field list, every detection threshold, and troubleshooting.

The short version: an ESP32-C3 reads the IMU at 200 Hz, runs a Madgwick AHRS
**and** motion-pattern detection on the microcontroller, and streams CSV at
33 Hz over USB serial or Bluetooth LE to a browser dashboard with Web Bluetooth,
Web Serial, a Three.js orientation view, a gyro chart, and a live pattern panel.

Two design points worth calling out:

- **Detection runs on the ESP32, not in the browser.** The board streams at
  33 Hz, so a 20 ms impact would be smeared out of existence if detection
  happened in JavaScript. It is evaluated at the full 200 Hz sample rate.
- **There is deliberately no position readout.** Position needs acceleration
  integrated twice; a consumer accelerometer's ~10 mg bias becomes 1.8 m of
  phantom travel after six seconds. That is arithmetic, not tuning, and no
  filter removes it because real slow motion and sensor bias share a frequency
  band. Orientation, tilt, rotation rate, jerk, vibration and discrete events
  are all solidly measurable and are what the dashboard shows.

The detection engine lives in `src/motion_detect.h` with **no Arduino
dependencies**, so it is tested on the host rather than on the board:

```bash
cd mpu6050_test
c++ -std=c++17 -O2 -o /tmp/test_motion test/test_motion.cpp && /tmp/test_motion
# → 38 passed, 0 failed
```

To flash and run: `flash.command` (builds and uploads, waiting for the correct
serial port because this machine also has a Bluetooth speaker that enumerates as
one), then `start_dashboard.command` (serves the folder on `localhost:8080` —
`http://localhost` is a secure context, so both Web Bluetooth and Web Serial
behave predictably, unlike `file://`).

---

## Design notes

`VEX_2026_2027_RESEARCH.md` is the working design document: drivetrain and lift
geometry, encoder math, the PID architecture, autonomous planning, and the port
table.

---

## Local-only files

Two large assets are intentionally **not** in this repository and are gitignored:

| File | Size | Why excluded |
|---|---|---|
| `276-9250-000 (2026-04-26).STEP` | ~126 MB | CAD model; too large to be worth versioning here |
| `override-2.0.pdf` | ~10 MB | VEX's copyrighted game manual — redistributing it is not appropriate |
| `manual_full.txt` | ~236 KB | Plain-text extraction of the above |

The game manual is available from VEX directly. `VEX_2026_2027_RESEARCH.md`
summarises the design-relevant conclusions from it and stands on its own.
