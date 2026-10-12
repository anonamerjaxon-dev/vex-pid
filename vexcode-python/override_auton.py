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
# 	Project:      Override Auton
#	Description:  Autonomous for V5RC Override (2026-27), plus driver
#	              control. Copied from Cascade Robot Auton (the PID
#	              autonomous program) on 2026-10-10. Driver control is the
#	              same as Cascade Robot Drive Limits.
#
#	Autonomous (AUTON_ROUTINE = "override"):
#	  1. Flip the field Toggle behind the robot to our color.
#	  2. Set the preload pin on our alliance goal.
#	  3. Grab the yellow pin-in-cup 2 tiles out and stack it on top.
#	  Start: middle of our alliance wall, the back of the robot (the
#	  toggle arm) touching the wall, cascade DOWN, claw CLOSED on the
#	  preload, toggle arm at rest. OVERRIDE_AUTON.md has the route and
#	  distances.
#
#	Buttons (the same as Drive Limits):
#	Left stick up/down ..... drive forward / backward
#	Right stick left/right . turn left / right
#	L1 ......... cascade up          L2 ......... cascade down
#	R1 ......... claw open           R2 ......... claw close
#	             (one press turns the claw CLAW_TURN_DEGREES by itself)
#	Right ...... toggle one way      Y .......... toggle the other way
#
#	Plain Run starts driver control. To test the autonomous, HOLD A for
#	1 second (off the field only; B stops it), or use Timed Run on the V5
#	controller. At a tournament, field control starts it by itself.
#
# ------------------------------------------

# Library imports
from vex import *
import math

# --- Settings (change these to tune the robot) ---

# Which autonomous runs: "override", "pid_test", "odom_test" or "none".
# A name that isn't one of these stops the program at startup with a
# message on the brain (so a typo can't mean "no autonomous" in a match).
AUTON_ROUTINE = "override"

# Our alliance this match: "red" or "blue" (capitals don't matter; anything
# else stops the program at startup). The route is the same for both (the
# field is the same, turned around). Only the way the field Toggle is
# turned changes - see TOGGLE_FLIP_DEGREES below.
ALLIANCE = "red"

# Gear cartridge inside each 11W motor:
#   GREEN = GearSetting.RATIO_18_1 (200 RPM)
#   BLUE  = GearSetting.RATIO_6_1  (600 RPM)
#   RED   = GearSetting.RATIO_36_1 (100 RPM)
DRIVE_GEARS = GearSetting.RATIO_18_1
CASCADE_GEARS = GearSetting.RATIO_36_1
CLAW_GEARS = GearSetting.RATIO_18_1

# ===== Driver control (the same as Drive Limits) =====

# How hard each part pushes when you hold its button (percent).
# For the drive these are the top speed, reached only with the stick
# pushed all the way (see the stick curve below). The autonomous drives
# at these speeds too (AUTON_DRIVE_SPEED, AUTON_TURN_SPEED).
DRIVE_SPEED = 40
TURN_SPEED = 40
CASCADE_SPEED = 55
CLAW_SPEED = 55      # how fast the claw turns after a press (and in autonomous)
TOGGLE_SPEED = 55

# Ignore tiny stick movements so the robot does not creep when you let go
DEADBAND = 5

# --- Stick curve (how the sticks turn into speed) ---
# A small push gives a small speed, and the top speed (DRIVE_SPEED or
# TURN_SPEED) only comes with the stick pushed all the way to the end:
#   speed = top speed x (e^(CURVE x push) - 1) / (e^CURVE - 1)
# where push goes from 0 (just past the deadband) to 1 (stick at the end).
#   CURVE   stick 10%   25%   50%   75%   100%
#     0           5%    21%   47%   74%   100%
#     1           3%    14%   35%   63%   100%
#     2           2%     8%   25%   53%   100%
#     3           1%     5%   16%   43%   100%
DRIVE_CURVE = 2     # left stick, forward / backward
TURN_CURVE = 2      # right stick, turning

# --- Claw: one press turns it a set amount ---
# Press the open button once and the claw turns CLAW_TURN_DEGREES the open
# way by itself; press the close button once and it turns that much the
# close way. Each press is one more turn from wherever the claw is.
# The autonomous opens and closes the claw the same way.
# Button names: "L1" "L2" "R1" "R2" "Up" "Down" "Left" "Right" "X" "Y" "A" "B"
# Don't pick one already used: L1 L2 (cascade), Right Y (toggle), A B
# (start and stop the autonomous test).
CLAW_OPEN_BUTTON = "R1"
CLAW_CLOSE_BUTTON = "R2"

# How many degrees one press turns the claw. The claw measured 192 degrees
# from fully open to fully closed (2026-10-10).
CLAW_TURN_DEGREES = 180

# Which way the motor turns to open the claw. If the open button closes
# it, swap FORWARD and REVERSE here. (Checked on the robot 2026-10-10:
# FORWARD closes this claw, the same as the 10-08 direction test.)
CLAW_OPEN_DIRECTION = REVERSE
CLAW_CLOSE_DIRECTION = FORWARD

# --- Limits ---
# Cascade, measured on the robot (2026-10-10): from the base to the top the
# left motor turned 755 degrees and the right one 759. Started at the
# base, the cascade may only move between 0 and that, less a margin at the
# top. The autonomous cascade heights are kept inside these limits too.
# None = no limit.
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

