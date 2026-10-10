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
# 	Project:      Cascade Robot Drive Limits
#	Description:  Drive v1 (Cascade Robot Drive) plus a live angle for
#	              every motor on the brain screen, refreshed all the time,
#	              and an optional soft limit for each mechanism motor. Use
#	              it to see how far the cascade, the toggle and the claw
#	              turn, and later to type those numbers in as limits.
#
#	Buttons (the same as drive v1, except the toggle):
#	Left stick up/down ..... drive forward / backward
#	Right stick left/right . turn left / right
#	L1 ......... cascade up          L2 ......... cascade down
#	R1 ......... claw close          R2 ......... claw open
#	Right ...... toggle one way      Y .......... toggle the other way
#
#	Every motor's count starts at 0 when the program starts, so start
#	every run with the cascade all the way DOWN and the claw fully OPEN
#	(and the toggle in the same place each time).
#
# ------------------------------------------

# Library imports
from vex import *

# --- Settings (change these to tune the robot) ---

# Gear cartridge inside each 11W motor:
#   GREEN = GearSetting.RATIO_18_1 (200 RPM)
#   BLUE  = GearSetting.RATIO_6_1  (600 RPM)
#   RED   = GearSetting.RATIO_36_1 (100 RPM)
DRIVE_GEARS = GearSetting.RATIO_18_1
CASCADE_GEARS = GearSetting.RATIO_36_1
CLAW_GEARS = GearSetting.RATIO_18_1

# How hard each part pushes when you hold its button (percent).
# The drive sticks already scale themselves from 0 to 100, so these are
# a ceiling - the stick goes from nothing up to this number, never past it.
DRIVE_SPEED = 40
TURN_SPEED = 40
CASCADE_SPEED = 55   # cascade, claw and toggle raised from 40 on 2026-10-10
CLAW_SPEED = 55
TOGGLE_SPEED = 55

# Ignore tiny stick movements so the robot does not creep when you let go
DEADBAND = 5

# --- Limits (optional - fill these in after testing) ---
# The brain screen shows every motor's angle in degrees, refreshed ten
# times a second, plus the lowest ("min") and highest ("max") it has
# reached since the program started. Each angle starts at 0 and goes UP
# when the motor turns the way its button asks: drive forward, cascade up
# (L1), claw close (R1) and toggle Right.
#
# How to find a limit, for example the top of the cascade:
#   1. Start with the arm all the way down and these limits set to None.
#   2. Hold L1 until the arm is as high as it may safely go.
#   3. Read the angle (or "max") for Cascade L 13 and Cascade R 2.
#   4. Type a number a little below that into CASCADE_LEFT_13_HIGH and
#      CASCADE_RIGHT_2_HIGH, download, and check that it stops there.
#
# None = no limit.
#   LOW  = the smallest angle allowed (cascade down, claw open, toggle Y)
#   HIGH = the biggest angle allowed (cascade up, claw close, toggle Right)
# The two motors of a pair are on one shaft, so the pair stops as soon as
# EITHER motor reaches its limit.
# Cascade, measured on the robot (2026-10-10): from the base to the top the
# left motor turned 755 degrees (-9 to 746) and the right one 759 (-15 to
# 744). That interval is the constraint: started at the base, the cascade
# may only move between 0 and the interval, less a margin at the top.
CASCADE_TRAVEL = 755      # base to top, the shorter of the two sides
CASCADE_MARGIN = 25       # stop this many degrees short of the top
CASCADE_LEFT_13_LOW = 0
CASCADE_LEFT_13_HIGH = CASCADE_TRAVEL - CASCADE_MARGIN     # 730
CASCADE_RIGHT_2_LOW = 0
CASCADE_RIGHT_2_HIGH = CASCADE_TRAVEL - CASCADE_MARGIN     # 730
TOGGLE_18_LOW = None
TOGGLE_18_HIGH = None
TOGGLE_8_LOW = None
TOGGLE_8_HIGH = None
# Claw, measured on the robot (2026-10-10): -16 fully open to 176 closed.
# Started fully open, it may only move between 0 and that interval, less a
# margin at the closed end.
CLAW_TRAVEL = 192         # fully open to fully closed
CLAW_MARGIN = 12          # stop this many degrees short of fully closed
CLAW_16_LOW = 0
CLAW_16_HIGH = CLAW_TRAVEL - CLAW_MARGIN                   # 180

