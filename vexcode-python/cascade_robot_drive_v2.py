#region VEXcode Generated Robot Configuration
from vex import *
import urandom
import math

# Brain should be defined by default
brain=Brain()

# Robot configuration code


# wait for rotation sensor to fully initialize
wait(30, MSEC)


# Make random actually random
def initializeRandomSeed():
    wait(100, MSEC)
    random = brain.battery.voltage(MV) + brain.battery.current(CurrentUnits.AMP) * 100 + brain.timer.system_high_res()
    urandom.seed(int(random))

# Set random seed
initializeRandomSeed()


def play_vexcode_sound(sound_name):
    # Helper to make playing sounds from the V5 in VEXcode easier and
    # keeps the code cleaner by making it clear what is happening.
    print("VEXPlaySound:" + sound_name)
    wait(5, MSEC)

# add a small delay to make sure we don't print in the middle of the REPL header
wait(200, MSEC)
# clear the console to make sure we don't have the REPL in the console
print("\033[2J")

#endregion VEXcode Generated Robot Configuration

# ------------------------------------------
#
# 	Project:      Cascade Robot Drive v2
#	Description:  The driver-control program, with a proper feel to it.
#
#	              Same controls as v1, but:
#	                * the sticks are non-linear - fine near the middle and
#	                  full power only when pushed all the way over
#	                * every command is ramped, so nothing ever slams on
#	                * the cascade watches its own current and eases off
#	                  when it starts to strain
#	                * the cascade cannot be driven below the place it was
#	                  hanging when the program started
#	                * the claw grips with a current limit, so it holds on
#	                  without cooking the motor
#	                * a full stop button: hold B and every motor on the
#	                  robot stops at once
#
#	Left stick up/down ..... drive forward / backward
#	Right stick left/right . turn left / right
#	  (that pair is called "split arcade drive")
#
#	L1 ......... cascade up          L2 ......... cascade down
#	R1 ......... claw close          R2 ......... claw open
#	Up ......... toggle one way      Down ....... toggle the other way
#	B .......... FULL STOP - every motor stops while this is held
#
#	v1 is the plain version, kept untouched:
#	  cascade_robot_drive.py
#
# ------------------------------------------

# Library imports
from vex import *

# ==========================================================================
#  SETTINGS - everything you might want to change lives in this block
# ==========================================================================

# Gear cartridge inside each 11W motor:
#   GREEN = GearSetting.RATIO_18_1 (200 RPM)
#   BLUE  = GearSetting.RATIO_6_1  (600 RPM)
#   RED   = GearSetting.RATIO_36_1 (100 RPM)
DRIVE_GEARS = GearSetting.RATIO_18_1
CASCADE_GEARS = GearSetting.RATIO_36_1
CLAW_GEARS = GearSetting.RATIO_18_1

# ---- How fast each part may ever go, in percent ---------------------------
# These are CEILINGS, not the normal speed. The stick curve below decides how
# much of the ceiling you get, and pushing the stick all the way over is the
# only way to reach the whole number. Tune these first.
DRIVE_SPEED = 40
TURN_SPEED = 40
CASCADE_SPEED = 40
CLAW_SPEED = 40
TOGGLE_SPEED = 40

# ---- How the sticks feel -------------------------------------------------
# Everything here is about the SHAPE of the stick, not the power.
DEADBAND = 5              # stick movement smaller than this does nothing
FINE_END = 0.85           # the first 85% of the travel is the fine zone
FINE_TOP = 0.40           # ...and it only reaches 40% of the ceiling there
STICK_EXPO = 2.0          # 1.0 = straight line, 2.0 = much finer in the middle
MIN_MOVE_FRACTION = 0.15  # the gentlest touch starts at 15% of the ceiling

# ---- The ramp ------------------------------------------------------------
# A motor may only change its power by this much per second. Without it, a
# button press asks for 0 to 40 in a single instant, and the robot snaps.
# This is the single biggest reason a machine feels smooth instead of jerky.
RAMP_PER_SECOND = 250
LOOP_MS = 20

# ---- The full stop -------------------------------------------------------
# Hold this button and every motor on the robot stops, whatever else is being
# asked for. It is the "something has gone wrong, be still" button, so it is
# the FIRST thing the loop looks at - nothing below it can command a motor
# once this is held.
#
# Let go and the robot is live again. That is safe because the stop pins every
# ramp to zero: the ramp is what would otherwise carry a number through the
# stop and set off at speed the instant the button came back up.
#
# B is free in this program. A, X, Y, Left and Right are free as well, if you
# would rather reach for one of those.
STOP_BUTTON = "B"