# Near a limit the part slows down to LIMIT_SLOW_SPEED percent within its
# slow band (degrees). A band of 0 turns the slowing off.
CASCADE_SLOW_BAND = 60
TOGGLE_SLOW_BAND = 20
LIMIT_SLOW_SPEED = 20

# A part keeps moving for a moment after the program decides to stop it,
# so it is stopped this many times earlier than a straight sum says.
LIMIT_SAFETY_FACTOR = 1.5

# --- Response ---
# False = the drive asks for a speed (the way that has been driven
# forward AND backward on the robot). True = it is sent a voltage.
DRIVE_USE_VOLTAGE = False

# How often driver control reads the controller, in milliseconds
LOOP_MS = 10

# The brain redraws this often, and the controller gets at most ONE
# changed line this often (its screen shares the radio with the sticks)
BRAIN_REDRAW_MS = 100
CONTROLLER_LINE_MS = 100

# ===== Autonomous =====

# --- The robot ---
# The drivetrain is the outside of the robot: 35 studs front to back and
# 30 studs side to side, 0.5 inch per stud (17.5" x 15").
ROBOT_LENGTH_STUDS = 35
ROBOT_WIDTH_STUDS = 30
STUD_IN = 0.5

# Where the claw holds a pin, in studs, looking AT the front of the robot:
# PIN_STUD_ACROSS counted from the left edge you see (15 of 30 = the
# middle), PIN_STUDS_FROM_FRONT counted back from the front edge.
PIN_STUD_ACROSS = 15
PIN_STUDS_FROM_FRONT = 4

# --- The field ---
# Inches, measured from the MIDDLE of our alliance wall (its inside face):
#   X = to the right, as the drive team sees it from the alliance station
#   Y = straight out into the field
# The robot starts with its back against the wall at X 0. The goal and
# the yellow pin sit on tile seams (tiles are 24"). These come from the
# game manual's field drawing - measure your field and correct them.
GOAL_X = 24.0       # our alliance goal: 1 tile right, 1 tile out
GOAL_Y = 24.0
STACK_X = 24.0      # yellow pin in a cup: 1 tile right, 2 tiles out
STACK_Y = 48.0

# How far the robot backs away after setting the preload (it then turns
# toward the yellow pin from there), and at the very end
BACK_OFF_IN = 12.0
FINAL_BACK_OFF_IN = 8.0

# --- Mechanism positions: measure the GUESSES ---
# Cascade heights in motor degrees from the bottom (0 to 730). To measure
# one: run Drive Limits with the cascade down, hold the part where it
# should be (e.g. the preload pin just above a goal), and read the
# Cascade L 13 / R 2 angle on the brain.
# CASCADE_SET_DEG was measured on the robot (2026-10-10, the C13 / C2
# numbers on the controller): at 150 the claw drops the preload onto the
# goal. The robot drives in a little higher (CARRY) so the pin clears the
# goal, lowers to SET, then opens. The stack numbers are still guesses,
# worked out from that 150: the yellow pin-in-cup has to clear the top of
# the preload pin, about one pin higher. Measure them the same way.
CASCADE_CARRY_DEG = 170      # preload pin's bottom just above the goal
CASCADE_SET_DEG = 150        # MEASURED: the preload drops onto the goal from here
CASCADE_PICKUP_DEG = 0       # GUESS: claw at the height to grab the yellow pin-in-cup
CASCADE_STACK_DEG = 380      # GUESS: the cup's bottom just above the preload pin
CASCADE_STACK_SET_DEG = 360  # GUESS: the stack drops onto the preload pin from here

# How far the toggle arm turns (motor degrees) to flip the field Toggle
# from yellow to our color. Plus = the way the Right button turns it in
# Drive Limits (the angle goes UP). To measure: run Drive Limits, turn
# the toggle with Right until the field Toggle shows red, and read the
# Toggle 18 / 8 angle. For blue, check which way turns it to blue.
TOGGLE_FLIP_DEGREES = 120
TOGGLE_FLIP_DEGREES_BLUE = -TOGGLE_FLIP_DEGREES

# --- Testing the autonomous without a field ---
# Plain Run starts driver control - the autonomous only starts by itself
# from field control, a competition switch or Timed Run. To test it
# without those: put the robot in its starting spot, HOLD A for
# AUTON_TEST_HOLD_MS, and the autonomous runs right there. Press B to stop
# it at any time; the driver gets control back when it ends. A does
# nothing when field control or a competition switch is connected, so it
# can never start the autonomous in the middle of a real match.
AUTON_TEST_HOLD_MS = 1000
# 15 = stop the test after 15 seconds, like a match. 0 = run the whole
# routine, however long it takes (handy while tuning the route).
AUTON_TEST_SECONDS = 0

# --- Autonomous speed and time limits ---
AUTON_DRIVE_SPEED = DRIVE_SPEED     # percent, the normal drive speed
AUTON_TURN_SPEED = TURN_SPEED
CLAW_MOVE_MS = 800                  # longest one claw turn may take
CLAW_STALL_CHECK_MS = 150           # after this, a claw that stopped is done
MECHANISM_TIMEOUT_MS = 1500         # longest a cascade/toggle move may take

