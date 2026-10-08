#include "robot.hpp"
#include "pros/rtos.hpp"
#include "pros/misc.h"

namespace vex_pid {

void Robot::initialize(const RobotConfig& config) {
    m_config = config;

    m_imu = pros::Imu(m_config.imu_port);
    m_imu.reset();
    m_imu_ready = false;
    m_imu_start_ms = pros::millis();

    m_drive.initialize(m_config.drive, &m_imu);
    m_cascade.initialize(m_config.cascade);
    m_claw.initialize(m_config.claw);
    m_toggle.initialize(m_config.toggle);

    m_logger.initialize(
        m_config.logger,
        &m_drive,
        &m_cascade,
        &m_claw,
        &m_toggle,
        &m_imu
    );

    m_tick_counter = 0;
    m_auton_state = AutonState::Idle;
}

void Robot::subsystems_tick() {
    m_tick_counter++;

    if (!m_imu_ready) {
        int elapsed = static_cast<int>(pros::millis()) - m_imu_start_ms;
        if (elapsed >= kImuCalibrateMs) {
            m_imu_ready = true;
            m_imu_start_ms = 0;
        }
    }

    if (m_imu_ready) {
        m_drive.update();
        m_cascade.update();
        m_toggle.update();
    }

    m_claw.update();

    if (m_logger.is_active() && m_tick_counter % 5 == 0) {
        m_logger.log_sample(pros::millis());
    }
}

void Robot::driver_tick() {
    if (!m_imu_ready) return;

    pros::Controller master(pros::E_CONTROLLER_MASTER);

    int throttle = master.get_analog(pros::E_CONTROLLER_ANALOG_LEFT_Y);
    int turn = master.get_analog(pros::E_CONTROLLER_ANALOG_RIGHT_X);
    // Pass the sticks straight through. The stick curve, the split arcade mix
    // and the ramp all live inside manual_control now, so the old
    // "turn * 0.7" fudge is gone - the curve is what makes the middle gentle.
    m_drive.manual_control(
        static_cast<double>(throttle),
        static_cast<double>(turn)
    );

    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        m_cascade.manual_control(127);
    } else if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        m_cascade.manual_control(-127);
    } else if (!m_cascade.is_at_target()) {
    } else {
        m_cascade.stop();
    }

    bool claw_pressed = master.get_digital(pros::E_CONTROLLER_DIGITAL_L1);
    if (claw_pressed && !m_claw_was_pressed && !m_claw.is_busy()) {
        if (m_claw.is_open()) {
            m_claw.close();
        } else {
            m_claw.open();
        }
    }
    m_claw_was_pressed = claw_pressed;

    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_A)) {
        m_cascade.move_to_preset(0);
    }
    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_B)) {
        m_cascade.move_to_preset(1);
    }
    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_X)) {
        m_cascade.move_to_preset(2);
    }
    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_Y)) {
        m_cascade.move_to_preset(3);
    }

    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_UP)) {
        m_toggle.flip_to_red();
    }
    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_DOWN)) {
        m_toggle.flip_to_blue();
    }
    if (master.get_digital_new_press(pros::E_CONTROLLER_DIGITAL_LEFT)) {
        m_toggle.flip_to_yellow();
    }
}

void Robot::auton_tick() {
    if (!m_imu_ready) return;

    if (m_auton_state == AutonState::Idle) {
        start_auton();
        return;
    }

    if (m_auton_state == AutonState::Done) {
        return;
    }

    run_auton_state();
}

void Robot::disabled_tick() {
    m_drive.stop();
    m_cascade.stop();
    m_claw.stop();
    m_toggle.stop();
    m_auton_state = AutonState::Idle;
}

void Robot::start_auton() {
    m_auton_start_ms = pros::millis();
    m_state_start_ms = m_auton_start_ms;
    m_tick_counter = 0;

    m_drive.stop();
    m_cascade.move_to(0.0);
    m_claw.open();
    m_toggle.flip_to_yellow();

    m_logger.start_session();

    m_auton_state = AutonState::DriveToGoal1;
}

void Robot::run_auton_state() {
    uint32_t now = pros::millis();
    uint32_t state_elapsed = now - m_state_start_ms;

    switch (m_auton_state) {
        case AutonState::DriveToGoal1: {
            if (state_elapsed < 100) {
                m_drive.drive_straight(24.0);
            }
            if (m_drive.is_at_target()) {
                m_auton_state = AutonState::ScorePreload;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::ScorePreload: {
            if (state_elapsed < 50) {
                m_cascade.move_to_preset(2);
                m_claw.open();
            }
            if (state_elapsed > 500 && state_elapsed < 550) {
                m_claw.open();
            }
            if (state_elapsed > 800) {
                m_cascade.move_to(0.0);
                m_auton_state = AutonState::DriveToPin;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::DriveToPin: {
            if (state_elapsed < 100) {
                m_drive.drive_straight(12.0);
            }
            if (m_drive.is_at_target()) {
                m_auton_state = AutonState::GrabPin;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::GrabPin: {
            if (state_elapsed < 50) {
                m_claw.close();
            }
            if (state_elapsed > 600) {
                m_auton_state = AutonState::DriveToGoal2;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::DriveToGoal2: {
            if (state_elapsed < 100) {
                m_drive.turn_to_heading(90.0);
            }
            if (m_drive.is_at_target() && state_elapsed > 300) {
                m_drive.drive_straight(18.0);
            }
            if (m_drive.is_at_target() && state_elapsed > 1000) {
                m_auton_state = AutonState::ScorePin;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::ScorePin: {
            if (state_elapsed < 50) {
                m_cascade.move_to_preset(2);
            }
            if (state_elapsed > 500 && state_elapsed < 550) {
                m_claw.open();
            }
            if (state_elapsed > 800) {
                m_cascade.move_to(0.0);
                m_auton_state = AutonState::DriveToMidfield;
                m_state_start_ms = now;
            }
            break;
        }
        case AutonState::DriveToMidfield: {
            if (state_elapsed < 100) {
                m_drive.drive_straight(30.0);
            }
            if (now - m_auton_start_ms > 14000) {
                m_drive.stop();
                m_auton_state = AutonState::Done;
            }
            break;
        }
        default:
            break;
    }
}

}  // namespace vex_pid