# ---- The cascade's soft lower limit --------------------------------------
# The arm has no physical stop and the chain can come off if it is driven
# down far enough. The program zeroes the encoder where the arm hangs at
# startup, so this is how far BELOW that it may ever be driven.
CASCADE_LOWER_LIMIT = -2.0    # degrees
CASCADE_DOWN_SIGN = -1        # L2 (down) makes the number go down. If you
                              # hold L2 and the number on the screen goes UP,
                              # change this to +1.
CASCADE_DEG_PER_SECOND = 600.0  # a red (36:1) motor turns at 100 RPM, which is
                              # 600 degrees of its output every second at its
                              # own top speed. Used to work out how far the arm
                              # will travel before the program next looks at
                              # it, so it can be stopped at the limit instead
                              # of coasting past. Change this if you change
                              # CASCADE_GEARS: green (18:1) is 1200 and blue
                              # (6:1) is 3600.
CASCADE_SAFETY_FACTOR = 1.5   # Assume the arm might travel this much further
                              # than the sum says. The program reads the
                              # position, then sends the command, then waits -
                              # and the arm keeps moving through all of that,
                              # so a straight calculation would always be a
                              # little optimistic. Bigger means the arm stops
                              # further from the limit.
CASCADE_CREEP_BAND = 25.0     # degrees above the limit where the arm stops
                              # coming down fast and starts creeping instead,
                              # so it can rest right on the limit rather than a
                              # full-speed step above it.
CASCADE_CREEP_SPEED = 10.0    # the slow speed it creeps down at over that last
                              # stretch. Slower means it gets closer to the
                              # limit; too slow and the motor will not turn at
                              # all, so raise this if the arm refuses to creep.

# ---- Strain protection ---------------------------------------------------
# Amps are the honest way to tell "working hard" from "stuck". The numbers
# below are a starting point: watch the brain screen while you test them.
USE_TORQUE_LIMITS = True      # set to False if set_max_torque misbehaves
TORQUE_FULL_AMPS = 2.5        # the ceiling for EACH motor when nothing is wrong.
                              # High enough that it is effectively "no limit".

# Careful: MotorGroup.current() adds the whole group up, but
# MotorGroup.set_max_torque() sets the ceiling on EACH motor. So the two
# numbers below are counted differently, and the comments say which is which.
CASCADE_STRAIN_AMPS = 2.0     # the two cascade motors ADDED TOGETHER
CASCADE_EASE_AMPS = 1.0       # ceiling for EACH motor while straining, so
                              # 2.0 A for the pair
CASCADE_EASE_SPEED = 0.35     # ...and ease the speed down to 35%
CASCADE_STRAIN_LOOPS = 3      # it must strain this many loops in a row
CASCADE_EASE_STEP = 0.05      # how quickly the easing comes on
CASCADE_RECOVER_STEP = 0.06   # how quickly it lets go again

CLAW_STRAIN_AMPS = 0.8        # one motor, so no adding up: above this it is
                              # gripping something
CLAW_HOLD_AMPS = 1.2          # its current ceiling, on for the whole run
CLAW_HOLD_SPEED = 0.35        # ease the closing speed to 35% while gripping
CLAW_STRAIN_LOOPS = 2
CLAW_EASE_STEP = 0.08
CLAW_RECOVER_STEP = 0.10

# ---- Strain protection: the other half -----------------------------------
# Amps catch a hard hit the instant it happens, because the current jumps the
# moment a motor is loaded. They are not the whole story: a chain starting to
# drag, or a game element wedged somewhere soft, can hold a motor back while
# it draws a perfectly ordinary current. So the encoder gets a say as well.
#
# "Told to spin and not spinning" needs no figure that you have to guess at in
# amps. It compares the motor's own speed reading against what we just asked
# it for, so it scales itself to whatever this mechanism normally does.
USE_VELOCITY_CHECK = True     # False = watch the current only
BLOCKED_FRACTION = 0.25       # moving slower than this share of what was
                              # asked for counts as "not moving"