# Drive measurements (from the vex-pid README - check on the robot with
# AUTON_ROUTINE = "pid_test": forward 24" should be 24")
DRIVE_WHEEL_DIAMETER_IN = 3.25
DRIVE_MOTOR_GEAR_TEETH = 18   # gear on the motor
DRIVE_WHEEL_GEAR_TEETH = 60   # gear on the wheel
TRACK_WIDTH_IN = 12.0         # center of left wheels to center of right wheels

# Inertial sensor. None = not on the robot (port 9 is the radio). If you
# add one, put its port here and turns get much more accurate.
IMU_PORT = None

# Tracking (coordinate) wheels: unpowered omni wheels on Rotation sensors.
# None = not on the robot yet. Once they're on, set the ports and offsets
# and the autonomous code uses them automatically.
#   VERTICAL wheel measures forward/back.
#     OFFSET = inches right of the robot's turning center (left is -)
#   HORIZONTAL wheel measures sideways.
#     OFFSET = inches in front of the robot's turning center (behind is -)
TRACKER_WHEEL_DIAMETER_IN = 2.0
VERTICAL_TRACKER_PORT = None
VERTICAL_TRACKER_REVERSED = False
VERTICAL_TRACKER_OFFSET_IN = 0.0
HORIZONTAL_TRACKER_PORT = None
HORIZONTAL_TRACKER_REVERSED = False
HORIZONTAL_TRACKER_OFFSET_IN = 0.0

# PID gains. Output is volts (-12 to 12). These were tuned on a computer
# simulation of the drive gearing above, so retune them on the robot.
# Tuning: raise kp until it overshoots a little, raise kd until the
# overshoot goes away, then add a small ki only if it stops short.
# settle_error/settle_ticks: "done" once the error stays under
# settle_error for settle_ticks loops in a row (1 tick = 10 ms).
DRIVE_GAINS = {"kp": 2.0, "ki": 0.02, "kd": 6.0, "integral_limit": 100.0,
               "settle_error": 0.5, "settle_ticks": 10}        # error in inches
HEADING_GAINS = {"kp": 0.5, "ki": 0.01, "kd": 1.0, "integral_limit": 100.0,
                 "output_max": 4.0,
                 "settle_error": 1.0, "settle_ticks": 10}      # error in degrees
TURN_GAINS = {"kp": 0.4, "ki": 0.005, "kd": 2.0, "integral_limit": 300.0,
              "settle_error": 1.5, "settle_ticks": 10}         # error in degrees
CASCADE_GAINS = {"kp": 0.04, "ki": 0.0005, "kd": 0.2, "integral_limit": 2000.0,
                 "settle_error": 10.0, "settle_ticks": 10}     # error in motor degrees
TOGGLE_GAINS = {"kp": 0.1, "ki": 0.0, "kd": 0.3,
                "settle_error": 3.0, "settle_ticks": 10}       # error in motor degrees

# Extra volts that hold the cascade up against gravity
CASCADE_FEEDFORWARD_VOLTS = 0.75

# The two motors of a pair are kept together in autonomous: volts of
# correction per degree they are apart (as a percent of 12 V)
SYNC_KP = 0.5

# --- Devices ---
# Front/back/left/right are as the robot drives forward.
# The same ports and True/False as Cascade Robot Drive Limits, the program
# that has been driven on the robot. If a motor turns the wrong way, flip
# its True/False here AND in Drive Limits.
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
# Port 9 is the radio. There is no inertial sensor (gyro) yet.

# --- Claw buttons ---
# Capitals don't matter ("r1" and "R1" are the same); a name that isn't a
# button stops the program with a message on the brain saying which
# setting to fix.
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

def setting_choice(value, choices, setting):
    # A setting that must be one of a few words. Capitals don't matter; a
    # word that isn't one of them stops the program with a message on the
    # brain, instead of quietly doing the wrong thing in a match.
    key = str(value).strip().lower()
    if key not in choices:
        brain.screen.clear_screen()
        brain.screen.set_cursor(1, 1)
        brain.screen.print("%s = %s is not allowed." % (setting, value))
        brain.screen.set_cursor(2, 1)
        brain.screen.print(("Use one of: " + " ".join(sorted(choices)))[:46])
        raise ValueError("%s = %s is not allowed" % (setting, value))
    return key

alliance = setting_choice(ALLIANCE, ("red", "blue"), "ALLIANCE")

# --- Numbers worked out from the settings ---

DRIVE_INCHES_PER_DEG = (math.pi * DRIVE_WHEEL_DIAMETER_IN / 360
                        * DRIVE_MOTOR_GEAR_TEETH / DRIVE_WHEEL_GEAR_TEETH)
TRACKER_INCHES_PER_DEG = math.pi * TRACKER_WHEEL_DIAMETER_IN / 360
SYNC_KP_VOLTS = SYNC_KP * 12 / 100

AUTON_DRIVE_VOLTS = AUTON_DRIVE_SPEED * 12.0 / 100
AUTON_TURN_VOLTS = AUTON_TURN_SPEED * 12.0 / 100

