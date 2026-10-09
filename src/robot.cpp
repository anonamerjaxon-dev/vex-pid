#include "robot.hpp"
#include "pros/misc.h"
#include "pros/rtos.hpp"
#include "pros/screen.hpp"

#include <cstdio>
#include <cstdint>

namespace vex_pid {

namespace {
// The full stop button: A.
//
// A is deliberate. It is the one button that is not next to a mechanism
// control, it can be hit with the thumb without letting go of a stick, and -
// unlike a trigger - it cannot be brushed by accident while reaching for
// something else.
//
// It is a TOGGLE, not a hold. One press stops every motor and the robot stays
// stopped when the thumb comes off, so the driver does not have to keep a
// finger on A while they sort out whatever went wrong. Pressing A again hands
// control back. Both sticks and every other button are ignored the whole time
// the robot is stopped.
const auto kStopButton = pros::E_CONTROLLER_DIGITAL_A;
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
    m_estopped = false;
    clear_stop_screen();
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
    // It returns true for as long as the robot is stopped, which is until A is
    // pressed a second time.
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
    // used to live on A/B/X/Y and were the only autonomous-facing controls
    // here; the driver drives the arm on the bumpers instead.

    // A live readout of what the strain guards can see. This is the number the
    // measured thresholds are for: hold a mechanism into a strain and the amps
    // climb while the ease figure falls. Five times a second is enough to read
    // and leaves the screen alone the rest of the time.
    const std::uint32_t now_ms = pros::millis();
    if (now_ms - m_last_readout_ms >= 200) {
        m_last_readout_ms = now_ms;
        show_guard_readout();
    }
}

// The top two screen lines: what each guarded mechanism is drawing and how far
// the guard has eased it back, so the automatic slow-down at strain can be
// watched happening rather than taken on trust.
void Robot::show_guard_readout() {
    char text[48];

    std::snprintf(text, sizeof(text), "casc %5.2f A  ease %3.0f%%%s",
                  m_cascade.current_draw_amps(),
                  100.0 * m_cascade.guard_factor(),
                  m_cascade.is_blocked() ? "  BLOCKED" : "");
    pros::screen::print(pros::E_TEXT_MEDIUM, 0, "%s", text);

    std::snprintf(text, sizeof(text), "claw %5.2f A  ease %3.0f%%%s",
                  m_claw.current_draw_amps(),
                  100.0 * m_claw.guard_factor(),
                  m_claw.is_blocked() ? "  BLOCKED" : "");
    pros::screen::print(pros::E_TEXT_MEDIUM, 1, "%s", text);
}

void Robot::stop_all() {
    m_drive.stop();
    m_cascade.stop();
    m_claw.stop();
    m_toggle.stop();
}

// A flash of red text where the driver is already looking, so "why will it not
// drive" has an answer on the brain as well as in the controller's hands.
void Robot::show_stop_screen() {
    pros::screen::set_pen(pros::Color::red);
    pros::screen::print(pros::E_TEXT_LARGE_CENTER, 5, "FULL STOP");
    pros::screen::print(pros::E_TEXT_MEDIUM_CENTER, 7, "press A to drive");
}

void Robot::clear_stop_screen() {
    pros::screen::erase();
    pros::screen::set_pen(pros::Color::white);
}

bool Robot::stop_requested() {
    // A is a toggle. get_digital_new_press() is edge triggered, so holding the
    // button down produces exactly one press - the robot cannot flicker
    // between stopped and driving while a thumb rests on it.
    pros::Controller master(pros::E_CONTROLLER_MASTER);

    if (master.get_digital_new_press(kStopButton)) {
        m_estopped = !m_estopped;

        if (m_estopped) {
            stop_all();
            show_stop_screen();
            // A long rumble: the robot has stopped and will stay stopped.
            master.rumble("-");
        } else {
            // Hands control back. stop_all() here as well, so every ramp is
            // zeroed and the wheels and mechanisms ease up from nothing rather
            // than jumping straight back to whatever the sticks are asking for
            // - the sticks may well have been pushed around during the stop.
            stop_all();
            clear_stop_screen();
            master.rumble(". ");
        }
    }

    return m_estopped;
}

void Robot::disabled_tick() {
    stop_all();
    // Being disabled already stops everything, and it should not leave the
    // robot latched stopped for the next time it is enabled - the driver would
    // press the sticks and nothing would happen.
    m_estopped = false;
    clear_stop_screen();
}

}  // namespace vex_pid
