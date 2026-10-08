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
# 	Project:      Cascade Robot (PID Auton)
#	Description:  Driver control + PID autonomous for the cascade robot.
#	              The PID setup is adapted from the vex-pid repo
#	              (github.com/anonamerjaxon-dev/vex-pid).
#
#	Controls:
#	  Left stick up/down ...... drive forward / back
#	  Right stick left/right .. turn
#	  L1 / L2 ................. cascade up / down
#	  R1 / R2 ................. claw close / open
#	  Down arrow .............. spin toggle (hold)
#
#	Autonomous building blocks:
#	  drive_distance(inches)    drive straight, holding heading
#	  turn_to_heading(degrees)  face a heading (0 = start, clockwise +)
#	  turn_to_point(x, y)       face a spot on the field (inches)
#	  drive_to_point(x, y)      turn toward a spot, then drive to it
#	  cascade.move_to(degrees)  lift to a height (motor degrees)
#	  toggle.move_to(degrees)   spin the toggle to an angle
#	  claw_open() / claw_close()
#
# ------------------------------------------

# Library imports
from vex import *
import math

# --- Settings (change these to tune the robot) ---

# Which autonomous runs: "pid_test", "odom_test", "example" or "none"
AUTON_ROUTINE = "pid_test"

# Gear cartridge inside each 11W motor (from the vex-pid design doc):
#   GREEN = GearSetting.RATIO_18_1 (200 RPM)
#   BLUE  = GearSetting.RATIO_6_1  (600 RPM)
#   RED   = GearSetting.RATIO_36_1 (100 RPM)
DRIVE_GEARS = GearSetting.RATIO_18_1
CASCADE_GEARS = GearSetting.RATIO_36_1
CLAW_GEARS = GearSetting.RATIO_18_1

# Driver control
CASCADE_SPEED = 100   # percent
CLAW_SPEED = 60       # percent
TOGGLE_SPEED = 100    # percent
SYNC_KP = 0.5         # percent per degree out of sync
DEADBAND = 5

# Drive measurements (from the vex-pid design doc - check on the robot)
DRIVE_WHEEL_DIAMETER_IN = 3.25
DRIVE_MOTOR_GEAR_TEETH = 18   # gear on the motor
DRIVE_WHEEL_GEAR_TEETH = 60   # gear on the wheel
TRACK_WIDTH_IN = 12.0         # center of left wheels to center of right wheels

# Inertial sensor. None = not on the robot. If you add one, put its port
# here and turns get much more accurate. If the brain's Devices screen
# shows the module on port 9 is an inertial sensor, use Ports.PORT9.
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

# Cascade heights in motor degrees from the bottom. These are vex-pid's
# placeholder presets - find the real ones by driving the lift to each
# height and reading the degrees on the brain screen.
CASCADE_PRESETS = {"down": 0, "low": 600, "mid": 1400, "high": 2200}
CASCADE_MAX_DEG = 2400

# Toggle angles in motor degrees (vex-pid placeholders)
TOGGLE_ANGLES = {"yellow": 0, "red": 90, "blue": -90}

# Claw: open for a set time, close until it squeezes something
CLAW_OPEN_MS = 400
CLAW_STALL_CHECK_MS = 200      # ignore the current spike when starting up
CLAW_STALL_AMPS = 1.2
CLAW_CLOSE_TIMEOUT_MS = 2000
CLAW_HOLD_VOLTS = 1.4          # light squeeze so the game piece stays put

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
toggle_18 = Motor(Ports.PORT18, GearSetting.RATIO_18_1, True)
toggle_8 = Motor(Ports.PORT8, GearSetting.RATIO_18_1, False)

# Claw: on the long 1500mm cable
claw_16 = Motor(Ports.PORT16, CLAW_GEARS, False)

# Port 9: a single device nobody has identified yet. Not used.

DRIVE_INCHES_PER_DEG = (math.pi * DRIVE_WHEEL_DIAMETER_IN / 360
                        * DRIVE_MOTOR_GEAR_TEETH / DRIVE_WHEEL_GEAR_TEETH)
TRACKER_INCHES_PER_DEG = math.pi * TRACKER_WHEEL_DIAMETER_IN / 360
SYNC_KP_VOLTS = SYNC_KP * 12 / 100

# --- Helpers ---

def clamp(value):
    return max(-100, min(100, value))

def limit(value, biggest):
    return max(-biggest, min(biggest, value))