# The robot turns about its middle. From there the claw holds the pin
# PIN_REACH_IN in front (6.75") and PIN_RIGHT_IN to the right (0 = middle).
ROBOT_LENGTH_IN = ROBOT_LENGTH_STUDS * STUD_IN
PIN_REACH_IN = ROBOT_LENGTH_IN / 2 - PIN_STUDS_FROM_FRONT * STUD_IN
PIN_RIGHT_IN = (ROBOT_WIDTH_STUDS / 2.0 - PIN_STUD_ACROSS) * STUD_IN

# Back against the middle of the wall, facing out: the middle of the
# robot is half its length out from the wall
START_X = 0.0
START_Y = ROBOT_LENGTH_IN / 2

# The autonomous never asks the cascade to go past the driver's limits
CASCADE_MAX_DEG = CASCADE_TRAVEL - CASCADE_MARGIN

# --- Helpers ---

def limit(value, biggest):
    return max(-biggest, min(biggest, value))

def wrap_180(angle):
    while angle > 180:
        angle -= 360
    while angle <= -180:
        angle += 360
    return angle

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

# --- PID controller ---

class PID:
    # Port of vex-pid's PIDController (include/pid.hpp): P, I, D and
    # feedforward terms, clamped integral, clamped output, settle detection.

    def __init__(self, gains):
        self.kp = gains.get("kp", 0.0)
        self.ki = gains.get("ki", 0.0)
        self.kd = gains.get("kd", 0.0)
        self.kf = gains.get("kf", 0.0)
        self.integral_limit = gains.get("integral_limit", 0.0)
        self.output_max = gains.get("output_max", 12.0)
        self.settle_error = gains.get("settle_error", 1.0)
        self.settle_ticks = gains.get("settle_ticks", 10)
        self.target = 0.0
        self.reset()

    def set_target(self, target):
        self.target = target

    def reset(self):
        self.integral = 0.0
        self.prev_error = None
        self.settle_timer = 0

    def update(self, current):
        return self.update_error(self.target - current)

    def update_error(self, error):
        self.integral += error
        if self.integral_limit > 0:
            self.integral = limit(self.integral, self.integral_limit)

        # No D on the first update, so a new target doesn't cause a jump
        derivative = 0.0
        if self.prev_error is not None:
            derivative = error - self.prev_error
        self.prev_error = error

        output = (self.kp * error + self.ki * self.integral
                  + self.kd * derivative + self.kf * self.target)
        output = limit(output, self.output_max)

        if abs(error) < self.settle_error:
            self.settle_timer += 1
        else:
            self.settle_timer = 0
        return output

    def is_settled(self):
        return self.settle_timer >= self.settle_ticks

# --- Position tracking (odometry) ---

class Odometry:
    # Keeps track of where the robot is on the field.
    #   x = inches sideways (right is +), y = inches forward (+)
    #   heading = degrees, 0 = the way the robot faced at set_pose(),
    #             clockwise is +
    # Right now it measures with the drive motor encoders, plus the
    # inertial sensor if IMU_PORT is set. When tracking wheels are added,
    # set their ports in Settings and it uses them instead - autonomous
    # routines don't need to change.

    def __init__(self):
        self.imu = None
        if IMU_PORT is not None:
            self.imu = Inertial(IMU_PORT)

        self.vertical = None
        self.vertical_offset = 0.0
        if VERTICAL_TRACKER_PORT is not None:
            self.vertical = Rotation(VERTICAL_TRACKER_PORT, VERTICAL_TRACKER_REVERSED)
            self.vertical_offset = VERTICAL_TRACKER_OFFSET_IN

        self.horizontal = None
        self.horizontal_offset = 0.0
        if HORIZONTAL_TRACKER_PORT is not None:
            self.horizontal = Rotation(HORIZONTAL_TRACKER_PORT, HORIZONTAL_TRACKER_REVERSED)
            self.horizontal_offset = HORIZONTAL_TRACKER_OFFSET_IN

        # Total inches driven forward, used by drive_distance()
        self.forward_total = 0.0
        self.set_pose(0, 0, 0)

    def calibrate(self):
        # The inertial sensor needs ~2 seconds holding still
        if self.imu is None:
            return
        self.imu.calibrate()
        while self.imu.is_calibrating():
            wait(50, MSEC)

    def left_inches(self):
        degrees = (drive_left_front_11.position(DEGREES) + drive_left_back_17.position(DEGREES)) / 2
        return degrees * DRIVE_INCHES_PER_DEG

    def right_inches(self):
        degrees = (drive_right_front_1.position(DEGREES) + drive_right_back_10.position(DEGREES)) / 2
        return degrees * DRIVE_INCHES_PER_DEG

    def raw_heading(self):
        if self.imu is not None:
            return self.imu.rotation(DEGREES)
        # No inertial sensor: the left side going further than the right
        # means the robot turned right
        return math.degrees((self.left_inches() - self.right_inches()) / TRACK_WIDTH_IN)

    def heading(self):
        return self.raw_heading() + self.heading_offset

    def forward_reading(self):
        if self.vertical is not None:
            return self.vertical.position(DEGREES) * TRACKER_INCHES_PER_DEG
        return (self.left_inches() + self.right_inches()) / 2

    def sideways_reading(self):
        if self.horizontal is not None:
            return self.horizontal.position(DEGREES) * TRACKER_INCHES_PER_DEG
        return 0.0

    def set_pose(self, x, y, heading):
        # Tell the robot where it is, e.g. its starting spot on the field
        self.x = x
        self.y = y
        self.heading_offset = heading - self.raw_heading()
        self.prev_forward = self.forward_reading()
        self.prev_sideways = self.sideways_reading()
        self.prev_heading = self.heading()

    def update(self):
        forward = self.forward_reading()
        sideways = self.sideways_reading()
        heading = self.heading()
        turned = math.radians(heading - self.prev_heading)

        # Turning in place rolls an off-center tracking wheel even though
        # the robot didn't go anywhere, so take that part out
        moved_forward = forward - self.prev_forward + self.vertical_offset * turned
        moved_sideways = sideways - self.prev_sideways - self.horizontal_offset * turned

        # Point the move along the average heading during this step
        angle = math.radians(self.prev_heading) + turned / 2
        self.x += moved_forward * math.sin(angle) + moved_sideways * math.cos(angle)
        self.y += moved_forward * math.cos(angle) - moved_sideways * math.sin(angle)
        self.forward_total += moved_forward

        self.prev_forward = forward
        self.prev_sideways = sideways
        self.prev_heading = heading

