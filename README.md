# vex-pid

Control software for a VEX V5 Robotics Competition robot, plus the ESP32-based
IMU instrumentation used to characterise it.

The robot code is a [PROS 4](https://pros.cs.purdue.edu/) C++20 project built
around a single reusable PID controller driving four subsystems. The IMU test
rig is a separate ESP32-C3 firmware that fuses an MPU-6050 at 200 Hz and streams
telemetry to a browser dashboard — it exists to measure what the robot actually
does, independent of the V5 brain.

---

## Repository layout

```
include/            PROS robot — headers
  main.h              The include every PROS source starts with (see Building)
  pid.hpp             PIDController: P/I/D/F, clamped integral, settle timer
  robot.hpp           Robot — owns subsystems, IMU, logger, auton state machine
  control_feel.hpp    Stick deadband, two-zone curve, slew rate limit, arcade mix
  data_logger.hpp     CSV telemetry logger
  subsystems/
    drive_base.hpp      Differential drive + heading correction
    cascade.hpp         Linear cascade lift with gravity feedforward
    claw.hpp            Single-motor claw with stall detection
    toggle.hpp          3-position toggle (yellow / red / blue)

src/                PROS robot — implementation
  main.cpp            PROS entry points + all port/pin configuration
  robot.cpp           Subsystem ticks, driver control, autonomous routine,
                      emergency stop
  data_logger.cpp     Per-sample CSV writer
  subsystems/*.cpp    Subsystem implementations

tools/              Build-time helpers (not part of the robot)
  syntax_check.sh     Compile-checks every C++ file with clang, no PROS needed
  pros_stub/pros/     Stand-in PROS headers that make that possible

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
| `opcontrol()` | `driver_tick()` — controller input — plus `subsystems_tick()` |
| `autonomous()` | `auton_tick()` — state machine — plus `subsystems_tick()` |
| `disabled()` | `disabled_tick()` — stops every subsystem, resets auton |

All configuration lives in one place. `src/main.cpp` is the only file that
mentions port numbers, so changing wiring never means editing a subsystem.

### PID controller

`include/pid.hpp` is a single `PIDController` class shared by every subsystem:

- **P/I/D/F** terms, with `kF` applied to the *target* rather than the error
  (the standard velocity/gravity feedforward shape).
- **Clamped integral** — `integral_limit` is a symmetric bound on the
  accumulator, which is what stops integral windup during a long approach.
- **Output clamping** between `output_min` and `output_max` (default ±127).
- **Settle detection** — `is_settled()` returns true once the error has stayed
  inside `settle_error` for `settle_ticks` consecutive updates. Autonomous
  states advance on this signal instead of on a fixed sleep, so the routine
  self-adjusts to how the robot is actually moving.

Gains are plain `PIDGains` structs, so they can be tuned per subsystem without
touching the controller.

### Subsystems

**Drive base** (`drive_base`) — differential drive over a `pros::Motor_Group`
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

- Extension is clamped to `max_extension_inches`, and `manual_control`
  additionally refuses to drive further past a soft limit in either direction.
- `gravity_feedforward` is added to the PID output so the controller does not
  have to build integral error just to hold the lift up against gravity.
- Four named presets (`presets[4]`), selected by the controller's face buttons.
- The output is clamped **after** `gravity_feedforward` is added, and then cast
  to the `std::int8_t` that `Motor_Group::move()` takes. The PID's own output is
  already limited to ±127, so adding the 8.0 feedforward could reach 135 — which
  does not fit in an `int8_t`, which is undefined behaviour, and which on the ARM
  wrapped round to **−121**. Any saturating upward command, such as pressing a
  preset from rest, would have driven the arm **down** at nearly full power.
- A **software floor** at the bottom of the travel (`kCascadeFloorInches`,
  −0.02 in, matching the Python side's `CASCADE_LOWER_LIMIT` of −2.0° at 0.01
  in/degree) refuses a downward command once the arm is there. The arm has no
  physical stopper.
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

**Claw** (`claw`) — one motor, two end states.

`open()` and `close()` are not symmetric. Opening is timed (`open_time_ms`).
Closing watches current draw: once past `stall_check_after_ms`, a draw above
`stall_current_ma` means the claw has closed on something, so it backs off to
`hold_power` and reports done. `close_timeout_ms` (2 s) falls back to the same
holding power, so a missed detection cannot stall the motor indefinitely.

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
limiter. Only the ceiling is separate (`drive_speed_max` and `turn_speed_max`,
both 51 = 40% of 127), so turning can be made slower than driving without
touching the shape. All the numbers live in `config.drive` in `src/main.cpp`.

**Holding L2 stops every motor at once.** `L2` is the only button this layout
leaves free — L1 is the claw, R1 and R2 the cascade, A/B/X/Y the four presets and
Up/Down/Left the toggle — and it is a trigger, so both thumbs stay on the sticks
while it is held. Three details make it a stop rather than a suggestion:

- it is read **before** the sticks and before the IMU check, so it works from the
  moment the program starts, not only once the gyro has finished calibrating;
- `subsystems_tick()` is gated on it as well as `driver_tick()`. That matters
  because the PIDs are what actually write the motors: a stop that only zeroed
  the driver's own outputs would be overwritten by `m_cascade.update()` on the
  very next pass;
- every ramp is reset, so letting go drives on from zero instead of jumping
  straight back to whatever the sticks are asking for.

It works in autonomous too, and `stop_all()` — drive, cascade, claw, toggle — is
the single definition of "stopped", used by the stop button and by `disabled()`.

The Python **drive v2** goes further than this port does. It also watches each
mechanism's current *and* its encoder, eases a mechanism that is being held back,
caps the claw's torque, and keeps the cascade above a software floor — and it
times each pass round the loop rather than assuming 20 ms, so the floor's
prediction of where the arm will be still holds when the brain is busy. The C++
side here is the stick feel only. Two Python files back it up:
`vexcode-python/TESTING_SAFETY.md`, for testing it without breaking the robot,
and `vexcode-python/cascade_robot_amps.py`, a measuring tool that reports what
each mechanism really draws — worth running before trusting any of v2's current
thresholds, because every one of them was picked by hand.

### Autonomous

`robot.cpp` runs a ten-state sequence — `DriveToGoal1` → `ScorePreload` →
`DriveToPin` → `GrabPin` → `DriveToGoal2` → `ScorePin` → `DriveToMidfield` →
`Done` — advancing on `is_at_target()` from the relevant subsystem rather than
on fixed delays.

There is a **watchdog over the whole routine**, not just over its last state.
Every waiting state polls `is_at_target()`, and the only timeout used to live
inside `DriveToMidfield`: if an earlier target never settled, the sequence would
never reach it and the robot would drive at full power until the match ended.
Fifteen seconds after the routine starts, whatever state it is in, it calls
`stop_all()` and marks itself `Done`. Holding L2 gives up immediately.

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
> line of robot code. It has been written (`#pragma once` plus `pros/api.h`) and
> the project now gets past that point.

**Checking it without the PROS toolchain.** There is no VEX kernel and no ARM
toolchain on a Mac, so `tools/syntax_check.sh` compiles every C++ file with
clang against the stand-in headers in `tools/pros_stub/`, which declare the slice
of the PROS API this project actually uses:

```bash
./tools/syntax_check.sh
```

That **is** a real check of this project's code — it catches syntax errors,
typos, missing includes and type mistakes such as the `int8_t` overflow above,
and the whole project currently passes it clean under `-Wall -Wextra`. It is
**not** the real API: passing does not prove the project links or runs on the
brain, and only a build in PROS can tell you that.

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