BLOCKED_LOOPS = 6             # ...for this many loops in a row before we
                              # believe it. A speed reading jitters more than
                              # a current reading, and a motor takes a moment
                              # to spin up to what it was asked for, so this
                              # debounce has to be the longer of the two.
BLOCKED_MIN_ASK = 5.0         # below this the speed reading is mostly noise,
                              # so small deliberate crawls are left alone

# How much a command may change in one loop of the driver program.
RAMP_PER_LOOP = RAMP_PER_SECOND * LOOP_MS / 1000.0

# --- Devices ---
# Front/back/left/right are as the robot drives forward.
# This block is the same in every version of the program (match, test and
# drive). If the test program shows a motor turning the wrong way, flip its
# True/False here AND in the other versions.
#
# Every pair on the same shaft has opposite True/False on the two sides,
# because the two motors face each other across that shaft.

controller_1 = Controller(PRIMARY)

# Drivetrain: 4 x 11W
drive_left_front_11 = Motor(Ports.PORT11, DRIVE_GEARS, True)
drive_left_back_17 = Motor(Ports.PORT17, DRIVE_GEARS, True)
drive_right_front_1 = Motor(Ports.PORT1, DRIVE_GEARS, False)
drive_right_back_10 = Motor(Ports.PORT10, DRIVE_GEARS, False)
left_drive = MotorGroup(drive_left_front_11, drive_left_back_17)
right_drive = MotorGroup(drive_right_front_1, drive_right_back_10)

# Cascade: 2 x 11W on one shaft
cascade_left_13 = Motor(Ports.PORT13, CASCADE_GEARS, False)
cascade_right_2 = Motor(Ports.PORT2, CASCADE_GEARS, True)
cascade = MotorGroup(cascade_left_13, cascade_right_2)

# Toggle: 2 x 5.5W on one shaft (5.5W motors are always 200 RPM)
toggle_18 = Motor(Ports.PORT18, GearSetting.RATIO_18_1, False)
toggle_8 = Motor(Ports.PORT8, GearSetting.RATIO_18_1, True)
toggle = MotorGroup(toggle_18, toggle_8)

# Claw: on the long 1500mm cable
claw_16 = Motor(Ports.PORT16, CLAW_GEARS, False)
claw = MotorGroup(claw_16)     # one motor, but the driver code talks to it as "claw"

# Port 6 is a communication device, not a motor. Ignore it.
# Port 9: a single device nobody has identified yet. Not used.

# ==========================================================================
#  HELPERS
# ==========================================================================

def clamp(value, low, high):
    if value < low:
        return low
    if value > high:
        return high
    return value

def apply_deadband(value):
    # Nothing inside the deadband, and past it the rest of the travel is
    # spread over the full range again. Cutting the middle out (instead of
    # rescaling like this) would throw away stick travel and make fine
    # control worse, not better.
    if abs(value) <= DEADBAND:
        return 0.0
    sign = 1.0 if value > 0 else -1.0
    return sign * (abs(value) - DEADBAND) * 100.0 / (100.0 - DEADBAND)

def stick_shape(value, ceiling):
    # Turn one raw stick reading (-100..100) into a power (-ceiling..ceiling).
    #
    # There are two zones:
    #   * the FINE zone is the first FINE_END (85%) of the travel, and it
    #     only reaches FINE_TOP (40%) of the ceiling. Almost all of the
    #     stick lives here, so almost all of the stick is slow and easy to
    #     aim, and a power curve (STICK_EXPO) makes the middle finer still.
    #   * the FAST zone is the last 15%: from the fine top right up to the
    #     full ceiling. The whole 40 is only yours when the stick is all the
    #     way over.
    #
    # At the seam both zones give the same number - 40% of the ceiling - so
    # the speed never jumps, it only changes slope. This is exactly how a
    # transmitter's "dual rate" and "expo" settings work.
    x = apply_deadband(value) / 100.0
    magnitude = abs(x)
    if magnitude == 0.0:
        return 0.0

    if magnitude <= FINE_END:
        shaped = FINE_TOP * (magnitude / FINE_END) ** STICK_EXPO
    else:
        over = (magnitude - FINE_END) / (1.0 - FINE_END)
        shaped = FINE_TOP + (1.0 - FINE_TOP) * over

    # The floor. Below about 5% a V5 motor often will not turn at all, so
    # the gentlest command is lifted to MIN_MOVE_FRACTION of the ceiling.
    # That is a step in the maths, but the ramp below turns it into a push.
    floor = ceiling * MIN_MOVE_FRACTION
    power = floor + (ceiling - floor) * shaped
    return power if x > 0 else -power