# --- Cascade and toggle in autonomous ---

class SyncedMechanism:
    # Two motors that must move together, held at a target angle by a PID.
    # Adapted from vex-pid's Cascade and Toggle subsystems, plus a
    # left/right sync correction.

    def __init__(self, motor_a, motor_b, gains, feedforward=0.0, min_deg=None, max_deg=None):
        self.motor_a = motor_a
        self.motor_b = motor_b
        self.pid = PID(gains)
        self.feedforward = feedforward
        self.min_deg = min_deg
        self.max_deg = max_deg
        self.active = False

    def position(self):
        return (self.motor_a.position(DEGREES) + self.motor_b.position(DEGREES)) / 2

    def move_to(self, degrees, wait_until_done=False, timeout_ms=2000):
        if self.min_deg is not None:
            degrees = max(self.min_deg, degrees)
        if self.max_deg is not None:
            degrees = min(self.max_deg, degrees)
        self.pid.reset()
        self.pid.set_target(degrees)
        self.active = True
        if wait_until_done:
            return self.wait_until_settled(timeout_ms)
        return False

    def hold_here(self):
        self.move_to(self.position())

    def is_settled(self):
        return self.pid.is_settled()

    def wait_until_settled(self, timeout_ms=2000):
        start = brain.timer.system()
        while auton_active and not self.pid.is_settled():
            if brain.timer.system() - start > timeout_ms:
                return False
            wait(10, MSEC)
        return self.pid.is_settled()

    def update(self):
        # Runs every 10 ms from background_tick() during autonomous
        if not self.active:
            return
        power = self.pid.update(self.position())
        # Feedforward only when lifted, so it doesn't push off the bottom
        if self.pid.target > 20:
            power += self.feedforward
        error = self.motor_a.position(DEGREES) - self.motor_b.position(DEGREES)
        correction = error * SYNC_KP_VOLTS
        self.motor_a.spin(FORWARD, limit(power - correction, 12), VoltageUnits.VOLT)
        self.motor_b.spin(FORWARD, limit(power + correction, 12), VoltageUnits.VOLT)

    def release(self):
        # Hand control back to the driver, holding where it is
        self.active = False
        self.motor_a.stop()
        self.motor_b.stop()

# --- Autonomous moves ---

def set_drive_volts(left, right):
    left_drive.spin(FORWARD, left, VoltageUnits.VOLT)
    right_drive.spin(FORWARD, right, VoltageUnits.VOLT)

def stop_drive():
    left_drive.stop()
    right_drive.stop()

def heading_to(x, y):
    return math.degrees(math.atan2(x - odom.x, y - odom.y))

def drive_distance(inches, max_volts=AUTON_DRIVE_VOLTS, timeout_ms=None, heading=None):
    # Drive straight (negative inches = backward) while holding heading.
    # Gives up after timeout_ms (default: 1 s + 0.15 s per inch at full
    # power, longer when driving slower).
    # Adapted from vex-pid DriveBase::drive_straight.
    if heading is None:
        heading = odom.heading()
    if timeout_ms is None:
        timeout_ms = 1000 + abs(inches) * 150 * 12.0 / max(max_volts, 1.0)
    drive_pid.reset()
    drive_pid.set_target(inches)
    heading_pid.reset()
    start_forward = odom.forward_total
    start_time = brain.timer.system()

    while auton_active and not drive_pid.is_settled():
        if brain.timer.system() - start_time > timeout_ms:
            break
        traveled = odom.forward_total - start_forward
        power = limit(drive_pid.update(traveled), max_volts)
        correction = heading_pid.update_error(wrap_180(heading - odom.heading()))
        left = power + correction
        right = power - correction
        # Scale both sides down together so neither goes past max_volts
        biggest = max(abs(left), abs(right))
        if biggest > max_volts:
            left = left * max_volts / biggest
            right = right * max_volts / biggest
        set_drive_volts(left, right)
        wait(10, MSEC)

    stop_drive()
    return drive_pid.is_settled()