def wrap_180(angle):
    while angle > 180:
        angle -= 360
    while angle <= -180:
        angle += 360
    return angle

def spin_synced(motor_a, motor_b, speed):
    # Whichever motor gets ahead is slowed down until the other catches up
    error = motor_a.position(DEGREES) - motor_b.position(DEGREES)
    correction = error * SYNC_KP
    motor_a.spin(FORWARD, clamp(speed - correction), PERCENT)
    motor_b.spin(FORWARD, clamp(speed + correction), PERCENT)

def run_pair(motor_a, motor_b, speed, was_moving):
    if speed != 0:
        spin_synced(motor_a, motor_b, speed)
        return True
    # Stop only once when the button is released so HOLD keeps its spot
    if was_moving:
        motor_a.stop()
        motor_b.stop()
    return False

def run_single(motor, speed, was_moving):
    if speed != 0:
        motor.spin(FORWARD, speed, PERCENT)
        return True
    if was_moving:
        motor.stop()
    return False

def apply_deadband(value):
    if abs(value) < DEADBAND:
        return 0
    return value

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
    # Adapted from vex-pid's Cascade and Toggle subsystems, plus the same
    # left/right sync correction driver control uses.

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

def drive_distance(inches, max_volts=12, timeout_ms=None, heading=None):
    # Drive straight (negative inches = backward) while holding heading.
    # Gives up after timeout_ms (default: 1 s + 0.15 s per inch).
    # Adapted from vex-pid DriveBase::drive_straight.
    if heading is None:
        heading = odom.heading()
    if timeout_ms is None:
        timeout_ms = 1000 + abs(inches) * 150
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

def turn_to_heading(target, max_volts=12, timeout_ms=None):
    # Turn in place the short way around to face target degrees.
    # Gives up after timeout_ms (default: 1 s + 15 ms per degree).
    # Adapted from vex-pid DriveBase::turn_to_heading.
    if timeout_ms is None:
        timeout_ms = 1000 + abs(wrap_180(target - odom.heading())) * 15
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

def turn_to_point(x, y, max_volts=12, timeout_ms=None):
    return turn_to_heading(heading_to(x, y), max_volts, timeout_ms)

def drive_to_point(x, y, max_volts=12, timeout_ms=None):
    # Uses odometry: works now with the motor encoders, and gets more
    # accurate once tracking wheels are added
    turn_to_point(x, y, max_volts)
    dx = x - odom.x
    dy = y - odom.y
    distance = math.sqrt(dx * dx + dy * dy)
    return drive_distance(distance, max_volts, timeout_ms, heading_to(x, y))

def claw_open():
    # Timed open (vex-pid Claw::open)
    claw_16.spin(REVERSE, CLAW_SPEED, PERCENT)
    wait(CLAW_OPEN_MS, MSEC)
    claw_16.stop()

def claw_close():
    # Close until the motor current spikes from squeezing something, then
    # keep a light grip. Gives up after the timeout. (vex-pid Claw::close)
    claw_16.spin(FORWARD, CLAW_SPEED, PERCENT)
    start = brain.timer.system()
    while auton_active:
        elapsed = brain.timer.system() - start
        if elapsed > CLAW_CLOSE_TIMEOUT_MS:
            break
        if elapsed > CLAW_STALL_CHECK_MS and claw_16.current(CurrentUnits.AMP) > CLAW_STALL_AMPS:
            break
        wait(10, MSEC)
    claw_16.spin(FORWARD, CLAW_HOLD_VOLTS, VoltageUnits.VOLT)

# --- Background loop (tracking, mechanism PID, brain screen) ---

auton_active = False
background_ticks = 0

def show_status():
    brain.screen.set_cursor(1, 1)
    brain.screen.clear_row()
    brain.screen.print("X %.1f  Y %.1f  H %.1f" % (odom.x, odom.y, odom.heading()))
    brain.screen.set_cursor(2, 1)
    brain.screen.clear_row()
    brain.screen.print("Cascade %d  Toggle %d" % (cascade.position(), toggle.position()))
    brain.screen.set_cursor(3, 1)
    brain.screen.clear_row()
    brain.screen.print("Auton: " + AUTON_ROUTINE)

def background_tick():
    global background_ticks
    odom.update()
    if auton_active:
        cascade.update()
        toggle.update()
    background_ticks += 1
    if background_ticks % 25 == 0:
        show_status()

def background_loop():
    while True:
        background_tick()
        wait(10, MSEC)

# --- Autonomous routines ---