def ramp_towards(current, target, step):
    # Walk one step towards the target and never overshoot it. Called once
    # per channel per loop, this is a slew rate limiter: it caps how fast a
    # command is allowed to change.
    #
    # Note it limits the RATE, it does not average the signal. A low-pass
    # filter would also smooth things out, but it would wash out the fine
    # detail and add a lag you can feel. This is the trick WPILib uses.
    if target > current + step:
        return current + step
    if target < current - step:
        return current - step
    return target

def loop_seconds(now_ms, last_ms):
    # How long the last pass around the driver loop took, in seconds.
    #
    # This used to be assumed: "the loop is 20 ms, so the arm turns 12
    # degrees". That is only true if the loop really is 20 ms, and the V5
    # makes no such promise - every device read in the loop costs time, and
    # the busier the brain is the longer a pass takes. If a pass takes 60 ms
    # the arm travels three times as far as that sum says, which is exactly
    # the situation where a soft limit gets walked straight through.
    #
    # So measure it instead. A short reading is never believed - anything
    # below one loop is rounded up - because under-estimating the time
    # under-estimates the travel, and that is the unsafe direction to be
    # wrong in.
    if last_ms is None:
        elapsed = LOOP_MS
    else:
        elapsed = now_ms - last_ms
        if elapsed < LOOP_MS:
            elapsed = LOOP_MS
    return (elapsed / 1000.0) * CASCADE_SAFETY_FACTOR

def cascade_travel(speed_percent, dt_seconds):
    # How many degrees the arm should turn before the program next looks at
    # it: at this speed, over this much time.
    return CASCADE_DEG_PER_SECOND * (speed_percent / 100.0) * dt_seconds

def arcade(forward, turn):
    # Split arcade: the left side gets forward + turn and the right side
    # gets forward - turn.
    #
    # Adding the two can ask for more than DRIVE_SPEED - up to twice it if
    # you drive and turn hard at the same time - so whenever that happens
    # BOTH sides are scaled down together. Scaling both keeps the shape of
    # the turn; clipping just one side would change where the robot goes.
    #
    # The result is the promise that matters: no wheel is ever asked for
    # more than DRIVE_SPEED, which is the number you set at the top.
    left = forward + turn
    right = forward - turn
    biggest = max(abs(left), abs(right))
    if biggest > DRIVE_SPEED:
        scale = float(DRIVE_SPEED) / biggest
        left = left * scale
        right = right * scale
    return left, right

def run_group(group, speed, was_moving):
    # Spin a pair of motors together. Stop once when you let go, so the
    # HOLD stopping mode can keep the part where it is.
    if speed != 0:
        group.spin(FORWARD, speed, PERCENT)
        return True
    if was_moving:
        group.stop()
    return False

def pad(text, width):
    return text + " " * (width - len(text))

# ==========================================================================
#  STRAIN GUARD
# ==========================================================================

