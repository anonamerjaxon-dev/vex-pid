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
#  Project:      Cascade Robot Amps
#  Description:  A measuring tool, not a driving program.
#
#	The current thresholds in drive v2 (and in the autonomous programs) were
#	picked by hand. Nobody has ever seen what this robot really draws. This
#	program runs one mechanism at a time and puts the honest number on the
#	screen, so the thresholds can be set from a measurement instead of a
#	guess.
#
#	Hold one of these buttons down for the whole test:
#	  A ... cascade, lifting up        B ... claw, closing
#	  X ... toggle, turning            Y ... drive, forwards
#
#	Let go of the button and the motor stops immediately.
#
#	While the test runs, load the mechanism BY HAND:
#	  * press down lightly on the arm as it lifts,
#	  * let the claw close on nothing, then put a game element in it,
#	  * hold the toggle back,
#	  * for drive, put the robot on blocks or hold it back.
#	The "now" reading climbs as you do. Read the PEAK afterwards - that is
#	the number to write down.
#
#	Two warnings, both deliberate:
#	  * this tool runs the motors with NO current ceiling, because a ceiling
#	    would hide the very number you are trying to read;
#	  * a test runs for a few seconds, so never wedge a mechanism so hard
#	    that the motor cannot turn at all and then walk away. Let go of the
#	    button and it stops.
#
#	Nothing in here drives the robot. It is for the bench, with the wheels
#	off the floor.
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

# ---- How hard the test pushes, in percent ---------------------------------
# Kept the same as the ceilings in the drive programs, so the numbers this
# tool reports are the numbers the driving program will really see. Change
# one of these and the measurement stops predicting the driving program.
CASCADE_SPEED = 40
CLAW_SPEED = 40
TOGGLE_SPEED = 40
DRIVE_SPEED = 40

LOOP_MS = 20              # how often the current is read
TEST_SECONDS = 5.0        # how long one test runs
STEADY_SAMPLES = 50       # the "steady" figure is the last second of them

# ---- Current ceiling -----------------------------------------------------
# OFF on purpose. A ceiling is a clamp: once the motor hits it, the reading
# stops climbing and you can no longer tell "working hard" from "about to
# stall". V5 motors have their own internal protection, and a test only lasts
# a few seconds - but that is the whole reason the tests are short.
USE_TORQUE_LIMITS = False
TORQUE_FULL_AMPS = 2.5

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
#  THE TESTS
# ==========================================================================
# FORWARD is "the way the drive program does it": the cascade lifts, the claw
# closes, the toggle turns. The encoder check in drive v2 measures a motor's
# velocity as a percentage of the command, so the direction here does not
# matter to it - only the size of the current does.

TESTS = [
    dict(key='cascade', line='cascade', button='A', groups=[cascade],
         direction=FORWARD, speed=CASCADE_SPEED,
         title='CASCADE  lifting up',
         hint1='Press down on the arm',
         hint2='lightly as it lifts.'),
    dict(key='claw', line='claw', button='B', groups=[claw],
         direction=FORWARD, speed=CLAW_SPEED,
         title='CLAW  closing',
         hint1='Let it close on nothing,',
         hint2='then put something in it.'),
    dict(key='toggle', line='toggle', button='X', groups=[toggle],
         direction=FORWARD, speed=TOGGLE_SPEED,
         title='TOGGLE  turning',
         hint1='Hold the toggle back',
         hint2='with your hand.'),
    dict(key='drive', line='drive', button='Y', groups=[left_drive, right_drive],
         direction=FORWARD, speed=DRIVE_SPEED,
         title='DRIVE  forwards',
         hint1='Wheels off the floor,',
         hint2='or hold the robot back.'),
]

# ==========================================================================
#  HELPERS
# ==========================================================================

def pad(text, width):
    if len(text) >= width:
        return text
    return text + " " * (width - len(text))

def motor_count(test):
    # How many motors are sharing this reading. The cascade has two, the claw
    # has one, the drive has four.
    #
    # THIS MATTERS: the figure on the screen is everything the test drives,
    # added up (MotorGroup.current() sums its motors). So for the cascade it
    # is a PAIR total, which is what CASCADE_STRAIN_AMPS is - and for one
    # motor it is that one motor, which is what CLAW_HOLD_AMPS is. Some of the
    # thresholds in drive v2 are per motor, so the screen shows the
    # per-motor figure too: the number in brackets is the total divided by
    # the number of motors.
    total = 0
    for group in test['groups']:
        total += group.count()
    return total