# Near a limit the part slows down so it does not reach the limit at full
# speed: within its slow band (degrees) it goes no faster than
# LIMIT_SLOW_SPEED percent. Each mechanism has its own band, because the
# claw only turns about 190 degrees end to end while the cascade turns 750.
# The cascade goes up against gravity, so if it stops moving in that last
# stretch, raise LIMIT_SLOW_SPEED. A band of 0 turns the slowing off.
CASCADE_SLOW_BAND = 60
TOGGLE_SLOW_BAND = 20
CLAW_SLOW_BAND = 20
LIMIT_SLOW_SPEED = 20

# A part keeps moving for a moment after the program decides to stop it,
# so it is stopped this many times earlier than a straight sum says.
LIMIT_SAFETY_FACTOR = 1.5

# --- Response (less delay between the controller and the robot) ---
# False = the drive asks for a speed, exactly like drive v1. This is the
# one that has been driven forward AND backward on the robot.
# True = the drive is sent a voltage instead, which the motors react to a
# little sooner. It was the default for one test and the robot would not
# drive backward, so it now sends the direction separately (FORWARD or
# REVERSE with a positive voltage). Try it again only on blocks.
# Letting go of the sticks brakes either way.
DRIVE_USE_VOLTAGE = False

# How often the program reads the controller, in milliseconds
LOOP_MS = 10

# Drawing a screen is slow, and the controller's screen goes over the
# radio. Drive v1 redrew both whenever a number on them changed, which is
# every pass while the sticks are moving, and that held up every pass. Now
# each screen only redraws this often. This is the main fix for the delay.
BRAIN_REDRAW_MS = 100
CONTROLLER_REDRAW_MS = 250

# --- Devices ---
# Front/back/left/right are as the robot drives forward.
# This block is the same in every version of the program (match, test and
# drive). If the test program shows a motor turning the wrong way, flip its
# True/False here AND in the other versions.
#
# Every pair on the same shaft has the same True/False on both sides, so
# both motors of a pair always turn the same way as each other.

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

# --- Helpers ---

def apply_deadband(value):
    if abs(value) < DEADBAND:
        return 0
    return value

def arcade(forward, turn):
    # Split arcade: the left side gets forward + turn and the right side
    # gets forward - turn. If that asks for more than DRIVE_SPEED, BOTH
    # sides are scaled down together, which keeps the shape of the turn.
    left = forward + turn
    right = forward - turn
    biggest = max(abs(left), abs(right))
    if biggest > DRIVE_SPEED:
        scale = float(DRIVE_SPEED) / biggest
        left = left * scale
        right = right * scale
    return left, right

def spin_volts(group, volts):
    if volts < 0:
        group.spin(REVERSE, -volts, VoltageUnits.VOLT)
    else:
        group.spin(FORWARD, volts, VoltageUnits.VOLT)

def drive_wheels(left, right, was_driving):
    # Send both sides of the drive. Sticks let go = stop once, so the BRAKE
    # stopping mode holds the robot still. Returns whether it is driving.
    if left == 0 and right == 0:
        if was_driving:
            left_drive.stop()
            right_drive.stop()
        return False
    if DRIVE_USE_VOLTAGE:
        # Never a negative voltage: backward is REVERSE with a positive number
        spin_volts(left_drive, left * 12.0 / 100.0)
        spin_volts(right_drive, right * 12.0 / 100.0)
    else:
        left_drive.spin(FORWARD, left, PERCENT)
        right_drive.spin(FORWARD, right, PERCENT)
    return True

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

def full_speed_deg_per_s(gears):
    # How many degrees a motor's output turns in one second at 100%
    if gears == GearSetting.RATIO_36_1:
        return 600.0     # red, 100 RPM
    if gears == GearSetting.RATIO_6_1:
        return 3600.0    # blue, 600 RPM
    return 1200.0        # green, 200 RPM (and every 5.5W motor)

# --- Position tracking and limits ---

class Tracked:
    # One motor: where it is now, the lowest and highest it has been since
    # the program started, and its limits.

    def __init__(self, name, motor, low, high, gears, slow_band=0):
        self.name = name
        self.motor = motor
        self.low = low
        self.high = high
        self.slow_band = slow_band
        self.deg_per_s = full_speed_deg_per_s(gears)
        self.now = 0.0
        self.min_seen = 0.0
        self.max_seen = 0.0
        self.at_limit = False

    def read(self):
        self.now = self.motor.position(DEGREES)
        if self.now < self.min_seen:
            self.min_seen = self.now
        if self.now > self.max_seen:
            self.max_seen = self.now

    def room(self, speed):
        # Degrees left before the limit in the direction it is being driven,
        # or None if there is no limit that way
        if speed > 0 and self.high is not None:
            return self.high - self.now
        if speed < 0 and self.low is not None:
            return self.now - self.low
        return None

    def limit_text(self):
        if self.low is None and self.high is None:
            return "none"
        low = "-" if self.low is None else "%d" % int(self.low)
        high = "-" if self.high is None else "%d" % int(self.high)
        return low + ".." + high