class StrainGuard:
    # Watches one mechanism and eases off when it strains.
    #
    # There are two ways to be in trouble, and either one is enough:
    #   * pulling more CURRENT than this mechanism should need, which is what
    #     the numbers in the settings block are for, or
    #   * being asked to move and not MOVING, which the encoder reports and
    #     which needs no number guessed in amps.
    #
    # Two things happen when it strains, and they do different jobs:
    #   * the SPEED is eased down (slowly, a little per loop, so you never
    #     feel a step) - this stops the mechanism mashing against whatever
    #     it has hit
    #   * a TORQUE ceiling is applied, which really is the protection: the
    #     motor physically cannot pull more than that many amps, so it
    #     cannot cook itself no matter how long you hold the button
    #
    # current(CurrentUnits.AMP) on a MotorGroup adds up every motor in the
    # group, so the number is the whole mechanism's appetite, not one
    # motor's.
    def __init__(self, mechanism, strain_amps, ease_amps, relaxed_amps,
                 ease_speed, strain_loops, ease_step, recover_step,
                 blocked_fraction=BLOCKED_FRACTION,
                 blocked_loops=BLOCKED_LOOPS,
                 blocked_min_ask=BLOCKED_MIN_ASK):
        self.mechanism = mechanism
        self.strain_amps = strain_amps    # amps that count as "working too hard"
        self.ease_amps = ease_amps        # torque ceiling while straining
        self.relaxed_amps = relaxed_amps  # torque ceiling the rest of the time
        self.ease_speed = ease_speed      # how far down the speed may ease (0..1)
        self.strain_loops = strain_loops  # loops in a row before we believe it
        self.ease_step = ease_step        # how fast the easing comes on
        self.recover_step = recover_step  # how fast it lets go again
        self.factor = 1.0                 # 1.0 = full speed, smaller = gentler
        self.counting = 0
        self.straining = False
        self.capped = None                # the ceiling we last sent

        # The encoder half. "speed" is kept because the brain screen shows it
        # and a second read of the motor would be another message on the wire.
        self.blocked_fraction = blocked_fraction
        self.blocked_loops = blocked_loops
        self.blocked_min_ask = blocked_min_ask
        self.blocked_count = 0
        self.blocked = False
        self.speed = 0.0

    def amps(self):
        return self.mechanism.current(CurrentUnits.AMP)

    def velocity(self):
        # MotorGroup.velocity() reports the FIRST motor of the group, in
        # percent of its own top speed - the same scale we command in, so the
        # two can be compared directly with no gear maths in the way.
        return self.mechanism.velocity(VelocityUnits.PERCENT)

    def clear_block(self):
        # Called when a mechanism is being stopped on purpose, so that a
        # deliberate stop is never mistaken for a mechanism in trouble.
        self.blocked_count = 0
        self.blocked = False

    def is_blocked(self, commanded):
        # Told to spin, and not spinning.
        #
        # The comparison is against what we actually asked the motor for
        # (after any easing), not the original button value, so easing the
        # speed down lowers the bar as well and the guard cannot chase itself
        # into a stall it made up.
        if not USE_VELOCITY_CHECK or abs(commanded) < self.blocked_min_ask:
            # Nothing meaningful is being asked, so do not judge - and do not
            # spend a bus read finding out.
            self.clear_block()
            self.speed = 0.0
            return False

        self.speed = self.velocity()
        if abs(self.speed) < abs(commanded) * self.blocked_fraction:
            self.blocked_count += 1
        else:
            self.blocked_count = 0
        self.blocked = self.blocked_count >= self.blocked_loops
        return self.blocked

    def set_cap(self, amps):
        # Only talk to the motor when the number actually changes - every
        # one of these calls is a message out to the motor on the wire.
        if not USE_TORQUE_LIMITS or amps == self.capped:
            return
        self.capped = amps
        self.mechanism.set_max_torque(amps, CurrentUnits.AMP)

    def watch(self, asked):
        if asked == 0:
            # Nothing is being asked of this mechanism, so there is nothing
            # to ease. Put the normal torque ceiling back and let the brake
            # hold it where it is. The eased factor is deliberately kept,
            # so if you press straight back into the same obstacle it is
            # still being gentle with itself.
            self.counting = 0
            self.straining = False
            self.clear_block()
            self.set_cap(self.relaxed_amps)
            return 0

        commanded = asked * self.factor

        over_amps = self.amps() >= self.strain_amps
        not_moving = self.is_blocked(commanded)

        if over_amps or not_moving:
            self.counting += 1
            if self.counting >= self.strain_loops:
                self.straining = True
                self.factor = max(self.ease_speed, self.factor - self.ease_step)
        else:
            self.counting = 0
            self.straining = False
            self.factor = min(1.0, self.factor + self.recover_step)

        self.set_cap(self.ease_amps if self.straining else self.relaxed_amps)
        return asked * self.factor

# ==========================================================================
#  SCREENS
# ==========================================================================

def show_brain(lines):
    for i in range(len(lines)):
        brain.screen.set_cursor(i + 1, 1)
        brain.screen.clear_row()
        brain.screen.print(lines[i])

def show_controller(lines):
    controller_1.screen.clear_screen()
    for i in range(len(lines)):
        controller_1.screen.set_cursor(i + 1, 1)
        controller_1.screen.print(lines[i])

# ==========================================================================
#  DRIVER CONTROL
# ==========================================================================