def turn_to_heading(target, max_volts=AUTON_TURN_VOLTS, timeout_ms=None):
    # Turn in place the short way around to face target degrees.
    # Gives up after timeout_ms (default: 1 s + 15 ms per degree at full
    # power, longer when turning slower).
    # Adapted from vex-pid DriveBase::turn_to_heading.
    if timeout_ms is None:
        timeout_ms = 1000 + abs(wrap_180(target - odom.heading())) * 15 * 12.0 / max(max_volts, 1.0)
    turn_pid.reset()
    start_time = brain.timer.system()

    while auton_active and not turn_pid.is_settled():
        if brain.timer.system() - start_time > timeout_ms:
            break
        power = limit(turn_pid.update_error(wrap_180(target - odom.heading())), max_volts)
        set_drive_volts(power, -power)
        wait(10, MSEC)

    stop_drive()
    return turn_pid.is_settled()

def turn_to_point(x, y, max_volts=AUTON_TURN_VOLTS, timeout_ms=None):
    return turn_to_heading(heading_to(x, y), max_volts, timeout_ms)

def drive_to_point(x, y, max_volts=AUTON_DRIVE_VOLTS, timeout_ms=None):
    # Uses odometry: works now with the motor encoders, and gets more
    # accurate once tracking wheels are added. A slower max_volts slows
    # the turn too.
    turn_to_point(x, y, min(max_volts, AUTON_TURN_VOLTS))
    dx = x - odom.x
    dy = y - odom.y
    distance = math.sqrt(dx * dx + dy * dy)
    return drive_distance(distance, max_volts, timeout_ms, heading_to(x, y))

def claw_target(x, y):
    # The heading to face, and how far to drive, so that the claw (not the
    # middle of the robot) ends up right over the spot (x, y)
    dx = x - odom.x
    dy = y - odom.y
    distance = math.sqrt(dx * dx + dy * dy)
    ahead = math.sqrt(max(0.0, distance * distance - PIN_RIGHT_IN * PIN_RIGHT_IN))
    heading = math.degrees(math.atan2(dx, dy)) - math.degrees(math.atan2(PIN_RIGHT_IN, ahead))
    return heading, ahead - PIN_REACH_IN

def reach_over(x, y):
    # Turn toward (x, y), wait for the cascade to reach its height, then
    # drive until the claw is over (x, y). Returns how far it drove, so
    # the robot can back out along the same line.
    heading, distance = claw_target(x, y)
    turn_to_heading(heading)
    cascade_pid.wait_until_settled(MECHANISM_TIMEOUT_MS)
    # Turning in place moves the middle of the robot a little: aim again
    heading, distance = claw_target(x, y)
    drive_distance(distance, heading=heading)
    return distance

def claw_turn(direction):
    # One claw turn of CLAW_TURN_DEGREES, the same as one button press.
    # Waits until the turn is done, or the claw has stopped against a pin
    # or the end of its travel, or CLAW_MOVE_MS has passed. A claw that
    # closes on a pin stops short of the full turn and keeps squeezing,
    # which is what holds the pin.
    if not auton_active:
        return True      # the autonomous was stopped: don't move the claw
    claw_16.spin_for(direction, CLAW_TURN_DEGREES, DEGREES, wait=False)
    start = brain.timer.system()
    while auton_active and not claw_16.is_done():
        elapsed = brain.timer.system() - start
        if elapsed > CLAW_MOVE_MS:
            return False
        if elapsed > CLAW_STALL_CHECK_MS and abs(claw_16.velocity(PERCENT)) < 2:
            return False
        wait(10, MSEC)
    return True

def claw_open():
    # Once the claw is open (or has stopped at the end of its travel) it
    # holds there, instead of pushing against its end for the rest of the
    # match and warming the motor
    done = claw_turn(CLAW_OPEN_DIRECTION)
    if not done:
        claw_16.stop()
    return done

def claw_close():
    return claw_turn(CLAW_CLOSE_DIRECTION)

# --- Position tracking and limits for driver control ---

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

# --- Background loop (tracking, mechanism PID, autonomous screen) ---

auton_active = False
background_ticks = 0
auton_step = ""
auton_start_ms = 0
# One line about the last autonomous, shown on the driver screen after it
auton_report = ""
# True while a test started by holding A is running, and whether B (or
# AUTON_TEST_SECONDS) stopped it
auton_test = False
auton_stopped = False

def show_status():
    brain.screen.set_cursor(1, 1)
    brain.screen.clear_row()
    brain.screen.print("X %.1f  Y %.1f  H %.1f" % (odom.x, odom.y, odom.heading()))
    brain.screen.set_cursor(2, 1)
    brain.screen.clear_row()
    brain.screen.print("Cascade %d  Toggle %d" % (cascade_pid.position(), toggle_pid.position()))
    brain.screen.set_cursor(3, 1)
    brain.screen.clear_row()
    seconds = (brain.timer.system() - auton_start_ms) / 1000.0
    brain.screen.print(("%.1f s  %s" % (seconds, auton_step))[:46])

