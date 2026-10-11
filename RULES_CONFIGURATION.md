# V5RC 2026-2027 "Override" — rules and electronic configuration (PID-testing view)

Source: official game manual **Version 2.0** (129 pages, published 2026-09-03), downloaded to
`override-2.0.pdf` in the repository root (gitignored — VEX's copyrighted document, per the
existing "Local-only files" policy in the README). Plain-text extraction is `manual_full.txt`
(also gitignored). Re-download URL if the file is ever lost:

```
https://content.vexrobotics.com/docs/2026-2027/override/files/override-2.0.pdf
```

(The pattern is `docs/<season>/override/files/override-<version>.pdf`. Note: `www.vexrobotics.com`,
`vexforum.com`, `kb.vex.com` and `events.vex.com` are all Cloudflare-blocked from this network —
only `content.vexrobotics.com` direct file paths work. v2.0 is the newest: 1.0/2.1/3.0 all 404.)

This file is the committed summary. Rule text below is paraphrased except for short quoted
fragments; the manual remains the authority.

## 1. Match format

- 0:15 Autonomous Period, then 1:45 Driver Controlled Period. Endgame is the last 10 s.
- Scoring: stack Pins and Cups on Goals, set Toggles to your color, end in the Midfield.

## 2. Rules that constrain our robot and our PID work

| Rule | What it says (paraphrased) | Impact on testing |
|---|---|---|
| **<R3>** | Robot must fit an 18"×18"×18" volume at match start | Cascade must be fully down at start; software floor must be trustworthy |
| **<SG2>** | Never larger than 24"×24" horizontal footprint at any time, including while lifting | Cascade geometry + sway must not swing past 24" — a sway measurement matters for legality, not just stability |
| **<SG3>** | No part may exceed **50" height** at any time (Endgame exception in <SG12>) | Hard ceiling for cascade PID max-extension limit (currently 24" in `src/main.cpp` — measured arm reach must stay under 50" with margin) |
| **<R6>** | **Only one V5 Brain. No other microcontrollers** — explicitly including "Raspberry Pi or Arduino devices" — "even as non-functional decorations" | **The ESP32-C3 + MPU6050 rig is ILLEGAL on the robot during a match or inspection.** It is a practice/diagnostic instrument only and must be unbolted before any event |
| **<R9>** | Must use a Competition Template responding to field control | Driver-control-only programs are fine for practice; matches need the competition template |
| **<R10>** | Total motor power ≤ **88W** (counts every motor on the robot, even unplugged ones). Legal combos: 8×11W+0×5.5W, 7×11W+2×5.5W, 6×11W+4×5.5W, 5×11W+6×5.5W, 0×11W+16×5.5W. V5 Smart Motors on Smart Ports only; no motors on 3-wire ports | Our robot: 7×11W (4 drive + 2 cascade + 1 claw = 77W) + 2×5.5W toggle = **88W exactly — combination "7 + 2", legal but with zero headroom.** Adding any motor to the robot, even disconnected, breaks <R10> |
| **<R11>** | Subsystem 2 (drive base) ≤ 55W, and drive motors may not power anything outside Subsystem 1 | Drive = 4×11W = 44W, legal. Do not reuse a drive motor for a mechanism |
| **<R12>** | One V5 battery only | A sagging battery changes every measured number — start each test session on a charged battery |
| **<R15>** | Max 2 controllers. "Using sensor feedback to augment driver control (such as motor encoders or the Vision Sensor) is permitted" | **Closed-loop driver control / PID assist is explicitly legal.** Our whole approach is within the rules |
| **<R19>** | Non-VEX items allowed only on the enumerated list (lube, hot glue on cable connections, rope ≤1/4", cable management, rubber bands ≤7.5"×0.25", drawer liner, zip ties ≤12", **a micro SD card in the Brain**, freeze spray for cooling, cleaners) | SD-card telemetry logging is legal. The MPU rig is not covered by <R19> — see <R6> |
| **<SG1>** | Start stationary, 18" cube, one Preload | — |
| **<SG6>** | Max possession: 1 Pin + 1 Cup | Claw must not hold a second Pin while grabbing; a claw-hold PID test with two Pins is an illegal configuration even on the practice field |
| **<SG7>** | Don't cross the Autonomous Line in Auton | Only matters if/when an auton is added |

## 3. Port budget

The V5 Brain has **21 Smart Ports** (numbered 1–21; confirmed from PROS's own
`vdml/registry.h` "range of V5 ports (1-21)"). Current wiring uses 11 of them:

| Port | Device | Program constant |
|---|---|---|
| 11, 17 | Drive left (11W green, reversed) | `drive_left_front_11`, `drive_left_back_17` |
| 1, 10 | Drive right (11W green) | `drive_right_front_1`, `drive_right_back_10` |
| 13, 2 | Cascade (11W red 36:1, one shaft) | `cascade_left_13`, `cascade_right_2` |
| 16 | Claw (11W green, 1500 mm cable) | `claw_16` |
| 18, 8 | Toggle (5.5W, one shaft) | `toggle_18`, `toggle_8` |
| 6 | Communication device (not a motor) | ignore |
| 9 | **The radio** (confirmed by the team 2026-10-11) | ignore |

**There is no IMU on the robot.** Consequences:

- The PROS project has a live bug waiting: `src/main.cpp:134` sets `config.imu_port = 9`, and
  `src/robot.cpp:30` unconditionally constructs `pros::Imu(9)` — that would build a gyro object
  on the radio's port. `m_imu_ready` flips true on a 2.5 s timer, not on real calibration
  status, so nothing would hang; instead the drive would silently apply heading correction from
  garbage. **Fix `imu_port` / add an IMU-off path in the PROS config before that project is ever
  downloaded.** The Python programs are already safe (`IMU_PORT = None`).
- Every heading/tilt number in the tuning plan therefore comes from the MPU6050 rig (practice
  only, per <R6>) or from drive-motor encoder differences — never from a V5 gyro.

Free: **10 Smart Ports** (3, 4, 5, 7, 12, 14, 15, 19, 20, 21) — far more room than needed: a
V5 Inertial Sensor (276-8400) fits easily and it is the only legal gyro for matches. Plus the
ADI/3-wire ports.

## 4. Electrical/power budget relevant to current thresholds

- V5 11W motor: working current typically well under 2 A; the amps tool (`cascade_robot_amps.py`)
  exists to measure real peaks before any current threshold is trusted (`watch_current` ships off).
- `src/main.cpp` threshold placeholders: cascade strain 2.0 A (pair total), ease 1.0 A (per motor),
  relaxed 2.5 A (per motor); claw strain 1.5 A. **These are unmeasured guesses** — Stage 0 of the
  test plan replaces them with measured numbers.

## 5. What changed vs. the old research doc

`VEX_2026_2027_RESEARCH.md` was written from manual v1.0-era notes and is stale in three places:
its port table still says "TBD" (ports are now known, see §3 above); its ESP32 wiring section says
SDA=GPIO6/SCL=GPIO7 but the actual rig is **SDA=GPIO5, SCL=GPIO4** (`mpu6050_test/README.md` is
correct); and it has no <R6> warning about the ESP32's illegality on the robot.
