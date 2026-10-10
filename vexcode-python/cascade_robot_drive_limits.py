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
#	Buttons (the same as drive v1, except the toggle and the claw):
#	Left stick up/down ..... drive forward / backward
#	Right stick left/right . turn left / right
#	L1 ......... cascade up          L2 ......... cascade down
#	R1 ......... claw open           R2 ......... claw close
#	             (one press turns the claw CLAW_TURN_DEGREES by itself;
#	              the buttons can be changed in the claw settings)
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
# For the drive these are the top speed, reached only with the stick
# pushed all the way (see the stick curve below).
DRIVE_SPEED = 40
TURN_SPEED = 40
CASCADE_SPEED = 55   # cascade, claw and toggle raised from 40 on 2026-10-10
CLAW_SPEED = 55      # how fast the claw turns after a press
TOGGLE_SPEED = 55

# Ignore tiny stick movements so the robot does not creep when you let go
DEADBAND = 5

# --- Stick curve (how the sticks turn into speed) ---
# A small push gives a small speed, and the top speed (DRIVE_SPEED or
# TURN_SPEED) only comes with the stick pushed all the way to the end.
# In between, the speed grows exponentially:
#   speed = top speed x (e^(CURVE x push) - 1) / (e^CURVE - 1)
# where push goes from 0 (just past the deadband) to 1 (stick at the end).
# A bigger CURVE is gentler in the middle; 0 is a straight line (the old
# feel). How much of the top speed you get:
#   CURVE   stick 10%   25%   50%   75%   100%
#     0           5%    21%   47%   74%   100%
#     1           3%    14%   35%   63%   100%
#     2           2%     8%   25%   53%   100%
#     3           1%     5%   16%   43%   100%
#     4           0%     2%   11%   34%   100%
DRIVE_CURVE = 2     # left stick, forward / backward
TURN_CURVE = 2      # right stick, turning

# --- Claw: one press turns it a set amount ---
# Press the open button once and the claw turns CLAW_TURN_DEGREES the open
# way by itself; press the close button once and it turns that much the
# close way. No need to hold the button. Each press is one more turn of
# CLAW_TURN_DEGREES from wherever the claw is - there is no limit.

# Which buttons open and close the claw. Any of these names:
#   "L1" "L2" "R1" "R2" "Up" "Down" "Left" "Right" "X" "Y" "A" "B"
# Don't pick one the cascade (L1, L2) or the toggle (Right, Y) already uses.
CLAW_OPEN_BUTTON = "R1"
CLAW_CLOSE_BUTTON = "R2"

# How many degrees one press turns the claw. The claw measured 192 degrees
# from fully open to fully closed (2026-10-10).
CLAW_TURN_DEGREES = 180

# Which way the motor turns to open the claw. If the open button closes
# it, swap FORWARD and REVERSE here.
CLAW_OPEN_DIRECTION = FORWARD
CLAW_CLOSE_DIRECTION = REVERSE

# The claw has no current or torque limit: it always has the motor's full
# strength. The only thing that sets how fast it turns is CLAW_SPEED above
# (100 = as fast as the motor goes).

# --- Limits (optional - fill these in after testing) ---
# The brain screen shows every motor's angle in degrees, refreshed ten
# times a second, plus the lowest ("min") and highest ("max") it has
# reached since the program started. Each angle starts at 0 and goes UP
# when the motor turns the way its button asks: drive forward, cascade up
# (L1) and toggle Right. (The claw is different - see above.)
#
# How to find a limit, for example the top of the cascade:
#   1. Start with the arm all the way down and these limits set to None.
#   2. Hold L1 until the arm is as high as it may safely go.
#   3. Read the angle (or "max") for Cascade L 13 and Cascade R 2.
#   4. Type a number a little below that into CASCADE_LEFT_13_HIGH and
#      CASCADE_RIGHT_2_HIGH, download, and check that it stops there.
#
# None = no limit.
#   LOW  = the smallest angle allowed (cascade down, toggle Y)
#   HIGH = the biggest angle allowed (cascade up, toggle Right)
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

# Near a limit the part slows down so it does not reach the limit at full
# speed: within its slow band (degrees) it goes no faster than
# LIMIT_SLOW_SPEED percent. Each mechanism has its own band, because the
# toggle turns much less than the cascade's 750 degrees.
# The cascade goes up against gravity, so if it stops moving in that last
# stretch, raise LIMIT_SLOW_SPEED. A band of 0 turns the slowing off.
# (The claw does not need one: the motor slows itself down at the end of
# each turn.)
CASCADE_SLOW_BAND = 60
TOGGLE_SLOW_BAND = 20
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
# radio - the same radio that brings the sticks and buttons to the robot.
# Drive v1 redrew both whenever a number on them changed, which is every
# pass while the sticks are moving, and that held up every pass. Now the
# brain redraws this often, and the controller gets at most ONE changed
# line this often (never the whole screen at once).
BRAIN_REDRAW_MS = 100
CONTROLLER_LINE_MS = 100

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

# --- Claw buttons ---
# The buttons named in the claw settings. Capitals don't matter ("r1" and
# "R1" are the same); a name that isn't a button stops the program with a
# message on the brain saying which setting to fix.
BUTTONS = {
    "L1": controller_1.buttonL1, "L2": controller_1.buttonL2,
    "R1": controller_1.buttonR1, "R2": controller_1.buttonR2,
    "UP": controller_1.buttonUp, "DOWN": controller_1.buttonDown,
    "LEFT": controller_1.buttonLeft, "RIGHT": controller_1.buttonRight,
    "X": controller_1.buttonX, "Y": controller_1.buttonY,
    "A": controller_1.buttonA, "B": controller_1.buttonB,
}