def stop_auton_test():
    # B pressed (or the test time is up): end the autonomous now. Every move
    # in the routine checks auton_active and ends, and the motors stop here
    # so nothing carries on with its last command.
    global auton_active, auton_stopped
    auton_active = False
    auton_stopped = True
    left_drive.stop()
    right_drive.stop()
    cascade.stop()
    toggle.stop()

def background_tick():
    global background_ticks
    odom.update()
    if auton_active and auton_test:
        out_of_time = (AUTON_TEST_SECONDS > 0 and
                       brain.timer.system() - auton_start_ms > AUTON_TEST_SECONDS * 1000)
        if controller_1.buttonB.pressing() or out_of_time:
            stop_auton_test()
    if auton_active:
        cascade_pid.update()
        toggle_pid.update()
        background_ticks += 1
        if background_ticks % 25 == 0:
            show_status()

def background_loop():
    while True:
        background_tick()
        wait(10, MSEC)

def step(text):
    # Name the part of the routine that is running, for the brain screen.
    # If the 15 seconds run out, the driver screen says where it stopped.
    global auton_step, auton_report
    if auton_stopped:
        return           # keep the step it was stopped in
    auton_step = text
    auton_report = "Auton cut off at: " + text

# --- Autonomous routines ---

def auton_override():
    # Flip our Toggle, set the preload on our goal, then stack the yellow
    # pin-in-cup on top of it. Field positions are in Settings (X right
    # from the middle of our wall, Y out into the field, inches).
    flip = TOGGLE_FLIP_DEGREES
    if alliance == "blue":
        flip = TOGGLE_FLIP_DEGREES_BLUE
    # Back against the middle of our wall, facing out into the field
    odom.set_pose(START_X, START_Y, 0)

    step("1 flip the toggle")
    # Squeeze the preload first (the claw was closed on it by hand and the
    # motor was not holding it), so it can't slip out when the cascade
    # lifts. The claw stops against the pin and keeps squeezing.
    if auton_active:
        claw_16.spin_for(CLAW_CLOSE_DIRECTION, CLAW_TURN_DEGREES, DEGREES, wait=False)
    toggle_pid.move_to(flip, wait_until_done=True, timeout_ms=MECHANISM_TIMEOUT_MS)

    step("2 out, level with the goal")
    cascade_pid.move_to(CASCADE_CARRY_DEG)          # lift the preload while driving
    drive_distance(GOAL_Y - START_Y)                # straight out (15.25")
    toggle_pid.move_to(0)                           # fold the toggle arm back

    step("3 preload onto the goal")
    reach_over(GOAL_X, GOAL_Y)                      # turn right, pin over the goal
    cascade_pid.move_to(CASCADE_SET_DEG, wait_until_done=True,
                        timeout_ms=MECHANISM_TIMEOUT_MS)
    claw_open()

    step("4 back away from the goal")
    drive_distance(-BACK_OFF_IN)
    cascade_pid.move_to(CASCADE_PICKUP_DEG)         # lower while turning

    step("5 grab the yellow pin")
    stack_inches = reach_over(STACK_X, STACK_Y)
    claw_close()
    cascade_pid.move_to(CASCADE_STACK_DEG)          # lift while backing out

    step("6 back out the same way")
    drive_distance(-stack_inches)

    step("7 stack onto the goal")
    reach_over(GOAL_X, GOAL_Y)
    cascade_pid.move_to(CASCADE_STACK_SET_DEG, wait_until_done=True,
                        timeout_ms=MECHANISM_TIMEOUT_MS)
    claw_open()

    step("8 back off")
    drive_distance(-FINAL_BACK_OFF_IN)

def auton_pid_test():
    # Use this to tune the PID and check the distances. It should end
    # where it started, facing the same way, and the 24" should be 24".
    drive_distance(24)
    turn_to_heading(90)
    turn_to_heading(0)
    drive_distance(-24)

def auton_odom_test():
    # Drives a 24 inch square and comes back to the start. The brain
    # screen should read close to X 0, Y 0, H 0 at the end. Run this
    # again after adding tracking wheels to check them.
    drive_to_point(0, 24)
    drive_to_point(24, 24)
    drive_to_point(24, 0)
    drive_to_point(0, 0)
    turn_to_heading(0)

def auton_none():
    pass

ROUTINES = {
    "override": auton_override,
    "pid_test": auton_pid_test,
    "odom_test": auton_odom_test,
    "none": auton_none,
}

# A misspelt routine name stops the program here, at startup, rather than
# silently doing nothing in a match
auton_routine = setting_choice(AUTON_ROUTINE, ROUTINES, "AUTON_ROUTINE")

# --- Startup ---

# BRAKE on the drive, HOLD on everything that holds something up or is
# a pair: let go and it stays put instead of falling or drifting
for motor in (drive_left_front_11, drive_left_back_17,
              drive_right_front_1, drive_right_back_10):
    motor.set_stopping(BRAKE)
for motor in (cascade_left_13, cascade_right_2,
              toggle_18, toggle_8, claw_16):
    motor.set_stopping(HOLD)

# Every count starts from 0 here, so start with the cascade all the way
# DOWN, the claw CLOSED on the preload and the toggle arm at rest
for part in all_parts:
    part.motor.set_position(0, DEGREES)

