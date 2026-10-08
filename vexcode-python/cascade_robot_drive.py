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
# 	Project:      Cascade Robot Drive
#	Description:  The simple driver-control program. Just the controller
#	              and the motors - no PID, no odometry, nothing to
#	              calibrate. Everything you need to drive the robot.
#
#	Left stick up/down ..... drive forward / backward
#	Right stick left/right . turn left / right
#	  (that pair is called "split arcade drive")
#
#	L1 ......... cascade up          L2 ......... cascade down
#	R1 ......... claw close          R2 ......... claw open
#	Up ......... toggle one way      Down ....... toggle the other way
#
#	Let go of a button and that part holds where it is, so the cascade
#	does not fall and the claw keeps its grip.
#
#	The other two programs are:
#	  cascade_robot_test.py   motor test - checks one motor at a time
#	  cascade_robot_auton.py  match program - driver control + PID auton
#
#	This one is the plain drive program.
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
#
# EVERYTHING IS 10 FOR THE FIRST DRIVE. That is crawling pace on purpose:
# the job of this first run is to check that each part moves the right way
# and that nothing crashes, not to drive properly.
#
# If a part will not move at all at 10, that is normal - 10% is not much
# torque - and the fix is to raise just that one number. Raise them about
# 10 at a time as you get comfortable. DRIVE_SPEED and TURN_SPEED are the
# two that matter most for driving.
DRIVE_SPEED = 10
TURN_SPEED = 10
CASCADE_SPEED = 10
CLAW_SPEED = 10
TOGGLE_SPEED = 10

# Ignore tiny stick movements so the robot does not creep when you let go
DEADBAND = 5

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

# --- Driver control ---

def driver():
    cascade_moving = False
    claw_moving = False
    toggle_moving = False
    shown = None
    loops = 0

    while True:
        # --- Split arcade drive ---
        forward = apply_deadband(controller_1.axis3.position()) * DRIVE_SPEED / 100.0
        turn = apply_deadband(controller_1.axis1.position()) * TURN_SPEED / 100.0
        left, right = arcade(forward, turn)
        left_drive.spin(FORWARD, left, PERCENT)
        right_drive.spin(FORWARD, right, PERCENT)

        # --- Cascade: L1 up, L2 down ---
        cascade_speed = 0
        if controller_1.buttonL1.pressing():
            cascade_speed = CASCADE_SPEED
        elif controller_1.buttonL2.pressing():
            cascade_speed = -CASCADE_SPEED
        cascade_moving = run_group(cascade, cascade_speed, cascade_moving)

        # --- Claw: R1 close, R2 open ---
        claw_speed = 0
        if controller_1.buttonR1.pressing():
            claw_speed = CLAW_SPEED
        elif controller_1.buttonR2.pressing():
            claw_speed = -CLAW_SPEED
        claw_moving = run_group(claw, claw_speed, claw_moving)

        # --- Toggle: Up one way, Down the other way ---
        toggle_speed = 0
        if controller_1.buttonUp.pressing():
            toggle_speed = TOGGLE_SPEED
        elif controller_1.buttonDown.pressing():
            toggle_speed = -TOGGLE_SPEED
        toggle_moving = run_group(toggle, toggle_speed, toggle_moving)

        # --- Words for the screens ---
        if cascade_speed > 0:
            cascade_text = "UP"
        elif cascade_speed < 0:
            cascade_text = "DOWN"
        else:
            cascade_text = "hold"

        if claw_speed > 0:
            claw_text = "CLOSE"
        elif claw_speed < 0:
            claw_text = "OPEN"
        else:
            claw_text = "hold"

        if toggle_speed > 0:
            toggle_text = "one way"
        elif toggle_speed < 0:
            toggle_text = "other way"
        else:
            toggle_text = "hold"

        brain_lines = [
            "CASCADE ROBOT - DRIVE  (no PID)",
            "L stick fwd/back   R stick turn",
            "",
            "Drive  L " + pad("%+4d" % int(left), 7) + "R " + pad("%+4d" % int(right), 7) + "%",
            "Cascade " + pad(cascade_text, 8) + "Claw " + claw_text,
            "Toggle  " + toggle_text,
            "",
            "L1/L2 cascade    R1/R2 claw",
            "Up/Down toggle   let go = hold",
        ]

        controller_lines = [
            "Drive " + "%+d" % int(left) + " / " + "%+d" % int(right),
            "Cascade " + cascade_text,
            "Claw " + claw_text + "  Tog " + toggle_text,
        ]

        # Redraw when something changed, and refresh the brain now and then
        # so the battery and cable readouts stay honest.
        if brain_lines != shown or loops % 25 == 0:
            shown = brain_lines
            show_brain(brain_lines)
            show_controller(controller_lines)

        loops += 1
        wait(20, MSEC)

# --- Startup ---

# Every motor on this robot is on a pair or holds something up, so HOLD is
# the safe choice: let go and it stays put instead of falling or drifting.
for motor in (drive_left_front_11, drive_left_back_17,
              drive_right_front_1, drive_right_back_10):
    motor.set_stopping(BRAKE)
for motor in (cascade_left_13, cascade_right_2,
              toggle_18, toggle_8, claw_16):
    motor.set_stopping(HOLD)

brain.screen.set_font(FontType.MONO15)
brain.screen.clear_screen()
brain.screen.print("CASCADE ROBOT - DRIVE")
brain.screen.set_cursor(2, 1)
brain.screen.print("Starting...")

controller_1.rumble(".")

driver()
