#include "main.h"
#include "robot.hpp"
#include "pros/rtos.h"

static vex_pid::Robot robot;

void initialize() {
    vex_pid::RobotConfig config;

    config.drive.left_ports = {-11, -17};
    config.drive.right_ports = {1, 10};

    config.drive.wheel_diameter_in = 3.25;
    config.drive.gear_ratio = 60.0 / 18.0;

    config.drive.gearset = pros::E_MOTOR_GEARSET_18;

    config.cascade.motor_ports = {13, -2};

    config.cascade.gearset = pros::E_MOTOR_GEARSET_36;

    config.cascade.max_extension_inches = 24.0;
    config.cascade.inches_per_encoder_degree = 0.01;

    config.cascade.presets[0] = 0.0;
    config.cascade.presets[1] = 6.0;
    config.cascade.presets[2] = 14.0;
    config.cascade.presets[3] = 22.0;

    config.cascade.gravity_feedforward = 8.0;

    config.claw.motor_port = 16;

    config.claw.open_power = 127;
    config.claw.close_power = 80;
    config.claw.hold_power = 15;
    config.claw.open_time_ms = 400;
    config.claw.stall_current_ma = 1200;
    config.claw.stall_check_after_ms = 200;

    config.toggle.motor_ports = {18, -8};

    config.toggle.yellow_angle = 0.0;
    config.toggle.red_angle = 90.0;
    config.toggle.blue_angle = -90.0;

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