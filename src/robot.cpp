#include "robot.hpp"
#include "pros/rtos.hpp"
#include "pros/misc.h"

namespace vex_pid {

namespace {
// The full stop button: A.
//
// A is deliberate. It is the one button that is not next to a mechanism
// control, it can be hit with the thumb without letting go of a stick, and -
// unlike a trigger - it cannot be brushed by accident while reaching for
// something else. Both sticks and every other button are ignored entirely
// while it is held.
const auto kStopButton = pros::E_CONTROLLER_DIGITAL_A;

// The whole autonomous routine is given up on after this long, whatever state
// it has reached. Every waiting state polls is_at_target(), and if a target is
// never declared reached there is nothing else to stop the robot.
constexpr std::uint32_t kAutonTimeoutMs = 15000;
}  // namespace

void Robot::initialize(const RobotConfig& config) {
    m_config = config;

    m_imu = new pros::Imu(m_config.imu_port);
    m_imu->reset();
    m_imu_ready = false;
    m_imu_start_ms = pros::millis();

    m_drive.initialize(m_config.drive, m_imu);
    m_cascade.initialize(m_config.cascade);
    m_claw.initialize(m_config.claw);
    m_toggle.initialize(m_config.toggle);

    m_logger.initialize(
        m_config.logger,
        &m_drive,
        &m_cascade,
        &m_claw,
        &m_toggle,
        m_imu
    );

    m_tick_counter = 0;
    m_auton_state = AutonState::Idle;
}

void Robot::subsystems_tick() {
    // The full stop comes first, and it has to come first: the PIDs write the
    // motors from in here, so a stop that only zeroed the driver's own outputs
    // would be overwritten by m_cascade.update() on the very next pass.
    if (m_estopped) {
        stop_all();
        return;
    }

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
    // Read the stop button before anything else, and before the IMU check, so
    // that it works during the couple of seconds the gyro spends calibrating.
    if (stop_requested()) {
        return;
    }

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

    // The rest of the controls are held buttons. Each one is read every pass
    // and passed on as a demand from -1 to +1; when nothing is held the demand
    // is 0, which lets the mechanism ease down instead of stopping dead. The
    // buttons are never passed straight to the motors: the ramping, the stick
    // curve on the drive, the strain easing and the software limits all live
    // in the subsystems, in one place each.

    // Cascade: L1 up, L2 down.
    double cascade_demand = 0.0;
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L1)) {
        cascade_demand += 1.0;
    }
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_L2)) {
        cascade_demand -= 1.0;
    }
    m_cascade.manual_control(cascade_demand);

    // Claw: R1 closes, R2 opens.
    double claw_demand = 0.0;
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R1)) {
        claw_demand += 1.0;
    }
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_R2)) {
        claw_demand -= 1.0;
    }
    m_claw.manual_control(claw_demand);

    // Toggle: B one way, Y the other. It is symmetrical, so which is "red" and
    // which is "blue" is only a matter of which way the driver finds natural -
    // swap the two signs here if it turns out to be the other way round.
    double toggle_demand = 0.0;
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_B)) {
        toggle_demand += 1.0;
    }
    if (master.get_digital(pros::E_CONTROLLER_DIGITAL_Y)) {
        toggle_demand -= 1.0;
    }
    m_toggle.manual_control(toggle_demand);

    // X, Up, Down and Left are deliberately unused. The four cascade presets
    // used to live on A/B/X/Y; they are still there for autonomous
    // (move_to_preset), but the driver now has the arm on the bumpers instead.
}

void Robot::auton_tick() {
    // The stop button works in autonomous too, which is what makes trying a
    // routine on the real robot safe: hold A and the robot gives up.
    if (stop_requested()) {
        return;
    }

    if (!m_imu_ready) return;

    if (m_auton_state == AutonState::Idle) {
        start_auton();
        return;
    }

    if (m_auton_state == AutonState::Done) {
        return;
    }

    // One watchdog over the whole routine. The states below wait on
    // is_at_target(), and the only timeout used to live inside DriveToMidfield
    // - which the routine would never reach if an earlier target never
    // settled, so the robot would drive at full power until the match ended.
    if (pros::millis() - m_auton_start_ms > kAutonTimeoutMs) {
        stop_all();
        m_auton_state = AutonState::Done;
        return;
    }

    run_auton_state();
}

void Robot::stop_all() {
    m_drive.stop();
    m_cascade.stop();
    m_claw.stop();
    m_toggle.stop();
}

bool Robot::stop_button_held() const {
    pros::Controller master(pros::E_CONTROLLER_MASTER);
    return master.get_digital(kStopButton) != 0;
}

bool Robot::stop_requested() {
    if (stop_button_held()) {
        if (!m_estopped) {
            m_estopped = true;
            stop_all();
        }
        return true;
    }

    m_estopped = false;
    return false;
}

void Robot::disabled_tick() {
    stop_all();
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