# Full strength for every motor. A motor can keep a torque limit from the
# last program that ran (an older program on the brain may have set one),
# so it is set back to 100% here.
for part in all_parts:
    part.motor.set_max_torque(100, PERCENT)

# The claw makes each turn at CLAW_SPEED
claw_16.set_velocity(CLAW_SPEED, PERCENT)

brain.screen.set_font(FontType.MONO15)
brain.screen.clear_screen()
brain.screen.print("Calibrating - don't move the robot")
odom = Odometry()
odom.calibrate()
odom.set_pose(0, 0, 0)

drive_pid = PID(DRIVE_GAINS)
heading_pid = PID(HEADING_GAINS)
turn_pid = PID(TURN_GAINS)
cascade_pid = SyncedMechanism(cascade_left_13, cascade_right_2, CASCADE_GAINS,
                              CASCADE_FEEDFORWARD_VOLTS, 0, CASCADE_MAX_DEG)
toggle_pid = SyncedMechanism(toggle_18, toggle_8, TOGGLE_GAINS)

brain.screen.clear_screen()
brain.screen.set_cursor(1, 1)
brain.screen.print("OVERRIDE AUTON - auton %s, %s" % (auton_routine, alliance))
brain.screen.set_cursor(2, 1)
brain.screen.print("Back on the wall, centered on the toggle.")
brain.screen.set_cursor(3, 1)
brain.screen.print("Cascade DOWN, claw CLOSED on the preload.")
brain.screen.set_cursor(5, 1)
brain.screen.print("Run = driver control. To test the auton:")
brain.screen.set_cursor(6, 1)
brain.screen.print("hold A for 1 s (B stops it), or Timed Run.")

controller_1.screen.clear_screen()
controller_1.rumble(".")

Thread(background_loop)

# --- Autonomous ---

def autonomous():
    global auton_active, auton_start_ms, auton_report, auton_stopped
    auton_stopped = False
    auton_active = True
    auton_start_ms = brain.timer.system()
    brain.screen.clear_screen()
    # Start of the routine is (0, 0) facing 0. A routine can call
    # odom.set_pose(x, y, heading) first to use real field coordinates.
    odom.set_pose(0, 0, 0)
    cascade_pid.hold_here()
    toggle_pid.hold_here()
    ROUTINES[auton_routine]()
    stop_drive()
    seconds = (brain.timer.system() - auton_start_ms) / 1000.0
    if auton_stopped:
        auton_report = "Auton stopped at %.1f s: %s" % (seconds, auton_step)
    else:
        auton_report = "Auton %s done in %.1f s" % (auton_routine, seconds)
    brain.screen.set_cursor(4, 1)
    brain.screen.clear_row()
    brain.screen.print(auton_report)

# --- Testing the autonomous from driver control ---

def off_field():
    # True when there is no field control and no competition switch, i.e.
    # the program was started with plain Run. If the brain can't tell,
    # it counts as ON a field, so A can never start the autonomous in a
    # real match.
    try:
        return not (competition.is_field_control() or competition.is_competition_switch())
    except Exception:
        return False

def run_auton_test():
    # Hold A off the field: run the autonomous right here, then give the
    # driver control back
    global auton_test, auton_active
    left_drive.stop()
    right_drive.stop()
    cascade.stop()
    toggle.stop()
    controller_1.rumble("-")
    show_controller_line(3, "AUTON TEST B=stop")
    auton_test = True
    autonomous()
    auton_test = False
    auton_active = False
    cascade_pid.release()
    toggle_pid.release()
    stop_drive()
    controller_1.rumble(".")

# --- Driver control ---

def driver_control():
    global auton_active
    # Stop any autonomous PID so it doesn't fight the driver
    auton_active = False
    cascade_pid.release()
    toggle_pid.release()

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
    a_down_ms = None         # when A was pressed, for the autonomous test
    a_let_go = True          # A must be let go before it can start another test

    while True:
        # How long the last pass took, measured rather than assumed
        now_ms = brain.timer.time(MSEC)
        pass_ms = LOOP_MS
        if last_ms is not None:
            pass_ms = max(LOOP_MS, now_ms - last_ms)
        last_ms = now_ms
        pass_s = pass_ms / 1000.0
        slowest_pass_ms = max(slowest_pass_ms, pass_ms)

        # --- Hold A (off the field only) to test the autonomous ---
        if not controller_1.buttonA.pressing():
            a_down_ms = None
            a_let_go = True
        elif a_let_go and off_field():
            if a_down_ms is None:
                a_down_ms = now_ms
            elif now_ms - a_down_ms >= AUTON_TEST_HOLD_MS:
                run_auton_test()
                # Start the driver loop afresh: nothing is moving, the
                # screens are redrawn, and the test's time isn't a "pass"
                a_down_ms = None
                a_let_go = False
                driving = False
                cascade_moving = False
                toggle_moving = False
                last_ms = None
                last_brain_ms = None
                last_controller_ms = None
                shown_controller = [None, None, None]
                wait(LOOP_MS, MSEC)
                continue

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
            if auton_report:
                lines.append(auton_report[:46])
            elif off_field():
                lines.append("Hold A 1 s to test the auton (B stops it)")
            else:
                lines.append("Start: cascade DOWN, claw CLOSED on preload.")
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

competition = Competition(driver_control, autonomous)
