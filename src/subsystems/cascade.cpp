#include "subsystems/cascade.hpp"
#include <algorithm>
#include <cmath>

namespace vex_pid {

void Cascade::initialize(const CascadeConfig& config) {
    m_config = config;

    m_motors = new pros::Motor_Group(
        std::vector<std::int8_t>(
            m_config.motor_ports.begin(),
            m_config.motor_ports.end()
        )
    );

    for (auto port : m_config.motor_ports) {
        pros::Motor motor(port);
        motor.set_gearing(m_config.gearset);
        motor.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    }

    m_position_pid = PIDController(m_config.position_pid);
    m_target_inches = 0.0;
}

bool Cascade::move_to(double inches) {
    m_manual_mode = false;

    double clamped = std::clamp(inches, 0.0, m_config.max_extension_inches);
    m_target_inches = clamped;

    m_position_pid.set_target(inches_to_ticks(clamped));
    m_position_pid.reset();

    return is_at_target();
}

bool Cascade::move_to_preset(int index) {
    if (index < 0 || index >= 4) {
        return false;
    }
    return move_to(m_config.presets[index]);
}

void Cascade::stop() {
    m_manual_mode = false;
    m_motors->move(0);
    m_target_inches = current_extension();
}

void Cascade::update() {
    if (m_manual_mode) {
        return;
    }

    double current = current_extension();
    double pid_output = m_position_pid.update(inches_to_ticks(current));

    if (m_config.gravity_feedforward > 0.0) {
        pid_output += m_config.gravity_feedforward;
    }

    m_motors->move(pid_output);
}

bool Cascade::is_at_target() const {
    return m_position_pid.is_settled();
}

double Cascade::current_extension() const {
    return ticks_to_inches(m_motors->get_position());
}

void Cascade::manual_control(int power) {
    m_manual_mode = true;

    double current = current_extension();
    if (current <= 0.0 && power < 0) {
        power = 0;
    }
    if (current >= m_config.max_extension_inches && power > 0) {
        power = 0;
    }

    m_motors->move(power);
}

void Cascade::home() {
    m_manual_mode = true;

    m_motors->move(-30);

    int stall_time = 0;
    const int stall_threshold_ms = 300;
    const int check_interval_ms = 20;

    while (stall_time < stall_threshold_ms) {
        pros::delay(check_interval_ms);

        double max_current = 0.0;
        for (auto port : m_config.motor_ports) {
            double c = pros::Motor(port).get_current_draw();
            if (c > max_current) {
                max_current = c;
            }
        }

        if (max_current > 1500.0) {
            stall_time += check_interval_ms;
        } else {
            stall_time = 0;
        }
    }

    m_motors->move(0);
    m_motors->tare_position();

    m_target_inches = 0.0;
    m_position_pid.set_target(0.0);
    m_position_pid.reset();
    m_manual_mode = false;
}

double Cascade::inches_to_ticks(double inches) const {
    return inches / m_config.inches_per_encoder_degree;
}

double Cascade::ticks_to_inches(double ticks) const {
    return ticks * m_config.inches_per_encoder_degree;
}

}  // namespace vex_pid