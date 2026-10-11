# PID test plan (staged, safety-first)

**Status: PLANNING ONLY.** No motor may be commanded until the robot is on the bench and the
Stage 0 gate passes. Rules context: `RULES_CONFIGURATION.md`. Safety doctrine: `TESTING_SAFETY.md`.

Baseline program: **drive v1, removed from the working tree upstream on 2026-10-10** — recover
it any time with `git show v1-drive-working-40:vexcode-python/cascade_robot_drive.py`. The
current on-robot driver is `vexcode-python/cascade_robot_drive_limits.py` ("drive live angles"),
which now also carries the measured travel limits.

Hard limits in force until verification says otherwise, applying to **every** motor:

- **Start at 10% speed.** No mechanism is first exercised above 10%.
- **Absolute ceiling 50%.** No code change may set any `*_SPEED` above 50 for now.
  (Conflict to resolve: `cascade_robot_drive_limits.py` has CASCADE/CLAW/TOGGLE at **55** —
  deliberately raised from 40 by the team after robot testing on 2026-10-10, which exceeds this
  plan's 50% ceiling. **Left as the team set it; the operator must consciously pick 50 or 55
  before the next run.** The auton file that had 100/60/100 was deleted upstream and is moot.)

Every threshold below marked *(chosen)* is a number picked at a desk for the first run — the
agent or the operator may adjust it on the bench, but must **report the change and the reason**.
All currents marked *(measured)* must come from the amps tool before they are trusted.

## 0. PID type decision

**Use the V5's internal motor PID (VEXcode `Motor.stop(HOLD)` / `set_stopping(HOLD)`) for
holding, and a software position PID only where holding is not enough.** Reasoning from the repo:

- Every mechanism already relies on HOLD for the "let go = stay put" behavior, and the C++
  `PIDController` (P/I/D/F, clamped integral, settle detection) exists in `include/pid.hpp` with
  a tuned-on-paper gain set. Its Python port *used to* live in `cascade_robot_auton.py` (line 232)
  — that file was deleted upstream 2026-10-10; recover it with
  `git show 723ddf1:vexcode-python/cascade_robot_auton.py` when the reference is needed.
- The internal PID cannot express the two things this robot actually needs: **gravity
  feedforward on the cascade** (`gravity_feedforward = 8.0` in `src/main.cpp`; the deleted auton
  file had `CASCADE_FEEDFORWARD_VOLTS = 0.75`) and **strain-guard easing**. So the plan
  is: internal PID for hold/position-at-rest, software PID for cascade extension moves and
  toggle flips, and the manual ramp path stays untouched as the known-good fallback.
- We do **not** tune on the PROS C++ project first: it has never even been linked. The Python
  programs are the ones with bench-harness evidence, and drive v1 (git tag `v1-drive-working-40`)
  is the version the team has actually driven. Python is the tuning vehicle; PROS is refreshed
  afterwards from the measured gains.

## 1. Data collection (all stages)

From the V5 (Python, `brain` API — sample every loop, ~10-20 ms):

| Channel | Source | Why |
|---|---|---|
| Motor position, deg | `Motor.position(DEGREES)` per motor | rotation angle; the PID error term itself |
| Motor velocity | `Motor.velocity(RPM)` or `PERCENT` | "told to move and not moving" detection |
| Motor current, A | `Motor.current(CurrentUnits.AMP)` per motor + `MotorGroup.current()` (sums) | saturation, strain, stall |
| Motor temperature, C | `Motor.temperature(TemperatureUnits.CELSIUS)` | overheating — the amps tool already prints this |
| Battery | `brain.battery.voltage(MV)`, `brain.battery.current()` | a sagging battery invalidates every other number |
| Loop timing | `brain.timer.system_high_res()` deltas | drive v1/v2 already learned the loop is not a clean 20 ms |
| Button state | controller buttons | to align events with logs |

From the MPU6050 rig (ESP32-C3, 200 Hz on-board, 33 Hz CSV over BLE or USB serial):

`roll, pitch, yaw, ax, ay, az, gx, gy, gz, aMag, jerk, evFlags, impactG, swayHz, swayRmsG,
cntImpact, cntDrop, cntSway, cntShake, cntTap` (full field list in `mpu6050_test/README.md`).

