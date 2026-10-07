# VEX V5RC 2026-2027: "Override" — Research & Design Doc

---

## Table of Contents

1. [Robot Hardware](#1-robot-hardware)
2. [Game Overview](#2-game-overview)
3. [Motor Data Reference](#3-motor-data-reference-pros-api)
4. [PID Architecture](#4-pid-architecture)
5. [Encoder Math](#5-encoder-math)
6. [Data Logging Architecture](#6-data-logging-architecture-v5--esp32-mpu6050)
7. [Autonomous Planning](#7-autonomous-planning)
8. [Code Architecture](#8-code-architecture)
9. [Port Assignments](#9-port-assignments-tbd)

---

## 1. Robot Hardware

### 1.1 Drive Base

```
Per-side wheel layout (6 wheels total):
  [Omni 3.25" powered] ─ [Friction idler (drop-center)] ─ [Omni 3.25" powered]

Gearing per side:  Motor [18T] → [60T] → Wheel
                    3.333:1 reduction
```

| Property | Value |
|----------|-------|
| Chassis | 35-hole × 20-hole C-channel |
| Drive motors | 4× 11W V5 Smart Motor (cartridge: 200 RPM / green) |
| Wheel diameter | **3.25 inches** (Omni, all powered wheels) |
| Wheel circumference | 3.25 × π = **10.210 inches** |
| Gear ratio (motor:wheel) | 60T/18T = **3.333 : 1** |
| Encoder degrees per wheel rev | 360 × 3.333 = **1200.0** |
| Inches per encoder degree | 10.210 / 1200 = **0.00851** |
| Encoder degrees per inch | 1200 / 10.210 = **117.55** |
| Wheel RPM (at shaft) | 360 RPM |
| Motor RPM (at shaft) | 360 × 3.333 = **1200 RPM** |
| Drivetrain power | 4× 11W = **44W** (limit: 55W) |

### 1.2 Cascade Lift

```
Linear cascade, pulley-driven, 2 strings per side:

     ┌─────────────┐
     │ Stage 4     │  ← Tip (MPU6050 placement position 3)
     │ 20-hole     │
     ├─────────────┤
     │ Stage 3     │
     │ 25-hole     │
     ├─────────────┤
     │ Stage 2     │
     │ 25-hole     │
     ├─────────────┤
     │ Stage 1     │  ← Base (MPU6050 placement position 2)
     │ 25-hole     │
     └──────┬──────┘
            │  Mounted on chassis
     ┌──────┴──────┐
     │  DRIVETRAIN │
     └─────────────┘

Motion: Straight up/down (linear), NOT rotational
```

| Property | Value |
|----------|-------|
| Type | 4-stage linear cascade, pulley-driven |
| Motors | 2× 11W V5 Smart Motor (high-torque / red cartridge: 100 RPM) |
| Feedback | Motor encoders (built-in) |
| Inches per encoder tick | **TBD** — will be measured with MPU6050 test runs |
| PID type | Linear position PID |
| Gravity compensation | Constant upward feed-forward (not angle-based — it's linear) |

### 1.3 Claw (Gripper)

| Property | Value |
|----------|-------|
| Motor | 1× 11W V5 Smart Motor |
| Function | Simple open/close — grabs Pins and Cups from center |
| Control | Timed open/close + current-based stall detection |
| Max possession | 1 Pin + 1 Cup (per SG6) |

### 1.4 Toggle Mechanism

| Property | Value |
|----------|-------|
| Motors | 2× 5.5W V5 Smart Motor |
| Design | Vertical beam that spins to flip field Toggles |
| Control | Synced position PID |

### 1.5 Power Budget

| Subsystem | Motors | Power | % of 88W limit |
|-----------|--------|-------|----------------|
| Drive | 4× 11W | 44W | 50.0% |
| Cascade | 2× 11W | 22W | 25.0% |
| Claw | 1× 11W | 11W | 12.5% |
| Toggle | 2× 5.5W | 11W | 12.5% |
| **Total** | **9 motors** | **88W** | **100%** |

All within R10 (88W total) and R11 (Subsystem 2 = drive = 44W < 55W limit).

### 1.6 Sensors

| Sensor | Required? | Port | Reason |
|--------|-----------|------|--------|
| **V5 Inertial Sensor (IMU)** | YES | TBD | Heading for turns, drive-straight correction |
| Vision Sensor | Recommended | TBD | Pin color detection, Goal finding |
| Optical Sensor | Optional | TBD | Proximity + color for close-range detection |
| GPS Sensor | Recommended (Skills) | TBD | Absolute field positioning |

Cascade uses **built-in motor encoders** — no external Rotation Sensor needed.

---

## 2. Game Overview

**Game:** VEX V5RC "Override" (2026-2027)  
**Source:** Official Game Manual v2.0 (September 3, 2026)

### 2.1 Field

| Element | Count | Detail |
|---------|-------|--------|
| Field size | 12' × 12' | Divided into 4 Quadrants + Midfield |
| Autonomous Line | 1 | Divides red/blue sides |
| Alliance-colored Goals | 4 | 2 red, 2 blue |
| Neutral Goals | 5 | 4 short, 1 tall (Midfield) |
| Toggles | 4 | One per Quadrant, on Field Perimeter |
| Loaders | 4 | Two adjacent to each Alliance Station |

### 2.2 Scoring Objects

**Pins (63 total):**

| Type | Pre-Placed | Match Loads | Preloads | Total |
|------|-----------|-------------|----------|-------|
| Red/Blue | 4 | 0 | 0 | 4 |
| Red/Yellow | 8 | 10 | 2 | 20 |
| Blue/Yellow | 8 | 10 | 2 | 20 |
| Yellow/Yellow | 17 | 2 | 0 | 19 |
| **Total** | **37** | **22** | **4** | **63** |

**Cups (56 total):**

| Type | Count |
|------|-------|
| Gray-up pre-placed | 24 |
| Clear-up pre-placed | 12 |
| Match loads (10 red, 10 blue) | 20 |
| **Total** | **56** |

### 2.3 Scoring Table

| Action | Points |
|--------|--------|
| Alliance-colored Pin Scored | **5 pts** |
| Yellow Pin Scored (Owned) | **10 pts** |
| Robot in Midfield at match end | **8 pts** |
| Autonomous Bonus | **12 pts** (6 per alliance) |

### 2.4 Toggle Mechanic (Game-Changer)

- 4 Toggles on the Field Perimeter, one per Quadrant
- Each Toggle can be set to: **Red**, **Blue**, or **Yellow (neutral)**
- Yellow Pins in a Quadrant are **Owned** by the Alliance whose Toggle color is shown
- **Midfield yellow Pins** are Owned by the Alliance with more Robots in Midfield at match end

**Why your Toggle mechanism (2×5.5W beam) is critical:** Flipping a Quadrant's Toggle to your color makes ALL yellow Pins in that quadrant worth 10 points to you instead of 0. A single Toggle flip can swing 20-40 points.

### 2.5 Key Rules (Code Impacts)

| Rule | Summary | Code Impact |
|------|---------|-------------|
| **SG1** | Start 18" cube, one Preload, stationary | Auto starts from legal position |
| **SG2** | Max 24"×24" horizontal expansion | Cascade within bounds |
| **SG3** | Max 50" vertical | Cascade PID max height limit |
| **SG6** | 1 Pin + 1 Cup max possession | Claw can only hold one of each at a time |
| **SG7** | No crossing Auto Line in Autonomous | Drive PID: soft boundary at auto line |
| **SG10** | No removing from Goals | Claw only grabs from floor/Loaders |
| **SG12** | Endgame rules change last 10s | Separate Endgame behavior in driver control |
| **SC8** | AWP: 6+ Pins, 2 Goals with 2+ Pins each | Auto routine must achieve this |

### 2.6 Match Timeline

```
[0:00]  Match Start
[0:00]  AUTONOMOUS PERIOD (15 seconds)
        - No driver input
        - Cannot cross Auto Line
        - Score Preload + field Pins
[0:15]  DRIVER CONTROL PERIOD (105 seconds)
        - Driver operates robot
        - Match Loads available through Loaders
        - Stack Pins/Cups, flip Toggles
[1:50]  ENDGAME (last 10 seconds)
        - "King of the hill" in Midfield
        - Cannot Place objects on Midfield Goal
        - Incidental damage not penalized
[2:00]  Match End
```

---

## 3. Motor Data Reference (PROS API)

### 3.1 Every Data Point Available

V5 Smart Motors report **14 data fields** in real time over the smart port protocol.

| # | Data | PROS Function | Unit | Update Rate | Use for PID? |
|---|------|--------------|------|-------------|-------------|
| 1 | **Position** | `motor.get_position()` | degrees | ~10kHz internally, 10ms batches to brain | ✅ Primary feedback |
| 2 | **Actual Velocity** | `motor.get_actual_velocity()` | RPM | Same as above | ✅ Velocity PID |
| 3 | **Target Velocity** | `motor.get_target_velocity()` | RPM | Same | Reference only |
| 4 | **Current Draw** | `motor.get_current_draw()` | mA | ~50Hz (20ms batches) | ✅ Stall detect, load sensing |
| 5 | **Power** | `motor.get_power()` | W | ~50Hz | Power budget monitoring |
| 6 | **Temperature** | `motor.get_temperature()` | °C | ~10Hz (slow thermal) | Overheat protection |
| 7 | **Torque** | `motor.get_torque()` | N·m | ~50Hz | Load feedback |
| 8 | **Efficiency** | `motor.get_efficiency()` | % (0-100) | ~50Hz | Mechanical health |
| 9 | **Voltage** | `motor.get_voltage()` | mV | ~50Hz | Battery sag detection |
| 10 | **Direction** | `motor.get_direction()` | ±1 | Instant | Sign correction |
| 11 | **Faults/Flags** | `motor.get_flags()` | bitfield | ~50Hz | Error handling |
| 12 | **Raw Encoder** | `motor.get_raw_position()` | raw ticks | 10kHz | For custom gearing math |
| 13 | **Over Temp** | `motor.is_over_temp()` | bool | ~10Hz | Safety cutoff |
| 14 | **Stalled** | Derived from current threshold | bool | Derived | Limit switch alternative |

### 3.2 How the Three Key Data Points Work

#### Position (`get_position()`)

```
PROS returns encoder degrees (not raw ticks).

360° = 1 full motor shaft revolution.

The encoder is ABSOLUTE — it remembers position across power cycles
(within the same match; resets on Brain reboot).

Reset with: motor.tare_position()
Set to value: motor.set_zero_position(0)   [PROS 4 convention]
```

**Important:** The encoder is on the motor shaft, BEFORE the gear cartridge.  
The green cartridge (200 RPM) has an internal 3:1 reduction the encoder doesn't know about.  
But `get_position()` in PROS 4 returns degrees *after* the internal gearset, so 360° = 1 output shaft revolution.

Actually, in PROS 4 this has been normalized. `get_position()` returns degrees at the output shaft of the chosen gearset. So:

```
360° on motor.get_position() = 1 output shaft revolution (after green cartridge 3:1)
```

#### Velocity (`get_actual_velocity()`)

```
Returns RPM at the output shaft.

For 200 RPM (green) cartridge: 0 to ~200 RPM
For 100 RPM (red) cartridge:   0 to ~100 RPM
For 600 RPM (blue) cartridge:  0 to ~600 RPM
```

This is the **actual measured velocity** from encoder deltas, not the target.  
Use this as feedback for velocity PID (flywheels, etc.).

#### Current (`get_current_draw()`)

```
Returns milliamps (mA). This is the SINGLE MOST USEFUL free sensor.

┌──────────────┬─────────────────────────────────────┐
│ Current (mA) │ What it means                        │
├──────────────┼─────────────────────────────────────┤
│ 0–100        │ Free-spinning, zero load            │
│ 100–500      │ Normal driving, light load          │
│ 500–1000     │ Moderate load (accelerating/pushing)│
│ 1000–2000    │ Heavy load (climbing, near-stall)   │
│ 2000–2500    │ Approaching PTC fuse trip           │
│ 2500+        │ PTC fuse tripping (motor off)       │
└──────────────┴─────────────────────────────────────┘

CLaw uses current to detect grip:
  Close claw motor → watch current → when mA > threshold → object is gripped → stop

Toggle uses current to detect flip complete:
  Spin beam → watch current → spike = mechanical stop → flip done

Cascade uses current to detect retraction limit:
  Retract cascade → current spikes at bottom → home position found
```

### 3.3 Timing in PROS (FreeRTOS)

PROS runs on **FreeRTOS**, a real-time OS. Three timing mechanisms:

```cpp
// 1. BLOCKING DELAY — pauses current task
pros::delay(10);  // 10 milliseconds

// 2. BACKGROUND TASK — runs independently
pros::Task logger_task([]{
    while (true) {
        write_to_sd_card();
        pros::delay(20);  // 50 Hz
    }
});

// 3. MILLIS TIMESTAMP — running time since power-on
uint32_t now = pros::millis();  // or pros::c::millis()
```

**The 10ms tick rule:**
The V5 Brain batches motor data every **10 milliseconds**. Running your control loop faster than 10ms reads stale data. Running slower loses control responsiveness. 10ms = 100Hz is the standard.

```
┌────10ms────┬────10ms────┬────10ms────┬────10ms────┐
│  Read all  │  Compute   │  Write all │  pros::    │
│  sensors   │  PID       │  motors    │  delay(10) │
└────────────┴────────────┴────────────┴────────────┘
```

### 3.4 PROS Motor Control Functions

```cpp
pros::Motor motor(port);         // Create motor object on smart port

// BASIC CONTROL
motor.move(127);                 // +127 = full forward, -127 = full reverse, 0 = stop
motor.move_voltage(12000);       // 12000 mV = 12V
motor.move_velocity(100);        // 100 RPM (requires internal velocity PID)
motor.move_absolute(90, 100);    // Move to 90° at 100 RPM
motor.move_relative(360, 100);   // Move 360° forward at 100 RPM

// CONFIGURATION
motor.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);   // Hold position when stopped
motor.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);  // Free spin when stopped
motor.set_brake_mode(pros::E_MOTOR_BRAKE_BRAKE);  // Passive braking
motor.set_gearing(pros::E_MOTOR_GEARSET_36);      // 100 RPM (red) — high torque
motor.set_gearing(pros::E_MOTOR_GEARSET_18);      // 200 RPM (green) — medium
motor.set_gearing(pros::E_MOTOR_GEARSET_06);      // 600 RPM (blue) — high speed

// READING
double pos = motor.get_position();           // Encoder degrees
double vel = motor.get_actual_velocity();    // RPM
double current = motor.get_current_draw();   // mA
double temp = motor.get_temperature();       // °C
double power = motor.get_power();            // W
double torque = motor.get_torque();          // N·m
double efficiency = motor.get_efficiency();  // %
int32_t flags = motor.get_flags();          // Error flags
bool hot = motor.is_over_temp();            // Overheat check

// ENCODER
motor.tare_position();          // Reset encoder to 0
motor.set_zero_position(0);     // Alternative reset
double raw = motor.get_raw_position();  // Raw ticks (not degrees)
```

### 3.5 Motor Group (Multiple Motors on One Axis)

```cpp
// For synced motors (e.g., 2 cascade motors, 2 left drive motors)
pros::Motor_Group left_drive({port1, port2});  // Ports as initializer list

left_drive.move(127);                    // All move together
double avg_pos = left_drive.get_position();   // Average of all motors
double avg_vel = left_drive.get_actual_velocity();
```

### 3.6 V5 Inertial Sensor (IMU)

```cpp
pros::Imu imu(port);                     // 3-wire port

imu.reset();                              // Calibrate (takes ~2 seconds!)
double heading = imu.get_heading();       // 0–360°, clockwise positive
double rotation = imu.get_rotation();     // Continuous (wraps past 360°)
double accel_x = imu.get_accel().x;       // m/s²
double gyro_z = imu.get_gyro_rate().z;    // °/s (roll = x, pitch = y, yaw = z)
pros::c::imu_status_e status = imu.get_status();  // Check if calibrating
bool calibrating = imu.is_calibrating();

// IMPORTANT: Sensor orientation matters.
// The V5 IMU must be mounted flat (logo up, triangular marking forward)
// for get_heading() to return correct yaw.
```

### 3.7 SD Card Logging

```cpp
// PROS supports SD card for data logging
// The SD card slot is on the V5 Brain

// Create/append a file
auto* file = fopen("/usd/log.csv", "a");

// Write CSV with timestamp
fprintf(file, "%lu,%.2f,%.2f,%.2f\n",
    pros::millis(),          // Timestamp
    motor.get_position(),    // Data column 1
    motor.get_current_draw(),// Data column 2
    imu.get_heading()        // Data column 3
);

fclose(file);  // Flush to card (do periodically, not every write)
```

---

## 4. PID Architecture

### 4.1 PID Controller Design

```
                    ┌──────────┐
                    │  kF * target  │  Feed-Forward (open-loop)
                    └─────┬────┘
                          │
                          ▼
target ──▶[+]──▶│  kP * error   │──▶[+]──▶│ output │──▶ motor
            ▲    └──────────────┘    ▲     │ clamp  │
            │                       │     └────────┘
            │    ┌──────────────┐   │
            ├────│  kI * ∫error │───┤
            │    └──────────────┘   │
            │                       │
  -current ─┘    ┌──────────────┐   │
                 │ kD * d(error)│───┘
                 └──────────────┘
```

| Gain | Purpose | When to increase | Symptom of too high |
|------|---------|-----------------|---------------------|
| **kP** | Proportional response | Slow to reach target | Overshoot, oscillation |
| **kI** | Eliminate steady-state error | Robot doesn't quite reach target | Slow oscillation, windup |
| **kD** | Dampen oscillation | Robot overshoots and bounces back | Jerky motion, noise amplification |
| **kF** | Open-loop feed-forward | Known resistance (gravity, friction) | Overshoot (adds directly to output) |

### 4.2 Anti-Windup (Integral Clamp)

When the motor is saturated (output = 127 but still hasn't reached target), the integral term keeps accumulating uselessly. This causes massive overshoot when the target is finally reached.

```
Solution: integral_limit — cap |∫error| at a fixed maximum.
```

### 4.3 Settle Detection

A PID target is "settled" when the error stays below `settle_error` for `settle_time_ms / TICK_MS` consecutive ticks. This prevents false positives from brief error dips.

### 4.4 Subsystem PID Assignments

| Subsystem | PID Type | kP Role | kI Role | kD Role | kF Role |
|-----------|----------|---------|---------|---------|---------|
| **Drive Straight** | Position | Main drive force | Fixes drift | Dampens oscillation | — |
| **Drive Turn** | Angle | Main turn force | Fixes heading offset | Smooths stop | — |
| **Cascade** | Position (linear) | Lifting force | Holds position at height | Dampens bounce | Gravity counter |
| **Toggle** | Position | Spin force | Holds at color | Smooth landing | — |
| **Claw** | None | — | — | — | — |

---

## 5. Encoder Math

### 5.1 Drive Conversion

```
KNOWN:
  Wheel diameter      = 3.25 inches
  Wheel circumference  = 3.25 × π = 10.210 inches
  Motor→Wheel ratio    = 60T / 18T = 3.333 : 1
  Encoder °/motor rev  = 360
  Encoder °/wheel rev  = 360 × 3.333 = 1200.0

CONVERSIONS:
  inches → encoder °:
    inches × (1200.0 / 10.210) = inches × 117.55

  encoder ° → inches:
    encoder_degrees × (10.210 / 1200.0) = encoder_degrees × 0.00851

EXAMPLE:
  Drive 24 inches:
    = 24 × 117.55 = 2821.2 encoder degrees

  Motor reads 1410 encoder degrees:
    = 1410 × 0.00851 = 12.0 inches traveled
```

### 5.2 Cascade Conversion

```
UNKNOWN (will be measured):
  Inches of cascade extension per motor revolution = TBD

Measurement plan:
  1. Place MPU6050 at cascade tip
  2. Run cascade fully up
  3. Record encoder delta and actual inches extended
  4. inches_per_encoder_degree = total_inches / encoder_delta
```

---

## 6. Data Logging Architecture (V5 + ESP32/MPU6050)

### 6.1 What the MPU6050 Gives You That Motors Can't

Motor encoders tell you what the motor is doing. The MPU6050 tells you what the **robot structure** is doing:

| Data | Motor Encoder | V5 IMU | MPU6050 |
|------|:---:|:---:|:---:|
| Motor position | ✅ | ❌ | ❌ |
| Motor velocity | ✅ | ❌ | ❌ |
| Motor current (load) | ✅ | ❌ | ❌ |
| Chassis heading | ❌ | ✅ | ⚠️ filtered |
| Chassis acceleration | ❌ | ✅ (filtered) | ✅ raw 16g |
| Gyro (all 3 axes) | ❌ | ✅ (filtered) | ✅ raw 2000°/s |
| **Cascade tip vibration** | ❌ | ❌ | ✅ |
| **Toggle beam impact** | ❌ | ❌ | ✅ |
| **Claw mount recoil** | ❌ | ❌ | ✅ |
| High-frequency data (100+ Hz) | ❌ | ❌ | ✅ |

### 6.2 Architecture Diagram

```
═══════════════════════════════════════════════════════════════════════════
                          V5 BRAIN (ROBOT SIDE)
═══════════════════════════════════════════════════════════════════════════

  ┌─────────────────────────────────────────────────────────────────────┐
  │  Main Control Loop (100 Hz = every 10ms)                           │
  │                                                                     │
  │  DriveBase::update()   Cascade::update()   Toggle::update()        │
  │  Claw::update()        IMU read                                     │
  │                                                                     │
  │  On tick counter % 5 == 0 (50 Hz): → DataLogger::log_sample()      │
  └─────────────────────────────────────────────────────────────────────┘

  ┌─────────────────────────────────────────────────────────────────────┐
  │  DataLogger (pros::Task, 50Hz)                                      │
  │                                                                     │
  │  Every 20ms, appends one CSV row to /usd/log_XXXX.csv:              │
  │                                                                     │
  │  Column │ Field              │ Source                              │
  │  ───────┼────────────────────┼─────────────────────────────────────│
  │  1      │ timestamp_ms       │ pros::millis()                      │
  │  2      │ drive_pos_avg      │ avg(motor.get_position())           │
  │  3      │ drive_vel_avg      │ avg(motor.get_actual_velocity())    │
  │  4      │ drive_current_max  │ max(all drive motor currents)       │
  │  5      │ cascade_pos_avg    │ avg(cascade motor pos)              │
  │  6      │ cascade_current    │ cascade motor current               │
  │  7      │ claw_current       │ claw motor current                  │
  │  8      │ toggle_pos         │ toggle motor position               │
  │  9      │ imu_heading        │ imu.get_heading()                   │
  │  10     │ imu_accel_x        │ imu.get_accel().x                   │
  │  11     │ imu_accel_y        │ imu.get_accel().y                   │
  │  12     │ imu_gyro_z         │ imu.get_gyro_rate().z               │
  │  13     │ left_motor_temp    │ drive left temp                     │
  │  14     │ battery_voltage    │ motor.get_voltage()                 │
  └─────────────────────────────────────────────────────────────────────┘

═══════════════════════════════════════════════════════════════════════════
                    ESP32 C3 SUPER MINI + MPU6050
═══════════════════════════════════════════════════════════════════════════

  ┌───────────────┐          ┌──────────────────────────────────┐
  │   MPU6050     │  I2C     │   ESP32 C3 Super Mini             │
  │               │─────────▶│                                    │
  │ Accel X,Y,Z   │ SDA/SCL  │   Reads 6-axis data @ 100 Hz      │
  │ Gyro X,Y,Z    │          │   Tags with micros() timestamp    │
  │ Temperature   │          │   Stores CSV to SPIFFS flash      │
  └───────────────┘          │   OR streams to USB Serial        │
                             │                                    │
                             │   CSV format (usb/serial):         │
                             │   ts,ax,ay,az,gx,gy,gz,temp       │
                             └──────────────────────────────────┘

  MPU6050 SPECS:
    Accel: ±2g/±4g/±8g/±16g  (use ±16g for impact detection)
    Gyro:  ±250/500/1000/2000°/s  (use ±2000°/s for vibration)
    Sample rate: up to 1kHz internally, 100Hz over I2C is practical
```

### 6.3 Multi-Position Test Plan

Same autonomous routine, same PID gains, 5 runs:

| Run | MPU6050 Location | What We Learn |
|-----|-----------------|---------------|
| 1 | **Chassis center** | Drive vibration baseline. Is there oscillation at PID frequency? |
| 2 | **Cascade base** (Stage 1 mount) | How much chassis recoils when cascade extends. Does acceleration transfer to drivetrain? |
| 3 | **Cascade tip** (Stage 4 end) | Max extension wobble. What frequency? Does it match drive oscillation? |
| 4 | **Toggle beam** | Impact acceleration when beam hits Toggle mechanical stop. Is there bounce-back? |
| 5 | **Claw mount** | Grip force vibration. Does gripping an object shake the chassis? |

### 6.4 How to Use the Data for PID Tuning

```
AFTER DATA COLLECTION (on laptop):

1. Timestamp-align V5 CSV ↔ MPU6050 CSV

2. Generate overlay plots:
   a) V5 drive encoder position vs time + MPU6050 accel X vs time
      → "Is the position overshoot visible as an acceleration spike?"
      → If yes: REDUCE kP or INCREASE kD

   b) V5 cascade encoder position vs time + MPU6050 tip gyro Z vs time
      → "Does the cascade wobble at a specific frequency?"
      → Find dominant frequency via FFT → set kD to dampen that freq

   c) V5 drive motor current vs time + MPU6050 chassis accel vs time
      → "Does high current correlate with high acceleration?"
      → If current spikes without acceleration: mechanical binding, not PID issue

3. PID tuning rules from data:
   ┌────────────────────┬──────────────────────┬────────────────────┐
   │ DATA SIGNATURE     │ DIAGNOSIS            │ FIX                │
   ├────────────────────┼──────────────────────┼────────────────────┤
   │ Position overshoot │ kP too high or       │ Reduce kP or       │
   │ + accel spike      │ kD too low           │ increase kD        │
   ├────────────────────┼──────────────────────┼────────────────────┤
   │ Position never     │ kP too low or        │ Increase kP or     │
   │ reaches target     │ friction too high    │ increase kI        │
   ├────────────────────┼──────────────────────┼────────────────────┤
   │ Oscillation at     │ kD wrong for         │ Adjust kD to       │
   │ constant frequency │ mechanical resonance │ counter resonance  │
   ├────────────────────┼──────────────────────┼────────────────────┤
   │ Cascade tip        │ Cascade PID           │ Reduce cascade kP  │
   │ wobble transfers   │ too aggressive        │ or increase kD     │
   │ to chassis         │                       │                    │
   ├────────────────────┼──────────────────────┼────────────────────┤
   │ Toggle beam        │ No PID damping or     │ Add kD to toggle   │
   │ bounce after flip  │ hard stop impact      │ PID, or reduce kP  │
   └────────────────────┴──────────────────────┴────────────────────┘
```

### 6.5 ESP32 MPU6050 Wiring

```
ESP32 C3 Super Mini    MPU6050
─────────────────     ───────
       3.3V    ────── VCC
       GND     ────── GND
       GPIO6   ────── SDA (I2C data)
       GPIO7   ────── SCL (I2C clock)
```

---

## 7. Autonomous Planning

### 7.1 Autonomous Win Point Requirements

**Standard events:**
1. 6+ Pins Scored for your Alliance
2. 2+ Goals each with 2+ Pins Scored
3. Neither Robot contacting Field Perimeter

### 7.2 Strategy: Preload + Field Pins

```
Phase 1 (0–3s):   Score Preload on nearest Alliance Goal
Phase 2 (3–8s):   Drive to nearest field Pins, grab, score on second Goal
Phase 3 (8–12s):  Grab third Pin, score on same Goal (= 2 Pins on Goal 2)
Phase 4 (12–15s): Position near Midfield for end-of-auto bonus
```

### 7.3 Key Distances (Need Field Measurement)

| Path | Est. Distance |
|------|--------------|
| Alliance station → nearest Alliance Goal | 24–36" |
| Alliance station → nearest field Pin | 36–48" |
| Alliance station → Midfield | 48–60" |
| Goal → nearby Toggle | 12–24" |

---

## 8. Code Architecture

### 8.1 File Structure

```
vex_pid/
├── project.pros                         # PROS project config
├── Makefile                             # Build configuration
├── VEX_2026_2027_RESEARCH.md            # This document
│
├── include/
│   ├── pid.hpp                          # Core PID controller (header-only)
│   ├── robot.hpp                        # Robot coordinator (all subsystems)
│   ├── data_logger.hpp                  # SD card data logging
│   └── subsystems/
│       ├── drive_base.hpp               # 4-motor drive with straight/turn PID
│       ├── cascade.hpp                  # Linear cascade lift
│       ├── claw.hpp                     # Simple open/close gripper
│       └── toggle.hpp                   # Synced toggle mechanism
│
└── src/
    ├── main.cpp                         # Entry: competition hooks, auto selector
    ├── robot.cpp                        # Orchestration, driver control, auton
    ├── data_logger.cpp                  # Logger implementation
    └── subsystems/
        ├── drive_base.cpp               # Drive PID implementation
        ├── cascade.cpp                  # Cascade PID implementation
        ├── claw.cpp                     # Claw implementation
        └── toggle.cpp                   # Toggle PID implementation
```

### 8.2 Class Hierarchy

```
PIDController (pid.hpp)
  │
  ├──→ DriveBase  (1 PID for straight, 1 PID for turn)
  │      Motors: left_group (2 motors), right_group (2 motors)
  │      Sensor: V5 IMU (heading)
  │
  ├──→ Cascade    (1 PID for linear position)
  │      Motors: motor_group (2 motors)
  │      Sensor: motor encoders (built-in)
  │
  ├──→ Toggle     (1 PID for position)
  │      Motors: 2 motors synced
  │      Sensor: motor encoders
  │
  └──→ Claw       (No PID — simple timed control)
         Motor: 1 motor
         Sensor: motor current (stall detect)
```

### 8.3 Robot Orchestration

```
Robot
  ├── owns: DriveBase, Cascade, Claw, Toggle, DataLogger
  │
  ├── subsystems_tick():  calls each subsystem.update() every 10ms
  │
  ├── driver_tick():      maps controller inputs to subsystem commands
  │   ├── L-stick Y → drive arcade (forward)
  │   ├── R-stick X → drive arcade (turn)
  │   ├── R1/R2 → cascade up/down
  │   ├── L1 → claw toggle (alternating open/close)
  │   └── A/B/X/Y → toggle mechanism / cascade presets
  │
  └── auton_tick():       autonomous state machine
      ├── States: IDLE, DRIVE_TO_GOAL, SCORE_PIN, GRAB_PIN, etc.
      └── Transitions based on subsystem is_at_target() checks
```

### 8.4 Data Flow Per Tick (10ms)

```
1. Read all sensors
     ├── IMU heading
     ├── Drive motor positions × 4
     ├── Cascade motor positions × 2
     ├── Toggle motor positions × 2
     └── Claw motor current × 1

2. Compute PIDs
     ├── DriveBase::update()
     │     ├── Straight PID: error = target_distance - current_distance
     │     ├── Turn PID:    error = wrapped(target_heading - current_heading)
     │     └── Output combined, sent to motor groups
     ├── Cascade::update()
     │     ├── Position PID: error = target_height - current_height
     │     └── Output → cascade motor group
     └── Toggle::update()
           ├── Position PID: error = target_angle - current_angle
           └── Output → toggle motors

3. Every 5th tick (50Hz): DataLogger::log_sample()
     └── Write CSV row to SD card

4. pros::delay(10)
```

---

## 9. Port Assignments (TBD)

> **Fill in when robot is wired.** These are placeholder port numbers.

| Motor/Sensor | Smart Port | Wire Port | Notes |
|-------------|-----------|-----------|-------|
| Drive Left Front | TBD | — | 11W, green cartridge, reversed |
| Drive Left Back | TBD | — | 11W, green cartridge, reversed |
| Drive Right Front | TBD | — | 11W, green cartridge |
| Drive Right Back | TBD | — | 11W, green cartridge |
| Cascade Motor A | TBD | — | 11W, red cartridge |
| Cascade Motor B | TBD | — | 11W, red cartridge |
| Claw Motor | TBD | — | 11W, green cartridge |
| Toggle Motor A | TBD | — | 5.5W |
| Toggle Motor B | TBD | — | 5.5W |
| V5 IMU | — | TBD | 3-wire port |

---

## 10. Next Steps

| # | Task | Status |
|---|------|--------|
| 1 | ✅ Research game rules & document | Done |
| 2 | ✅ Document motor API & data pipeline | Done |
| 3 | ✅ Architecture & class design | Done |
| 4 | ⬜ Fill in port assignments | Waiting on wiring |
| 5 | ⬜ Measure cascade inches/encoder-tick | Waiting on MPU6050 test runs |
| 6 | ⬜ Rewrite code (fix bugs + new subsystems) | Next |
| 7 | ⬜ Write ESP32 + MPU6050 firmware | After V5 code |
| 8 | ⬜ Tune PID gains with MPU6050 data | After hardware tests |