def auton_pid_test():
    # Use this to tune the PID. It should end where it started, facing
    # the same way. If it overshoots or stops short, tune the gains.
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

def auton_example():
    # vex-pid's example routine (robot.cpp) rebuilt with these moves.
    # The distances are guesses - measure them on a real field.
    claw_close()                                                   # grip the preload
    drive_distance(24)                                             # drive to goal 1
    cascade.move_to(CASCADE_PRESETS["mid"], wait_until_done=True)
    claw_open()                                                    # score the preload
    cascade.move_to(CASCADE_PRESETS["down"])                       # lower while driving
    drive_distance(12)                                             # drive to a pin
    claw_close()                                                   # grab it
    turn_to_heading(90)
    drive_distance(18)                                             # drive to goal 2
    cascade.move_to(CASCADE_PRESETS["mid"], wait_until_done=True)
    claw_open()                                                    # score the pin
    cascade.move_to(CASCADE_PRESETS["down"])
    drive_distance(30)                                             # head to midfield

def auton_none():
    pass

ROUTINES = {
    "pid_test": auton_pid_test,
    "odom_test": auton_odom_test,
    "example": auton_example,
    "none": auton_none,
}

# --- Startup ---

left_drive.set_stopping(BRAKE)
right_drive.set_stopping(BRAKE)
cascade_left_13.set_stopping(HOLD)
cascade_right_2.set_stopping(HOLD)
toggle_18.set_stopping(HOLD)
toggle_8.set_stopping(HOLD)
claw_16.set_stopping(HOLD)

# Start both sides of each synced pair from the same position.
# The cascade must be all the way down when the program starts.
cascade_left_13.set_position(0, DEGREES)
cascade_right_2.set_position(0, DEGREES)
toggle_18.set_position(0, DEGREES)
toggle_8.set_position(0, DEGREES)

brain.screen.print("Calibrating - don't move the robot")
odom = Odometry()
odom.calibrate()
odom.set_pose(0, 0, 0)
brain.screen.clear_screen()

drive_pid = PID(DRIVE_GAINS)
heading_pid = PID(HEADING_GAINS)
turn_pid = PID(TURN_GAINS)
cascade = SyncedMechanism(cascade_left_13, cascade_right_2, CASCADE_GAINS,
                          CASCADE_FEEDFORWARD_VOLTS, 0, CASCADE_MAX_DEG)
toggle = SyncedMechanism(toggle_18, toggle_8, TOGGLE_GAINS)

Thread(background_loop)

# --- Autonomous ---

def autonomous():
    global auton_active
    auton_active = True
    # Start of the routine is (0, 0) facing 0. A routine can call
    # odom.set_pose(x, y, heading) first to use real field coordinates.
    odom.set_pose(0, 0, 0)
    cascade.hold_here()
    toggle.hold_here()
    ROUTINES.get(AUTON_ROUTINE, auton_none)()
    stop_drive()

# --- Driver control ---

def driver_control():
    global auton_active
    # Stop any autonomous PID so it doesn't fight the driver
    auton_active = False
    cascade.release()
    toggle.release()

    cascade_moving = False
    claw_moving = False
    toggle_moving = False

    while True:
        # Split arcade drive
        forward = apply_deadband(controller_1.axis3.position())
        turn = apply_deadband(controller_1.axis1.position())
        left_drive.spin(FORWARD, clamp(forward + turn), PERCENT)
        right_drive.spin(FORWARD, clamp(forward - turn), PERCENT)

        # Cascade: L1 up, L2 down
        cascade_speed = 0
        if controller_1.buttonL1.pressing():
            cascade_speed = CASCADE_SPEED
        elif controller_1.buttonL2.pressing():
            cascade_speed = -CASCADE_SPEED
        cascade_moving = run_pair(cascade_left_13, cascade_right_2, cascade_speed, cascade_moving)

        # Claw: R1 close, R2 open
        claw_speed = 0
        if controller_1.buttonR1.pressing():
            claw_speed = CLAW_SPEED
        elif controller_1.buttonR2.pressing():
            claw_speed = -CLAW_SPEED
        claw_moving = run_single(claw_16, claw_speed, claw_moving)

        # Toggle: hold Down arrow to spin
        toggle_speed = 0
        if controller_1.buttonDown.pressing():
            toggle_speed = TOGGLE_SPEED
        toggle_moving = run_pair(toggle_18, toggle_8, toggle_speed, toggle_moving)

        wait(20, MSEC)

competition = Competition(driver_control, autonomous)