def button_named(name, setting):
    key = str(name).strip().upper()
    if key not in BUTTONS:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("%s = %s is not a button." % (setting, name))
        brain.screen.set_cursor(2, 1)
        brain.screen.print("Use L1 L2 R1 R2 Up Down Left Right X Y A B")
        raise ValueError("%s = %s is not a button" % (setting, name))
    return BUTTONS[key]

claw_open_button = button_named(CLAW_OPEN_BUTTON, "CLAW_OPEN_BUTTON")
claw_close_button = button_named(CLAW_CLOSE_BUTTON, "CLAW_CLOSE_BUTTON")

# --- Helpers ---

def stick_curve(value, curve, top_speed):
    # Turn a stick position (-100 to 100) into a speed (-top_speed to
    # top_speed) along the exponential curve in the settings
    size = abs(value)
    if size < DEADBAND:
        return 0
    push = min(1.0, (size - DEADBAND) / (100.0 - DEADBAND))
    if curve == 0:
        speed = push * top_speed
    else:
        speed = (math.exp(curve * push) - 1) / (math.exp(curve) - 1) * top_speed
    if value < 0:
        return -speed
    return speed

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

    def connected(self):
        # False if the brain can't see the motor (cable out or loose)
        try:
            return self.motor.installed()
        except Exception:
            return True

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
# The claw is listed for its live angle only: it has no limit
claw_parts = [
    Tracked("Claw 16", claw_16, None, None, CLAW_GEARS),
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

def show_controller_line(row, text):
    # One line, padded with spaces so it covers whatever was there before
    # (no clearing the screen, which would be another radio message)
    controller_1.screen.set_cursor(row, 1)
    controller_1.screen.print(pad(text[:19], 19))

def motor_line(part, plugged_in):
    if not plugged_in:
        return pad(part.name, 13) + "  UNPLUGGED - check cable"
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
    claw_word = "-"          # the claw's last press: OPEN or CLOSE
    open_was_pressed = False
    close_was_pressed = False
    toggle_moving = False
    last_ms = None
    last_brain_ms = None
    last_controller_ms = None
    shown_controller = [None, None, None]
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
        forward = stick_curve(controller_1.axis3.position(), DRIVE_CURVE, DRIVE_SPEED)
        turn = stick_curve(controller_1.axis1.position(), TURN_CURVE, TURN_SPEED)
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

        # --- Claw: one press turns it CLAW_TURN_DEGREES open or closed ---
        # Only the moment a button goes down counts, so holding it does
        # nothing more. The motor then makes the turn by itself (it does not
        # wait here) and holds where it ends up.
        open_pressed = claw_open_button.pressing()
        close_pressed = claw_close_button.pressing()
        if open_pressed and not open_was_pressed:
            claw_word = "OPEN"
            claw_16.spin_for(CLAW_OPEN_DIRECTION, CLAW_TURN_DEGREES, DEGREES, wait=False)
        elif close_pressed and not close_was_pressed:
            claw_word = "CLOSE"
            claw_16.spin_for(CLAW_CLOSE_DIRECTION, CLAW_TURN_DEGREES, DEGREES, wait=False)
        open_was_pressed = open_pressed
        close_was_pressed = close_pressed

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
            plugged = [part.connected() for part in all_parts]
            unplugged = [all_parts[i].name for i in range(len(all_parts)) if not plugged[i]]
            stopped_at = [part.name for part in mechanism_parts if part.at_limit]
            lines = [
                "LIVE ANGLES (deg)   slowest pass %d ms" % slowest_pass_ms,
                pad("Motor", 13) + " angle   min   max  limit",
            ]
            for i in range(len(all_parts)):
                lines.append(motor_line(all_parts[i], plugged[i]))
            if unplugged:
                lines.append(("UNPLUGGED: " + ", ".join(unplugged))[:46])
            elif stopped_at:
                lines.append("AT LIMIT: " + ", ".join(stopped_at))
            else:
                lines.append("Drive L " + "%+d" % int(left) + "  R " + "%+d" % int(right)
                             + "   casc " + direction_text(cascade_speed, "UP", "DOWN")
                             + "  claw " + claw_word)
            lines.append("Start: cascade DOWN, claw OPEN.")
            show_brain(lines)
            slowest_pass_ms = 0
        elif last_controller_ms is None or now_ms - last_controller_ms >= CONTROLLER_LINE_MS:
            last_controller_ms = now_ms
            third = "Claw %+d " % int(claw_parts[0].now) + claw_word
            if any(part.at_limit for part in mechanism_parts):
                third = third + " LIM"
            controller_lines = [
                "C13 %+d C2 %+d" % (int(cascade_parts[0].now), int(cascade_parts[1].now)),
                "T18 %+d T8 %+d" % (int(toggle_parts[0].now), int(toggle_parts[1].now)),
                third,
            ]
            # Send only the first line that changed: every line is a message
            # over the radio, so at most one goes each time
            for i in range(3):
                if controller_lines[i] != shown_controller[i]:
                    shown_controller[i] = controller_lines[i]
                    show_controller_line(i + 1, controller_lines[i])
                    break

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

# Full strength for every motor. A motor can keep a torque limit from the
# last program that ran (drive v2 limits the claw and the cascade), so it
# is set back to 100% here.
for part in all_parts:
    part.motor.set_max_torque(100, PERCENT)

# The claw makes each turn at CLAW_SPEED
claw_16.set_velocity(CLAW_SPEED, PERCENT)

brain.screen.set_font(FontType.MONO15)
brain.screen.clear_screen()
brain.screen.print("CASCADE ROBOT - DRIVE LIVE ANGLES")
brain.screen.set_cursor(2, 1)
brain.screen.print("Starting...")

controller_1.screen.clear_screen()
controller_1.rumble(".")

driver()