def each(test, value):
    n = motor_count(test)
    if n <= 1:
        return ""
    return "  (%4.2f each)" % (value / n)

class Reading:
    # A rolling picture of what one mechanism is drawing.
    #
    # `peak` is the biggest reading of the whole test - that is the number
    # worth writing down, because a threshold has to be above the worst the
    # mechanism does on purpose. `steady` is the average of the last second,
    # which is what it settles at once the motor has spun up.
    def __init__(self, window=STEADY_SAMPLES):
        self.window = window
        self.peak = 0.0
        self.now = 0.0
        self.recent = []

    def reset(self):
        self.peak = 0.0
        self.now = 0.0
        self.recent = []

    def watch(self, amps):
        self.now = amps
        if amps > self.peak:
            self.peak = amps
        self.recent.append(amps)
        if len(self.recent) > self.window:
            self.recent.pop(0)

    def steady(self):
        if len(self.recent) == 0:
            return 0.0
        total = 0.0
        for value in self.recent:
            total += value
        return total / len(self.recent)

# ==========================================================================
#  SCREENS
# ==========================================================================

def show_brain(lines):
    for i in range(len(lines)):
        brain.screen.set_cursor(i + 1, 1)
        brain.screen.clear_row()
        brain.screen.print(lines[i])

def peak_text(test):
    if test['peak'] is None:
        return "--.--"
    return "%4.2f" % test['peak']

def show_idle(tests):
    show_brain([
        "CASCADE ROBOT AMPS",
        "A cascade   B claw",
        "X toggle    Y drive",
        "Hold a button %g s." % TEST_SECONDS,
        "Let go to stop it.",
        "Load the part by hand.",
        "peaks so far:",
        "cascade %s  claw %s" % (peak_text(tests[0]), peak_text(tests[1])),
        "toggle  %s  drive %s" % (peak_text(tests[2]), peak_text(tests[3])),
    ])

def show_running(test, reading, left):
    show_brain([
        test['title'],
        "now    %4.2f A%s" % (reading.now, each(test, reading.now)),
        "peak   %4.2f A%s" % (reading.peak, each(test, reading.peak)),
        "steady %4.2f A%s" % (reading.steady(), each(test, reading.steady())),
        "%4.1f s left" % left,
        test['hint1'],
        test['hint2'],
    ])

def show_result(test, reading):
    show_brain([
        test['title'] + "  DONE",
        "peak   %4.2f A%s" % (reading.peak, each(test, reading.peak)),
        "steady %4.2f A%s" % (reading.steady(), each(test, reading.steady())),
        "temp   %4.1f C" % test['groups'][0].temperature(TemperatureUnits.CELSIUS),
        "",
        "Write the PEAK down.",
        "",
        "Let go, then hold",
        "another button.",
    ])

# ==========================================================================
#  THE TOOL
# ==========================================================================

def run_test(test, reading):
    # Spin one mechanism and watch what it draws until the time is up or the
    # button is let go, whichever happens first. A measuring tool should never
    # keep pushing once the hand has come off.
    reading.reset()
    button = getattr(controller_1, 'button' + test['button'])
    started = brain.timer.time(MSEC)
    while True:
        elapsed = (brain.timer.time(MSEC) - started) / 1000.0
        left = TEST_SECONDS - elapsed
        if left <= 0.0:
            break
        if not button.pressing():
            break
        for group in test['groups']:
            group.spin(test['direction'], test['speed'], PERCENT)
        total = 0.0
        for group in test['groups']:
            total += group.current(CurrentUnits.AMP)
        reading.watch(total)
        show_running(test, reading, left)
        wait(LOOP_MS, MSEC)
    for group in test['groups']:
        group.stop()
    return reading.peak

def amps_tool():
    for test in TESTS:
        test['reading'] = Reading()
        test['peak'] = None

    if USE_TORQUE_LIMITS:
        for test in TESTS:
            for group in test['groups']:
                group.set_max_torque(TORQUE_FULL_AMPS, CurrentUnits.AMP)

    brain.screen.set_font(FontType.MONO15)
    show_idle(TESTS)
    controller_1.rumble(".")

    while True:
        for test in TESTS:
            button = getattr(controller_1, 'button' + test['button'])
            if not button.pressing():
                continue
            test['peak'] = run_test(test, test['reading'])
            show_result(test, test['reading'])
            # one press is one test: wait for the hand to come off
            while button.pressing():
                wait(LOOP_MS, MSEC)
            show_idle(TESTS)
        wait(LOOP_MS, MSEC)

amps_tool()