def limit_speed(parts, speed, pass_s):
    # Slow a part down near its limit and stop it at the limit. parts is
    # every motor of one mechanism (both motors of a pair). Returns the
    # speed it is allowed to go.
    for part in parts:
        part.at_limit = False
    if speed == 0:
        return 0

    for part in parts:
        room = part.room(speed)
        if room is not None and part.slow_band > 0 and room <= part.slow_band:
            if speed > 0:
                speed = min(speed, LIMIT_SLOW_SPEED)
            else:
                speed = max(speed, -LIMIT_SLOW_SPEED)

    # How far it will turn before the program looks again, worked out from
    # how long a pass really takes, so a slow pass cannot step past a limit.
    # Never less than 20 ms: a new command takes about that long to reach
    # the motor and take effect.
    look_ahead_s = max(pass_s, 0.02)
    for part in parts:
        room = part.room(speed)
        if room is None:
            continue
        travel = part.deg_per_s * abs(speed) / 100.0 * look_ahead_s * LIMIT_SAFETY_FACTOR
        if room <= travel:
            part.at_limit = True
            return 0
    return speed

# (name on the screen, motor, lowest allowed, highest allowed, cartridge,
#  slow band)
# LF/LB/RF/RB = left front, left back, right front, right back.
# The drive wheels spin round and round, so they have no limits. They are
# listed so you can see all four count up when you drive forward.
drive_parts = [
    Tracked("Drive LF 11", drive_left_front_11, None, None, DRIVE_GEARS),
    Tracked("Drive LB 17", drive_left_back_17, None, None, DRIVE_GEARS),
    Tracked("Drive RF 1", drive_right_front_1, None, None, DRIVE_GEARS),
    Tracked("Drive RB 10", drive_right_back_10, None, None, DRIVE_GEARS),
]
cascade_parts = [
    Tracked("Cascade L 13", cascade_left_13, CASCADE_LEFT_13_LOW, CASCADE_LEFT_13_HIGH,
            CASCADE_GEARS, CASCADE_SLOW_BAND),
    Tracked("Cascade R 2", cascade_right_2, CASCADE_RIGHT_2_LOW, CASCADE_RIGHT_2_HIGH,
            CASCADE_GEARS, CASCADE_SLOW_BAND),
]
toggle_parts = [
    Tracked("Toggle 18", toggle_18, TOGGLE_18_LOW, TOGGLE_18_HIGH,
            GearSetting.RATIO_18_1, TOGGLE_SLOW_BAND),
    Tracked("Toggle 8", toggle_8, TOGGLE_8_LOW, TOGGLE_8_HIGH,
            GearSetting.RATIO_18_1, TOGGLE_SLOW_BAND),
]
claw_parts = [
    Tracked("Claw 16", claw_16, CLAW_16_LOW, CLAW_16_HIGH, CLAW_GEARS, CLAW_SLOW_BAND),
]
mechanism_parts = cascade_parts + toggle_parts + claw_parts
# The mechanisms are listed first on the screen, the drive underneath
all_parts = mechanism_parts + drive_parts

# --- Screens ---

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

def motor_line(part):
    flag = " STOP" if part.at_limit else ""
    return (pad(part.name, 13)
            + "%6d%6d%6d" % (int(part.now), int(part.min_seen), int(part.max_seen))
            + "  " + part.limit_text() + flag)

def direction_text(speed, plus_word, minus_word):
    if speed > 0:
        return plus_word
    if speed < 0:
        return minus_word
    return "hold"

# --- Driver control ---