def driver():
    # One ramp value per channel. Each one is its own little filter - you
    # must not share one between two inputs, or they fight each other.
    ramped_left = 0.0
    ramped_right = 0.0
    ramped_cascade = 0.0
    ramped_claw = 0.0
    ramped_toggle = 0.0

    # Motors that are stopped only need telling once.
    cascade_moving = False
    claw_moving = False
    toggle_moving = False

    cascade_guard = StrainGuard(
        cascade,
        strain_amps=CASCADE_STRAIN_AMPS,
        ease_amps=CASCADE_EASE_AMPS,
        relaxed_amps=TORQUE_FULL_AMPS,
        ease_speed=CASCADE_EASE_SPEED,
        strain_loops=CASCADE_STRAIN_LOOPS,
        ease_step=CASCADE_EASE_STEP,
        recover_step=CASCADE_RECOVER_STEP,
    )
    claw_guard = StrainGuard(
        claw,
        strain_amps=CLAW_STRAIN_AMPS,
        ease_amps=CLAW_HOLD_AMPS,
        relaxed_amps=CLAW_HOLD_AMPS,
        ease_speed=CLAW_HOLD_SPEED,
        strain_loops=CLAW_STRAIN_LOOPS,
        ease_step=CLAW_EASE_STEP,
        recover_step=CLAW_RECOVER_STEP,
    )

    # Look the full stop button up ONCE, here, rather than every pass. Doing
    # it here also means a typo in STOP_BUTTON is something you can read on
    # the screen at startup, instead of an exception thrown while the robot is
    # moving. (A misspelt name in this program has done exactly that before.)
    #
    # If the name is not a button, the program says so and refuses to drive.
    # A robot that will not move is the safe way to be wrong.
    stop_button = getattr(controller_1, 'button' + STOP_BUTTON, None)
    if stop_button is None:
        show_brain([
            "STOP_BUTTON is wrong:",
            "   " + STOP_BUTTON,
            "",
            "Use one of A B X Y",
            "   Left Right Up Down",
            "   L1 L2 R1 R2",
            "",
            "Fix it in SETTINGS, then run",
            "the program again.",
        ])
        return

    shown = None
    loops = 0
    last_loop_ms = None   # measured from brain.timer, never assumed
    stopped = False       # is the full stop button being held right now?

    while True:
        # How long the last pass around this loop took. Measured, not assumed
        # - see loop_seconds(). brain.timer is the brain's own clock, so this
        # costs no device traffic and does not itself slow the loop down.
        now_ms = brain.timer.time(MSEC)
        dt_seconds = loop_seconds(now_ms, last_loop_ms)
        last_loop_ms = now_ms

        # --- FULL STOP ----------------------------------------------------
        # Looked at before anything else, so that once this button is held
        # there is no path through the rest of the loop that reaches a motor.
        #
        # The ramps are pinned to zero rather than simply left alone: the ramp
        # is what would otherwise carry a number straight through the stop,
        # and let go at speed the moment the button came back up. From zero it
        # has to build up again, so releasing can never make the robot jump.
        if stop_button.pressing():
            ramped_left = 0.0
            ramped_right = 0.0
            ramped_cascade = 0.0
            ramped_claw = 0.0
            ramped_toggle = 0.0
            if not stopped:
                # Tell each motor once, not fifty times a second.
                stopped = True
                left_drive.stop()
                right_drive.stop()
                cascade.stop()
                claw.stop()
                toggle.stop()
                # A mechanism stopped on purpose is not one in trouble.
                cascade_guard.clear_block()
                claw_guard.clear_block()
            show_brain([
                "***  FULL  STOP  ***",
                "",
                "Every motor is stopped.",
                "",
                "Let go of " + STOP_BUTTON + " to drive again.",
                "",
                "Nothing moves while this",
                "button is held down.",
            ])
            show_controller(["STOP", STOP_BUTTON + " held", ""])
            # Force the normal screen to be redrawn when this is released.
            shown = None
            loops += 1
            wait(LOOP_MS, MSEC)
            continue
        stopped = False

        # --- Split arcade drive -------------------------------------------
        forward = stick_shape(controller_1.axis3.position(), DRIVE_SPEED)
        turn = stick_shape(controller_1.axis1.position(), TURN_SPEED)
        want_left, want_right = arcade(forward, turn)
        ramped_left = ramp_towards(ramped_left, want_left, RAMP_PER_LOOP)
        ramped_right = ramp_towards(ramped_right, want_right, RAMP_PER_LOOP)
        left_drive.spin(FORWARD, ramped_left, PERCENT)
        right_drive.spin(FORWARD, ramped_right, PERCENT)

        # --- Cascade: L1 up, L2 down --------------------------------------
        asked_cascade = 0.0
        if controller_1.buttonL1.pressing():
            asked_cascade = CASCADE_SPEED
        elif controller_1.buttonL2.pressing():
            asked_cascade = -CASCADE_SPEED

        # Where is the arm? One read of the motor, used by both checks below -
        # asking the motor is not free.
        cascade_position = cascade.position(DEGREES)

        # Close to the floor, aim for a crawl instead of the full speed. The
        # ramp then has the whole last stretch to slow the arm down, so it
        # arrives at the limit gently and can rest right on it.
        #
        # This happens BEFORE the guard so that the guard judges the command
        # the arm is really being given. Left until afterwards, a creeping arm
        # would be measured against full speed and read as "barely moving".
        if (asked_cascade * CASCADE_DOWN_SIGN > 0
                and cascade_position - CASCADE_LOWER_LIMIT <= CASCADE_CREEP_BAND):
            asked_cascade = CASCADE_CREEP_SPEED * CASCADE_DOWN_SIGN

        # Strain easing next, so the ramp below smooths whatever the guard
        # decided.
        asked_cascade = cascade_guard.watch(asked_cascade)

        ramped_cascade = ramp_towards(ramped_cascade, asked_cascade, RAMP_PER_LOOP)

        # The soft lower limit. There is no physical stop on this arm, so we
        # simply refuse to send a command that would take it below where it
        # started.
        #
        # The limit is checked against the command we are ABOUT to send, not
        # the last one, because the ramp may have just made that command
        # bigger than the loop before.
        #
        # The travel is worked out from the measured loop time rather than an
        # assumed 20 ms - see loop_seconds(). That is what stops a slow loop
        # from stepping the arm straight through the limit.
        next_position = cascade_position + cascade_travel(ramped_cascade, dt_seconds)

        at_floor = (ramped_cascade * CASCADE_DOWN_SIGN > 0
                    and next_position <= CASCADE_LOWER_LIMIT)

        # A limit is a limit. The ramp exists to make driving feel smooth, not
        # to soften a stop, so at the floor the command goes straight to zero
        # and the brake does the rest. Coming down at the creep speed above is
        # what makes "the floor" mean within about a degree of the limit
        # instead of a whole fast step above it.
        #
        # The ramp itself is brought straight down to the crawl rather than to
        # nothing. Zeroing it would make the ramp start over from nothing on
        # the next loop and nudge the arm down again; leaving it alone would
        # let the arm set off again at whatever number the ramp happened to be
        # holding when the floor stopped it, which can be a lot bigger than a
        # crawl. The crawl is the most the arm is ever allowed from here, so
        # the ramp is put there now and the next move can only ever be a crawl.
        if at_floor:
            send_cascade = 0.0
            ramped_cascade = CASCADE_CREEP_SPEED * CASCADE_DOWN_SIGN
            # Being stopped on purpose is not a mechanism in trouble, so the
            # guard is told not to read this as "asked to move and did not" -
            # otherwise sitting on the floor with the button held would slowly
            # ease the arm for no reason.
            cascade_guard.clear_block()
        else:
            send_cascade = ramped_cascade

        cascade_moving = run_group(cascade, send_cascade, cascade_moving)

        # --- Claw: R1 close, R2 open --------------------------------------
        # The claw's current ceiling is on the whole time, so the moment it
        # meets an object it grips firmly and then simply cannot pull any
        # harder. The guard then eases the closing speed down as well.
        asked_claw = 0.0
        if controller_1.buttonR1.pressing():
            asked_claw = CLAW_SPEED
        elif controller_1.buttonR2.pressing():
            asked_claw = -CLAW_SPEED
        asked_claw = claw_guard.watch(asked_claw)
        ramped_claw = ramp_towards(ramped_claw, asked_claw, RAMP_PER_LOOP)
        claw_moving = run_group(claw, ramped_claw, claw_moving)

        # --- Toggle: Up one way, Down the other way -----------------------
        # The toggle is the one exception: it gets the ramp, so it does not
        # slam, but no stick curve and no strain easing. It is a simple
        # two-position thing - you hold the button until it is where you
        # want it.
        asked_toggle = 0.0
        if controller_1.buttonUp.pressing():
            asked_toggle = TOGGLE_SPEED
        elif controller_1.buttonDown.pressing():
            asked_toggle = -TOGGLE_SPEED
        ramped_toggle = ramp_towards(ramped_toggle, asked_toggle, RAMP_PER_LOOP)
        toggle_moving = run_group(toggle, ramped_toggle, toggle_moving)

        # --- Words for the screens ----------------------------------------
        if ramped_cascade > 0:
            cascade_text = "UP"
        elif ramped_cascade < 0:
            cascade_text = "DOWN"
        else:
            cascade_text = "hold"
        if at_floor:
            cascade_text = "FLOOR"

        if ramped_claw > 0:
            claw_text = "CLOSE"
        elif ramped_claw < 0:
            claw_text = "OPEN"
        else:
            claw_text = "hold"

        if ramped_toggle > 0:
            toggle_text = "one way"
        elif ramped_toggle < 0:
            toggle_text = "other way"
        else:
            toggle_text = "hold"

        brain_lines = [
            "CASCADE ROBOT DRIVE v2",
            "L stick drive   R stick turn",
            "stick  L" + pad("%+6.1f" % forward, 9) + "R" + pad("%+6.1f" % turn, 9),
            "wheels L" + pad("%+6.1f" % ramped_left, 9) + "R" + pad("%+6.1f" % ramped_right, 9),
            "cascade " + pad("%+7.1f" % cascade_position, 9) + "deg " + cascade_text,
            "  amps " + pad("%4.2f" % cascade_guard.amps(), 6)
                + " vel " + pad("%+5.1f" % cascade_guard.speed, 7)
                + " ease " + str(int(cascade_guard.factor * 100)) + "%"
                + (" BLOCKED" if cascade_guard.blocked else ""),
            "claw " + pad(claw_text, 6)
                + " amps " + pad("%4.2f" % claw_guard.amps(), 6)
                + " vel " + pad("%+5.1f" % claw_guard.speed, 7)
                + " ease " + str(int(claw_guard.factor * 100)) + "%"
                + (" BLOCKED" if claw_guard.blocked else ""),
            "floor " + pad("%+6.1f" % CASCADE_LOWER_LIMIT, 9) + " toggle " + toggle_text,
            "",
            "L1/L2 cascade R1/R2 claw Up/Dn toggle" + "  " + STOP_BUTTON + "=STOP",
        ]

        controller_lines = [
            "L" + "%+d" % int(ramped_left) + " R" + "%+d" % int(ramped_right),
            "casc " + cascade_text,
            "claw " + claw_text,
        ]

        # Redraw when something changed, and refresh the brain now and then
        # so the battery and cable readouts stay honest.
        if brain_lines != shown or loops % 25 == 0:
            shown = brain_lines
            show_brain(brain_lines)
            show_controller(controller_lines)

        loops += 1
        wait(LOOP_MS, MSEC)

# ==========================================================================
#  STARTUP
# ==========================================================================

# Every motor on this robot is on a pair or holds something up, so HOLD is
# the safe choice: let go and it stays put instead of falling or drifting.
for motor in (drive_left_front_11, drive_left_back_17,
              drive_right_front_1, drive_right_back_10):
    motor.set_stopping(BRAKE)
for motor in (cascade_left_13, cascade_right_2,
              toggle_18, toggle_8, claw_16):
    motor.set_stopping(HOLD)

# The arm is hanging free right now, so this is its natural resting place.
# Zeroing the encoder here is what makes the floor a simple number, and it
# means the limit is always measured from where the arm really sits - not
# from wherever it happened to be when the robot was switched on.
cascade.reset_position()

# The claw's current ceiling is on for the whole run. It is what lets the
# claw squeeze an object and keep holding it without cooking the motor.
if USE_TORQUE_LIMITS:
    claw.set_max_torque(CLAW_HOLD_AMPS, CurrentUnits.AMP)

brain.screen.set_font(FontType.MONO15)
brain.screen.clear_screen()
brain.screen.print("CASCADE ROBOT DRIVE v2")
brain.screen.set_cursor(2, 1)
brain.screen.print("Starting...")

controller_1.rumble(".")

driver()
