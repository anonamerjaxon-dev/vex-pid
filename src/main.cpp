#include "main.h"
#include "robot.hpp"
#include "pros/rtos.hpp"

static vex_pid::Robot robot;

void initialize() {
    vex_pid::RobotConfig config;

    config.drive.left_ports = {-11, -17};
    config.drive.right_ports = {1, 10};

    config.drive.wheel_diameter_in = 3.25;
    config.drive.gear_ratio = 60.0 / 18.0;

    config.drive.gearset = pros::E_MOTOR_GEARSET_18;

    // The driver's stick feel, ported from the Python drive program
    // (vexcode-python/cascade_robot_drive_v2.py). 51 is 40% of 127, which is
    // where the Python program sits. See include/control_feel.hpp for what
    // each number does, and vexcode-python/CONTROL_FEEL.md for why.
    config.drive.drive_speed_max = 51.0;   // 40% of 127
    config.drive.turn_speed_max = 44.0;    // 35% of 127, a little gentler again
    config.drive.ramp_percent_per_second = 250.0;

    config.drive.stick.deadband = 6.35;           // 5% of the stick's travel
    config.drive.stick.fine_end = 0.85;           // gentle over the first 85%
    config.drive.stick.fine_top = 0.40;           // ...up to 40% of the ceiling
    config.drive.stick.expo = 2.0;                // and a curve inside that
    config.drive.stick.min_move_fraction = 0.15;  // but never below 15%

    config.cascade.motor_ports = {13, -2};

    config.cascade.gearset = pros::E_MOTOR_GEARSET_36;

    // The free speed of a red 36:1 motor. Used to work out how far the arm
    // will travel before the next pass. 200 for green 18:1, 600 for blue 6:1.
    config.cascade.motor_max_rpm = 100.0;

    config.cascade.max_extension_inches = 24.0;
    config.cascade.inches_per_encoder_degree = 0.01;

    // The four positions the arm can be sent to, for autonomous only - the
    // driver drives the arm with L1 and L2 now.
    config.cascade.presets[0] = 0.0;
    config.cascade.presets[1] = 6.0;
    config.cascade.presets[2] = 14.0;
    config.cascade.presets[3] = 22.0;

    config.cascade.gravity_feedforward = 8.0;

    // --- the driver's arm ----------------------------------------------------
    // 40 is the same ceiling the Python drive program uses, so the arm behaves
    // the same on the robot as it did in the test harness.
    config.cascade.manual_speed_percent = 40.0;
    config.cascade.creep_speed_percent = 10.0;
    config.cascade.creep_band_inches = 0.25;
    config.cascade.ramp_percent_per_second = 250.0;

    // Where the arm tared its encoder at start-up, less a hair. There is no
    // physical stopper, so this is what keeps the chain on the sprocket.
    config.cascade.floor_inches = -0.02;
    config.cascade.safety_factor = 1.5;

    // The strain guard. watch out for the units: the cascade reads the whole
    // pair added together, so strain_amps is a pair total, but the current
    // LIMIT is applied to each motor separately, so ease_amps and relaxed_amps
    // are per motor.
    config.cascade.guard.strain_amps = 2.0;    // the pair, added up
    config.cascade.guard.ease_amps = 1.0;      // each motor, while straining
    config.cascade.guard.relaxed_amps = 2.5;   // each motor, when fine
    config.cascade.guard.ease_speed = 0.35;

    config.claw.motor_port = 16;

    // The autonomous open/close actions still use these.
    config.claw.open_power = 127;
    config.claw.close_power = 80;
    config.claw.hold_power = 15;
    config.claw.open_time_ms = 400;
    config.claw.stall_current_ma = 1200;
    config.claw.stall_check_after_ms = 200;

    // --- the driver's claw ---------------------------------------------------
    // Simple: R1 closes, R2 opens, both held. No stick curve - a claw wants to
    // grip, not to be driven at a speed.
    config.claw.manual_speed_percent = 40.0;
    config.claw.ramp_percent_per_second = 250.0;
    config.claw.motor_max_rpm = 200.0;   // green 18:1

    // A claw closed on a game object is a motor told to turn that has stopped.
    // The guard treats that as the grip: it eases the squeeze back and caps the
    // current, so the claw holds firmly without cooking the motor. One motor
    // here, so every number is that motor.
    config.claw.guard.strain_amps = 1.5;
    config.claw.guard.ease_amps = 1.0;
    config.claw.guard.relaxed_amps = 2.0;
    config.claw.guard.ease_speed = 0.35;

    config.toggle.motor_ports = {18, -8};

    config.toggle.yellow_angle = 0.0;
    config.toggle.red_angle = 90.0;
    config.toggle.blue_angle = -90.0;

    // --- the driver's toggle -------------------------------------------------
    // B one way, Y the other, held. No stick curve and no strain guard: it is a
    // "go that way" button, and it has hard stops at both ends of its travel.
    config.toggle.manual_speed_percent = 40.0;
    config.toggle.ramp_percent_per_second = 250.0;

    config.logger.filename = "/usd/vex_log.csv";
    config.logger.log_rate_hz = 50;

    config.imu_port = 9;   // the unidentified module; no IMU confirmed yet

    robot.initialize(config);
}

void disabled() {
    robot.disabled_tick();
}

void competition_initialize() {
}

void autonomous() {
    while (true) {
        robot.auton_tick();
        robot.subsystems_tick();
        pros::delay(10);
    }
}

void opcontrol() {
    while (true) {
        robot.driver_tick();
        robot.subsystems_tick();
        pros::delay(10);
    }
}