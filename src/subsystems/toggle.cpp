#include "subsystems/toggle.hpp"
#include <cmath>

namespace vex_pid {

void Toggle::initialize(const ToggleConfig& config) {
    m_config = config;

    m_motors = new pros::Motor_Group(
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
}

bool Toggle::flip_to(ToggleState state) {
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
    m_motors->move(0);
}

void Toggle::update() {
    double current = current_position();
    double output = m_position_pid.update(current);
    m_motors->move(output);
}

bool Toggle::is_at_target() const {
    return m_position_pid.is_settled();
}

double Toggle::current_position() const {
    return m_motors->get_position();
}

}  // namespace vex_pid