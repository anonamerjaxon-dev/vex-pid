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
# 	Project:      Cascade Robot Motor Test
#	Description:  Spins ONE motor at a time, to check every port and
#	              direction before using the match program. It uses the
#	              same ports and True/False settings as the match program,
#	              so each motor turns exactly the way it will in a match.
#
#	Put the robot on a stand. Hold a button to spin its motor:
#	  Up ......... drive left front  (11)  wheel rolls forward
#	  Down ....... drive left back   (17)  wheel rolls forward
#	  X .......... drive right front (1)   wheel rolls forward
#	  B .......... drive right back  (10)  wheel rolls forward
#	  L1 ......... cascade left      (13)  lift goes up
#	  R1 ......... cascade right     (2)   lift goes up
#	  L2 ......... toggle            (18)  spins the same way as 8
#	  R2 ......... toggle            (8)   spins the same way as 18
#	  A .......... claw              (16)  claw closes
#	  Right ...... flip direction MATCH <-> REVERSE (to move things back)
#
#	If a motor goes the wrong way, flip its True/False in the Devices
#	block below AND in the match program.
#
# ------------------------------------------

# Library imports
from vex import *

# --- Settings ---

# Test speed in percent. Slower than the match on purpose; the direction
# is exactly the same as the match program.
TEST_SPEED = 40

# Gear cartridges - same as the match program
#   GREEN = GearSetting.RATIO_18_1 (200 RPM)
#   BLUE  = GearSetting.RATIO_6_1  (600 RPM)
#   RED   = GearSetting.RATIO_36_1 (100 RPM)
DRIVE_GEARS = GearSetting.RATIO_18_1
CASCADE_GEARS = GearSetting.RATIO_36_1
CLAW_GEARS = GearSetting.RATIO_18_1

# --- Devices ---
# Front/back/left/right are as the robot drives forward.
# This block is the same in every version of the program (match, test and
# driver-only). If the test program shows a motor turning the wrong way,
# flip its True/False here AND in the other versions.

controller_1 = Controller(PRIMARY)

# Drivetrain: 4 x 11W
drive_left_front_11 = Motor(Ports.PORT11, DRIVE_GEARS, True)
drive_left_back_17 = Motor(Ports.PORT17, DRIVE_GEARS, True)
drive_right_front_1 = Motor(Ports.PORT1, DRIVE_GEARS, False)
drive_right_back_10 = Motor(Ports.PORT10, DRIVE_GEARS, False)
left_drive = MotorGroup(drive_left_front_11, drive_left_back_17)
right_drive = MotorGroup(drive_right_front_1, drive_right_back_10)

# Cascade: 2 x 11W, kept in sync
cascade_left_13 = Motor(Ports.PORT13, CASCADE_GEARS, False)
cascade_right_2 = Motor(Ports.PORT2, CASCADE_GEARS, True)

# Toggle: 2 x 5.5W, kept in sync (5.5W motors are always 200 RPM)
toggle_18 = Motor(Ports.PORT18, GearSetting.RATIO_18_1, False)
toggle_8 = Motor(Ports.PORT8, GearSetting.RATIO_18_1, True)

# Claw: on the long 1500mm cable
claw_16 = Motor(Ports.PORT16, CLAW_GEARS, False)

# Port 9: a single device nobody has identified yet. Not used.

# --- Test buttons ---
# (button name, button, motor, name on screen, what it should do)
# Holding a button spins its motor FORWARD, the same as the match program
# does for drive forward, cascade up (L1), claw close (R1) and toggle (Down).

TESTS = [
    ("Up", controller_1.buttonUp, drive_left_front_11, "Drive L front 11", "roll FORWARD"),
    ("Down", controller_1.buttonDown, drive_left_back_17, "Drive L back 17", "roll FORWARD"),
    ("X", controller_1.buttonX, drive_right_front_1, "Drive R front 1", "roll FORWARD"),
    ("B", controller_1.buttonB, drive_right_back_10, "Drive R back 10", "roll FORWARD"),
    ("L1", controller_1.buttonL1, cascade_left_13, "Cascade L 13", "go UP"),
    ("R1", controller_1.buttonR1, cascade_right_2, "Cascade R 2", "go UP"),
    ("L2", controller_1.buttonL2, toggle_18, "Toggle 18", "same way as 8"),
    ("R2", controller_1.buttonR2, toggle_8, "Toggle 8", "same way as 18"),
    ("A", controller_1.buttonA, claw_16, "Claw 16", "CLOSE"),
]

