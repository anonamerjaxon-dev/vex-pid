#include "subsystems/claw.hpp"
#include "pros/rtos.hpp"

namespace vex_pid {

void Claw::initialize(const ClawConfig& config) {
    m_config = config;

    m_motor = new pros::Motor(m_config.motor_port);
    m_motor->set_gearing(m_config.gearset);
    m_motor->set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
}

void Claw::open() {
    m_busy = true;
    m_stall_check_active = false;
    m_start_ms = pros::millis();

    // Say which action this is. update() branches on m_is_open, and the driver
    // decides what to do next from is_open(), so if the flag were left over
    // from the previous action this would drive the claw the wrong way.
    m_is_open = true;

    m_motor->move(-m_config.open_power);
}

void Claw::close() {
    m_busy = true;
    m_stall_check_active = false;
    m_start_ms = pros::millis();

    m_is_open = false;

    m_motor->move(m_config.close_power);
}

void Claw::stop() {
    m_busy = false;
    m_motor->move(0);
}

void Claw::update() {
    if (!m_busy) {
        return;
    }

    // Measured, not counted. The old code assumed every call was exactly 10 ms
    // apart, so when a pass round the loop took longer than that the claw's
    // 400 ms open ran long and the 2 s squeeze timeout stretched with it.
    const int elapsed_ms = static_cast<int>(pros::millis() - m_start_ms);

    if (m_is_open) {
        if (elapsed_ms >= m_config.open_time_ms) {
            m_motor->move(0);
            m_busy = false;
            return;
        }
    } else {
        if (elapsed_ms >= m_config.stall_check_after_ms) {
            m_stall_check_active = true;
        }

        if (m_stall_check_active) {
            if (current_draw_ma() > m_config.stall_current_ma) {
                m_motor->move(m_config.hold_power);
                m_busy = false;
                return;
            }
        }

        if (elapsed_ms > m_config.close_timeout_ms) {
            m_motor->move(m_config.hold_power);
            m_busy = false;
            return;
        }
    }
}

int Claw::current_draw_ma() const {
    if (m_motor) {
        return static_cast<int>(m_motor->get_current_draw());
    }
    return 0;
}

}  // namespace vex_pid