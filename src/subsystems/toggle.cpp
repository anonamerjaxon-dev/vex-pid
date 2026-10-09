#include "subsystems/toggle.hpp"
#include "control_feel.hpp"
#include "pros/rtos.hpp"

#include <algorithm>
#include <cmath>

namespace vex_pid {

namespace {
constexpr std::uint32_t kNominalLoopMs = 20;
}  // namespace

void Toggle::initialize(const ToggleConfig& config) {
    m_config = config;

    m_motors = new pros::MotorGroup(
        std::vector<std::int8_t>(
            m_config.motor_ports.begin(),
            m_config.motor_ports.end()
        )
    );

    for (auto port : m_config.motor_ports) {
        pros::Motor motor(port);
        motor.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    }

    m_position_pid = PIDController(m_config.position_pid);
    m_target_state = ToggleState::Yellow;
    m_target_angle = m_config.yellow_angle;

    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;
    m_last_ms = 0;
}

bool Toggle::flip_to(ToggleState state) {
    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

    m_target_state = state;

    switch (state) {
        case ToggleState::Yellow:
            m_target_angle = m_config.yellow_angle;
            break;
        case ToggleState::Red:
            m_target_angle = m_config.red_angle;
            break;
        case ToggleState::Blue:
            m_target_angle = m_config.blue_angle;
            break;
    }

    m_position_pid.set_target(m_target_angle);
    m_position_pid.reset();

    return is_at_target();
}

bool Toggle::flip_to_yellow() { return flip_to(ToggleState::Yellow); }
bool Toggle::flip_to_red()    { return flip_to(ToggleState::Red); }
bool Toggle::flip_to_blue()   { return flip_to(ToggleState::Blue); }

void Toggle::stop() {
    m_manual_mode = false;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

    m_motors->move(0);

    // Adopt where the toggle actually is. Zeroing the motors alone was not
    // enough: the position PID still had its old target, so the very next
    // update() drove the toggle straight back to it. That is why the toggle
    // came back to life on its own after being disabled.
    m_target_angle = current_position();
    m_position_pid.set_target(m_target_angle);
    m_position_pid.reset();
}

void Toggle::manual_control(double demand) {
    if (!m_manual_mode) {
        // Entering the manual path: adopt where the toggle is now, so that
        // letting go and falling back to the PID does not snap it back to
        // whatever target it was last given.
        m_target_angle = current_position();
        m_position_pid.set_target(m_target_angle);
        m_position_pid.reset();
    }

    m_manual_mode = true;
    m_manual_demand = std::clamp(demand, -1.0, 1.0);
}

void Toggle::test_raw(double percent) {
    // Amps measuring only. No ramp and no PID.
    if (!m_manual_mode) {
        m_target_angle = current_position();
        m_position_pid.set_target(m_target_angle);
        m_position_pid.reset();
    }
    m_manual_mode = true;
    m_manual_demand = 0.0;
    m_ramped = 0.0;
    m_motors->move(static_cast<std::int32_t>(std::lround(percent)));
}

double Toggle::current_draw_amps() const {
    double milliamps = 0.0;
    for (const auto value : m_motors->get_current_draw_all()) {
        milliamps += static_cast<double>(value);
    }
    return milliamps / 1000.0;
}

void Toggle::update() {
    const std::uint32_t now = pros::millis();
    const double dt_seconds = loop_seconds(now, m_last_ms, kNominalLoopMs);
    m_last_ms = now;

    if (m_manual_mode) {
        update_manual(dt_seconds);
        return;
    }

    const double output = m_position_pid.update(current_position());
    m_motors->move(static_cast<std::int32_t>(std::lround(output)));
}

void Toggle::update_manual(double dt_seconds) {
    const double wanted = m_manual_demand * m_config.manual_speed_percent;
    m_ramped = rate_limit(
        m_ramped, wanted, ramp_step(m_config.ramp_percent_per_second, dt_seconds)
    );
    m_motors->move(static_cast<std::int32_t>(std::lround(m_ramped)));
}

bool Toggle::is_at_target() const {
    return m_position_pid.is_settled();
}

double Toggle::current_position() const {
    return m_motors->get_position();
}

}  // namespace vex_pid