**How to record:** two streams, timestamped independently, aligned afterwards by a physical
synchronization event (a deliberate hand tap on the chassis at the start of every trial shows up
as `cntTap`/`impactG` on the MPU and as a current spike in the V5 log). V5 logging: extend the
display loop to also write a CSV line to the SD card (`brain.sdcard`, legal under <R19i>) — the
C++ `DataLogger` (50 Hz, `/usd/vex_log.csv`) shows the column layout to copy. BLE bandwidth is
fine for diagnostics but drop-prone under motor noise: prefer **USB serial** for any trial where
losing data would waste a risky maneuver, and use BLE only for untethered driving trials.

**What NOT to expect from the MPU:** no position/odometry. Double-integrating a consumer
accelerometer gives ~1.8 m of phantom drift in 6 s (documented in the repo README) — orientation,
tilt, rate, jerk, and vibration only.

## 2. Stage-by-stage plan

### Stage 0 — Bench gate (no PID at all)

1. Wheels off the floor, arm clear. Charged battery (≥ 12.4 V *(chosen)* to start).
2. Confirm direction, one motor at a time, at 10%. The old `cascade_robot_test.py` was deleted
   upstream (recover: `git show 723ddf1:vexcode-python/cascade_robot_test.py`); the replacement
   is **drive live angles**, which shows every motor's live angle sign — or the amps tool (step 3)
   spins one mechanism at a time. Fix any reversed motor before anything else runs.
3. Run the amps tool (`cascade_robot_amps.py`, already at 40 → temporarily set to 10 for the
   first pull, then 25) to record **peak / steady / temp** per mechanism. Write the numbers into
   this file's table below. This replaces the guessed thresholds in `src/main.cpp`.
4. Verify the stop: in every program, the stop control (A latch in PROS, B in drive v2, any
   d-pad in amps tool) actually silences everything, and stays stopped when released.
5. MPU rig smoke test: dashboard connects, calibration completes flat and still, a hand tap
   registers. Port 9 is the radio (confirmed 2026-10-11) — **there is no IMU on the robot**, so
   every heading/tilt number in this plan comes from the MPU rig (practice only) or from
   drive-encoder differences. Before the PROS project is ever downloaded, `imu_port = 9` in
   `src/main.cpp:134` must be fixed: `src/robot.cpp:30` builds a `pros::Imu(9)` on the radio's
   port and would apply heading correction from garbage after a 2.5 s timer.
6. **Probe the v2 API suspect:** `cascade_robot_drive_v2.py` calls
   `set_max_torque(…, CurrentUnits.AMP)` — at startup for the claw, and every loop the strain
   guard changes a cap. If this VEXcode build does not accept current units on that call, the
   program dies before driving. Test it in a 10-second bench run before trusting v2 at all
   (this is the most likely concrete reason v2 felt "broken").

**Stage 0 success criteria:** every motor spins the documented direction at 10%; all four
mechanisms report peak current and temperature; stop verified on all programs; MPU CSV flowing.
**Abort if:** any motor is hot to touch (> 50 °C *(chosen)*), any direction is wrong and cannot
be fixed in code, or the battery sags below 11.5 V *(chosen)* under no load.

**Amps table to fill in during Stage 0:**

| Mechanism | peak A (per motor) | steady A | temp after 10 s |
|---|---|---|---|
| Drive (one side) | _tbd_ | _tbd_ | _tbd_ |
| Cascade up (pair total) | _tbd_ | _tbd_ | _tbd_ |
| Cascade down | _tbd_ | _tbd_ | _tbd_ |
| Claw close on object | _tbd_ | _tbd_ | _tbd_ |
| Toggle | _tbd_ | _tbd_ | _tbd_ |

### Stage 1 — Basic movement, no cascade PID

