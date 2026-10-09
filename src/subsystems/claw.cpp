#include "subsystems/claw.hpp"
#include "control_feel.hpp"
#include "pros/rtos.hpp"

#include <algorithm>
#include <cmath>

namespace vex_pid {

namespace {
constexpr std::uint32_t kNominalLoopMs = 20;
}  // namespace

void Claw::initialize(const ClawConfig& config) {
    m_config = config;

    m_motor = new pros::Motor(m_config.motor_port);
    m_motor->set_gearing(m_config.gearset);
    m_motor->set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);

    m_manual_demand = 0.0;
    m_ramped = 0.0;
    m_last_ms = 0;

    m_guard = StrainGuard(m_config.guard);
    m_guard.reset();
    apply_current_limit();
}

void Claw::open() {
    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

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
    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

    m_busy = true;
    m_stall_check_active = false;
    m_start_ms = pros::millis();

    m_is_open = false;

    m_motor->move(m_config.close_power);
}

void Claw::stop() {
    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

    m_busy = false;
    m_guard.clear_block();
    m_motor->move(0);
}

void Claw::update() {
    const std::uint32_t now = pros::millis();
    const double dt_seconds = loop_seconds(now, m_last_ms, kNominalLoopMs);
    m_last_ms = now;

    if (m_manual_mode) {
        update_manual(dt_seconds);
        return;
    }

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

void Claw::update_manual(double dt_seconds) {
    double wanted = m_manual_demand * m_config.manual_speed_percent;

    // A claw closed on a game object, or already at the end of its travel, is a
    // motor told to turn that has stopped. The guard reads that and eases the
    // squeeze back to a current the motor can sit at, so the grip holds without
    // the motor cooking. Judged against the command the claw is really being
    // given, so easing never creates the stall it is trying to avoid.
    const double judged = wanted * m_guard.factor();
    const double factor = m_guard.update(
        current_draw_amps(), measured_speed(), judged
    );
    wanted *= factor;

    m_ramped = rate_limit(
        m_ramped, wanted, ramp_step(m_config.ramp_percent_per_second, dt_seconds)
    );

    apply_current_limit();
    m_motor->move(static_cast<std::int32_t>(std::lround(m_ramped)));
}

void Claw::manual_control(double demand) {
    m_manual_mode = true;
    m_busy = false;
    m_manual_demand = std::clamp(demand, -1.0, 1.0);
}

void Claw::apply_current_limit() {
    if (m_motor && m_guard.take_limit_change()) {
        m_motor->set_current_limit(m_guard.limit_ma());
    }
}

int Claw::current_draw_ma() const {
    if (m_motor) {
        return static_cast<int>(m_motor->get_current_draw());
    }
    return 0;
}

double Claw::current_draw_amps() const {
    return static_cast<double>(current_draw_ma()) / 1000.0;
}

double Claw::measured_speed() const {
    if (!m_motor) {
        return 0.0;
    }
    return velocity_fraction(
        m_motor->get_actual_velocity(), m_config.motor_max_rpm
    );
}

}  // namespace vex_pid