def driver():
    driving = False
    cascade_moving = False
    claw_moving = False
    toggle_moving = False
    last_ms = None
    last_brain_ms = None
    last_controller_ms = None
    shown_controller = None
    slowest_pass_ms = 0

    while True:
        # How long the last pass took, measured rather than assumed
        now_ms = brain.timer.time(MSEC)
        pass_ms = LOOP_MS
        if last_ms is not None:
            pass_ms = max(LOOP_MS, now_ms - last_ms)
        last_ms = now_ms
        pass_s = pass_ms / 1000.0
        slowest_pass_ms = max(slowest_pass_ms, pass_ms)

        for part in mechanism_parts:
            part.read()

        # --- Split arcade drive ---
        forward = apply_deadband(controller_1.axis3.position()) * DRIVE_SPEED / 100.0
        turn = apply_deadband(controller_1.axis1.position()) * TURN_SPEED / 100.0
        left, right = arcade(forward, turn)
        driving = drive_wheels(left, right, driving)

        # --- Cascade: L1 up, L2 down ---
        cascade_speed = 0
        if controller_1.buttonL1.pressing():
            cascade_speed = CASCADE_SPEED
        elif controller_1.buttonL2.pressing():
            cascade_speed = -CASCADE_SPEED
        cascade_speed = limit_speed(cascade_parts, cascade_speed, pass_s)
        cascade_moving = run_group(cascade, cascade_speed, cascade_moving)

        # --- Claw: R1 close, R2 open ---
        claw_speed = 0
        if controller_1.buttonR1.pressing():
            claw_speed = CLAW_SPEED
        elif controller_1.buttonR2.pressing():
            claw_speed = -CLAW_SPEED
        claw_speed = limit_speed(claw_parts, claw_speed, pass_s)
        claw_moving = run_group(claw, claw_speed, claw_moving)

        # --- Toggle: Right one way, Y the other way ---
        toggle_speed = 0
        if controller_1.buttonRight.pressing():
            toggle_speed = TOGGLE_SPEED
        elif controller_1.buttonY.pressing():
            toggle_speed = -TOGGLE_SPEED
        toggle_speed = limit_speed(toggle_parts, toggle_speed, pass_s)
        toggle_moving = run_group(toggle, toggle_speed, toggle_moving)

        # --- Screens: one at a time, and only every so often ---
        if last_brain_ms is None or now_ms - last_brain_ms >= BRAIN_REDRAW_MS:
            last_brain_ms = now_ms
            for part in drive_parts:
                part.read()
            stopped_at = [part.name for part in mechanism_parts if part.at_limit]
            lines = [
                "LIVE ANGLES (deg)   slowest pass %d ms" % slowest_pass_ms,
                pad("Motor", 13) + " angle   min   max  limit",
            ]
            for part in all_parts:
                lines.append(motor_line(part))
            if stopped_at:
                lines.append("AT LIMIT: " + ", ".join(stopped_at))
            else:
                lines.append("Drive L " + "%+d" % int(left) + "  R " + "%+d" % int(right)
                             + "   casc " + direction_text(cascade_speed, "UP", "DOWN"))
            lines.append("Start: cascade DOWN, claw OPEN.")
            show_brain(lines)
            slowest_pass_ms = 0
        elif last_controller_ms is None or now_ms - last_controller_ms >= CONTROLLER_REDRAW_MS:
            last_controller_ms = now_ms
            third = "Claw %+d" % int(claw_parts[0].now)
            if any(part.at_limit for part in mechanism_parts):
                third = third + " LIMIT"
            controller_lines = [
                "C13 %+d C2 %+d" % (int(cascade_parts[0].now), int(cascade_parts[1].now)),
                "T18 %+d T8 %+d" % (int(toggle_parts[0].now), int(toggle_parts[1].now)),
                third,
            ]
            # Only send it if it changed: every line is a message over the radio
            if controller_lines != shown_controller:
                shown_controller = controller_lines
                show_controller(controller_lines)

        wait(LOOP_MS, MSEC)

# --- Startup ---

# Every motor on this robot is on a pair or holds something up, so HOLD is
# the safe choice: let go and it stays put instead of falling or drifting.
for motor in (drive_left_front_11, drive_left_back_17,
              drive_right_front_1, drive_right_back_10):
    motor.set_stopping(BRAKE)
for motor in (cascade_left_13, cascade_right_2,
              toggle_18, toggle_8, claw_16):
    motor.set_stopping(HOLD)

# Every count starts from 0 here, so the numbers mean the same thing on
# every run - as long as each part starts in the same place
for part in all_parts:
    part.motor.set_position(0, DEGREES)

brain.screen.set_font(FontType.MONO15)
brain.screen.clear_screen()
brain.screen.print("CASCADE ROBOT - DRIVE LIVE ANGLES")
brain.screen.set_cursor(2, 1)
brain.screen.print("Starting...")

controller_1.rumble(".")

driver()