Drive only, with the cascade **fully down** and the claw open (per `TESTING_SAFETY.md`, the arm
must start at its natural floor or the software floor is in the wrong place). Mechanisms held
manually at 10%, later 25%, via the v1 drive program; PID only on drive-straight/turn if we test
that (PROS `drive_straight` or the auton file's `drive_inches`, both never run on hardware).

- 1a. Wheels off floor: small stick inputs, confirm ramp feel and that letting go brakes.
- 1b. Floor, open space: 3 straight runs of ~24" at 25% then 40%; 90° and 180° turns.
- 1c. Only after 1a/1b are boring: try a drive-straight PID move at **output capped at 25%**
  *(chosen)*.

**Success criteria (chosen):** straight run ends within ±3" of the target and ±5° of heading;
turn overshoot ≤ 10° and settles within 1.5 s; no sustained oscillation (≤ 2 visible
back-and-forth corrections); peak drive current stays under 2× the Stage 0 steady value.
**Watch for:** oscillation (kP too high), drift to one side (mechanical or heading bias, not a
gain problem), current spikes without motion (binding), any drive motor above 60 °C *(chosen)*.

### Stage 2 — Cascade up high (the fall-risk stage)

The cascade is the known-broken subsystem: it was previously destroyed by running the motor too
fast. It has **no physical bottom stop** (chain can jump the sprocket) and, extended, the robot
is a tall top-heavy pendulum on a 3.25" wheel base.

**Tilt prevention, in order:**
1. **Test on the floor, never on a table edge**, with the drive locked (BRAKE, sticks neutral)
   during every lift — a drive input while extended is the classic tip-over.
2. **One axis at a time.** First lifts happen with the robot against a wall/field perimeter so a
   tip falls inward only a few degrees, and with a spotter's hand hovering (not touching) the
   frame.
3. **Height ladder, not full height.** 25% → 50% → 75% → 100% of the measured 755° travel
   (from `cascade_robot_drive_limits.py`), each held 5 s, before the next rung. The software
   limit `CASCADE_MARGIN = 25°` stays.
4. **Speed ladder inside each rung.** 10% → 25% → 40% → 50% ceiling. The previous failure was
   speed, so speed climbs last, and only after the hold is proven.
5. **Gravity feedforward + hold at every rung.** The arm never coasts; when the button is
   released the PID/HOLD must hold height indefinitely — if it sags more than 1" in 5 s
   *(chosen)*, do not go higher until the feedforward is raised.
6. **MPU pitch watch.** During every lift, an operator watches live pitch. **Abort the lift
   above 8° chassis pitch or 10° roll** *(chosen)*; a slow return below 3° is required before
   the next rung.

**Success criteria (chosen):** reaches full measured travel; overshoot ≤ 3° of motor angle at
the top; holds height 30 s with ≤ 1° drift and no more than 3 A pair current *(to be replaced
by Stage 0 measurements)*; tip sway (MPU on cascade top) settles within 2 s and peak sway ≤ 10°
*(chosen — revisit after first measurement)*; chain stays on; no motor above 60 °C.
**Watch for:** sway frequency rising with height (resonance → reduce kP/increase kD), sag under
gravity (raise feedforward, not kP — kP fights the whole travel), chain skip sounds, current
climbing while holding (binding), the robot "walking" its wheels while extended.

### Stage 3 — Claw and toggle PID

Claw: hold a real Pin. Toggle: flip to a color consistently.

- 3a. Claw at 10%, close on a Pin, hold 10 s. Measure grip current (the strain guard's
  `CLAW_STRAIN_AMPS` replacement). Then 25%, then 40%. Test on Pin, Cup, and empty (stall on
  hard stops is the worst case — confirm the guard eases instead of grinding).
- 3b. Toggle: flip Yellow↔Red↔Blue at 10/25/40%, 10 cycles each, on the field perimeter toggle
  if available, else against a soft stop. It has hard mechanical stops — the PID must land on
  them, not slam: add kD / settle band so the last 20° (the `TOGGLE_SLOW_BAND`) is crawled.

**Success criteria (chosen):** claw holds a Pin through a 30 s gentle drive without slipping
(≤ 5 mm of creep *(chosen)*), grip current ≤ measured normal-grip peak + 0.5 A, and the guard
eases (visible on screen) rather than stalling; claw temperature rise ≤ 10 °C over 2 min of
intermittent gripping. Toggle: 10/10 flips reach the commanded color within ±5° *(chosen)*,
settle ≤ 1 s, no bounce off the hard stop (bounce → add damping, not less speed).
**Watch for:** claw motor hot while "holding" (that is a stalled motor cooking — the guard must
catch it), toggle oscillating between colors (kP too high), the toggle stalling into its stop at
full power (current spike at each flip's end).

### Stage 4 — Cascade + claw together (consistent stacking)

The full task: lift a Pin, extend, place/stack, retract. This is where coupling shows up — the
claw's grip shifts the cascade's load, and the release moment unloads the arm and makes it
jump.

- 4a. Lift-and-lower cycles with a Pin gripped, 25% ceiling, 10 cycles. Confirm the cascade PID
  tuning from Stage 2 still holds **with the extra mass** (it will need slightly more
  feedforward; measure, don't guess).
- 4b. Place the Pin on a target (a stack height marked on the wall first, then a real Goal).
  Success is judged by placement, not by encoder numbers: the stack must stand.
- 4c. Only when 4a/4b are consistent: raise speeds toward the 50% ceiling, one mechanism at a
  time.

**Success criteria (chosen):** 10 consecutive lift-place-retract cycles with the stack standing
≥ 8/10; placement position repeatable within ±0.5" *(chosen)*; no tip event, no drop, no chain
skip; every motor under the Stage 0 measured safe current at all times.
**Watch for:** the arm jumping at release (unloading shock → ease the descent/hold transition),
sway at height with the load (re-tune kD from Stage 2 data), grip slip during lift (claw kP /
grip current), driver-visible oscillation anywhere — any oscillation at all means stop and fix,
because oscillation at height is how the cascade broke last time.

## 3. Standing safety procedure (every session)

1. Charged battery; wheels off floor for the first command of the session.
2. Start every mechanism at 10%. Climb: 10 → 25 → 40 → 50. Never jump rungs; one change at a
   time.
3. Hand on the Brain's power button whenever anything is extended or gripping.
4. **Immediate stop, power off if needed, if:** any motor exceeds 60 °C or smells hot; any
   mechanism moves opposite to the command; chassis pitch > 8° or roll > 10° with the cascade
   extended; sustained current above 2× the Stage 0 steady value; any oscillation that does not
   decay within 2 s; any sound of chain skip or hard grinding.
5. After any incident: write down mechanism, command, observed behavior, and screen readout
   before changing anything (`TESTING_SAFETY.md` "If something goes wrong").
6. The ESP32/MPU rig comes **off** the robot before any match, inspection, or field time
   (<R6> — no microcontrollers other than the Brain, even decorative).

## 4. Threshold register

All chosen numbers from this plan, in one place (adjustable on the bench, changes must be
reported): battery start ≥ 12.4 V, abort < 11.5 V; motor touch-abort 50 °C, run abort 60 °C;
drive straight ±3" / ±5°; turn overshoot ≤ 10°, settle ≤ 1.5 s; PID first-move output cap 25%;
cascade height ladder 25/50/75/100%; sag under hold ≤ 1" per 5 s; tilt abort pitch 8° / roll
10°; tip sway peak ≤ 10°, settle ≤ 2 s; claw creep ≤ 5 mm; toggle accuracy ±5°, 10/10 flips;
stacking ≥ 8/10, placement ±0.5".

## 5. File audit — unused / unnecessary files

**Audit result (2026-10-11, after upstream cleanup):** the three programs the audit originally
flagged as most dangerous/unnecessary — `cascade_robot_auton.py`, `cascade_robot_test.py` and
drive v1 `cascade_robot_drive.py` — **were deleted from the tree by the team's own upstream
commits on 2026-10-10** (the cleanup commit, pushed as part of this task, only fixes what that
left behind). They remain recoverable:

```bash
git show 723ddf1:vexcode-python/cascade_robot_auton.py
git show 723ddf1:vexcode-python/cascade_robot_test.py
git show v1-drive-working-40:vexcode-python/cascade_robot_drive.py
```

What the cleanup commit in this task actually did: pruned `sync_files.py` from 6 program pairs
to 3 (it crashed on the deleted files), removed the deleted-file rows from both READMEs, re-synced
the drive-limits pair (the `.v5python` was stale), and fixed the stale sections of the research
doc. Remaining audit items, in priority order:

1. **`vexcode-python/cascade_robot_drive_v2.py` / `Cascade Robot Drive V2.v5python`** — the
   team reports v2 as broken on the robot. By inspection its logic is internally consistent and
   it passed the fake-VEX bench harness, but it has never run on hardware, and it has two
   concrete suspect points that match "broken" symptoms: (a) the permanent torque ceilings —
   `CLAW_HOLD_AMPS = 1.2 A` and `TORQUE_FULL_AMPS = 2.5 A` per motor — if those are below what
   the real mechanisms need, the claw will not grip and the arm will not lift no matter how long
   the button is held; (b) `set_max_torque(…, CurrentUnits.AMP)` may be rejected by this
   VEXcode build (Stage 0 probe #6). **Plan: keep as reference, diagnose with the Stage 0 amps
   table before any further use** — do not tune around a wrong torque cap. (Note: the team's
   own fix on the live-angles side — resetting every motor to `set_max_torque(100, PERCENT)` at
   startup "in case drive v2 left a limit on it" — confirms suspect (a) was real on the robot.)
2. **`VEX_2026_2027_RESEARCH.md` — factually stale in three places:** port table still says
   "TBD" (ports are known), §6.5 lists ESP32 wiring as GPIO6/7 (the rig actually uses SDA=GPIO5,
   SCL=GPIO4), and there is no <R6> warning about the ESP32 rig's illegality on the robot.
   **Fixed in the cleanup commit.** It stays in the repo — it still holds the encoder math and
   PID architecture notes.
3. **`.DS_Store` (repo root, untracked+ignored)** — cosmetic; git already ignores it.
4. **`tools/.pros_headers/` (gitignored)** — regenerated by `tools/get_pros_headers.sh`; nothing
   to do, just noting it is not part of the repo's tracked content.
5. **`TESTING_SAFETY.md` (root) vs `vexcode-python/TESTING_SAFETY.md`** — two different safety
   docs (root one newer, Oct 10; vexcode-python one "Written 2026-10-08"). Both are referenced
   from their own READMEs; the older one's stop-button/speed facts were synced in the cleanup
   commit. Keep both.

Explicitly checked and **should be kept**: all of `include/` and `src/` (each header has a
referencing module; `data_logger` is wired into `robot.cpp`; `amps_tool` is the Stage 0
instrument), `tools/feel_test.cpp` + stubs (the only offline verification), all three remaining
`.py`/`.v5python` pairs (VEXcode opens the `.v5python`, `sync_files.py` keeps them equal — all
three report "Same"), `mpu6050_test/` in full (the diagnostic rig the plan depends on),
`Makefile`, `project.pros`, `sync_files.py`, and both READMEs.

## 6. Git push plan

State: branch `main`, `origin` = `https://github.com/anonamerjaxon-dev/vex-pid` (already
authenticated on this Mac as `anonamerjaxon-dev` via `gh`, token in the macOS keychain — never
printed or copied). Upstream moved during planning (4 new commits, all "Drive live angles" work,
one of which deleted the three programs above) and local has been fast-forwarded onto it.

Steps, in order, all from the repo root:

1. Commit the two new planning documents (`RULES_CONFIGURATION.md`, `PID_TEST_PLAN.md`).
2. Commit the cleanup: `sync_files.py` pruned to 3 pairs, README dead-file rows removed,
   drive-limits `.v5python` re-synced, research doc's three stale sections fixed, older
   TESTING_SAFETY facts synced.
3. `git push origin main`.
4. `override-2.0.pdf` (10.4 MB) and `manual_full.txt` stay **untracked**: `.gitignore` already
   excludes `override-*.pdf` and `manual_full.txt`, and the README documents the policy — VEX's
   manual is copyrighted and "no portion may be copied or redistributed", so the committed
   artifact is the paraphrased `RULES_CONFIGURATION.md`, not the PDF. The manual is "added to
   the repository" in the sense the task means (present in the working tree, versioned source
   for the rules doc, re-downloadable from the recorded URL), without redistributing VEX's
   document on GitHub.