# --- Helpers ---

def pad(text, width):
    return text + " " * (width - len(text))

def connected(motor):
    # True if the brain can see a motor on that port
    try:
        return motor.installed()
    except Exception:
        return None

def status_text(index, running):
    if index == running:
        return "RUN"
    found = connected(TESTS[index][2])
    if found is None:
        return "?"
    if found:
        return "ok"
    return "MISSING"

def draw_brain(running, match_direction, message):
    brain.screen.set_cursor(1, 1)
    brain.screen.clear_row()
    brain.screen.print("MOTOR TEST - hold one button")
    brain.screen.set_cursor(2, 1)
    brain.screen.clear_row()
    if match_direction:
        brain.screen.print("Direction: MATCH   (Right arrow flips)")
    else:
        brain.screen.print("Direction: REVERSE (Right arrow flips)")
    brain.screen.set_cursor(3, 1)
    brain.screen.clear_row()
    brain.screen.print(pad("Btn", 6) + pad("Motor", 17) + pad("Should", 15) + "Status")
    for i in range(len(TESTS)):
        name, button, motor, label, should = TESTS[i]
        brain.screen.set_cursor(4 + i, 1)
        brain.screen.clear_row()
        brain.screen.print(pad(name, 6) + pad(label, 17) + pad(should, 15) + status_text(i, running))
    brain.screen.set_cursor(13, 1)
    brain.screen.clear_row()
    brain.screen.print(message)

def draw_controller(line1, line2, line3):
    controller_1.screen.clear_screen()
    controller_1.screen.set_cursor(1, 1)
    controller_1.screen.print(line1)
    controller_1.screen.set_cursor(2, 1)
    controller_1.screen.print(line2)
    controller_1.screen.set_cursor(3, 1)
    controller_1.screen.print(line3)

# --- Motor test ---

def motor_test():
    match_direction = True
    right_was_pressed = False
    running = None
    shown = None
    loops = 0

    while True:
        # Right arrow flips the direction, once per press
        right_pressed = controller_1.buttonRight.pressing()
        if right_pressed and not right_was_pressed:
            match_direction = not match_direction
            controller_1.rumble(".")
        right_was_pressed = right_pressed

        pressed = []
        for i in range(len(TESTS)):
            if TESTS[i][1].pressing():
                pressed.append(i)

        # Never more than one motor: two buttons at once runs nothing
        wanted = None
        if len(pressed) == 1:
            wanted = pressed[0]
        if wanted != running and running is not None:
            TESTS[running][2].stop()
        running = wanted

        if running is not None:
            speed = TEST_SPEED
            if not match_direction:
                speed = -TEST_SPEED
            TESTS[running][2].spin(FORWARD, speed, PERCENT)

        # Screens: redraw when something changes, and the brain every
        # second so a cable plugged in or pulled out shows up
        if match_direction:
            direction = "MATCH direction"
        else:
            direction = "REVERSE direction"
        if len(pressed) > 1:
            line2 = "One button only!"
            line3 = ""
        elif running is not None:
            line2 = TESTS[running][3]
            if match_direction:
                line3 = "-> " + TESTS[running][4]
            else:
                line3 = "-> OPPOSITE way"
        else:
            line2 = "Hold a button"
            line3 = "Right = flip dir"
        if (direction, line2, line3, running) != shown:
            shown = (direction, line2, line3, running)
            draw_controller(direction, line2, line3)
            draw_brain(running, match_direction, line2 + "  " + line3)
        elif loops % 50 == 0:
            draw_brain(running, match_direction, line2 + "  " + line3)

        loops += 1
        wait(20, MSEC)

# --- Startup ---

# Every motor coasts in the test, so the other motor of a pair turns
# freely instead of fighting the one being tested
for test in TESTS:
    test[2].set_stopping(COAST)

brain.screen.set_font(FontType.MONO15)
motor_test()
