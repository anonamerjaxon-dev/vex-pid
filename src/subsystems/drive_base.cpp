#include "subsystems/drive_base.hpp"
#include <algorithm>
#include <cmath>

namespace vex_pid {

void DriveBase::initialize(const DriveConfig& config, pros::Imu* imu) {
    m_config = config;
    m_imu = imu;

    m_left = new pros::Motor_Group(
        std::vector<std::int8_t>(
            m_config.left_ports.begin(),
            m_config.left_ports.end()
        )
    );
    m_right = new pros::Motor_Group(
        std::vector<std::int8_t>(
            m_config.right_ports.begin(),
            m_config.right_ports.end()
        )
    );

    for (auto port : m_config.left_ports) {
        pros::Motor motor(port);
        motor.set_gearing(m_config.gearset);
        motor.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    }
    for (auto port : m_config.right_ports) {
        pros::Motor motor(port);
        motor.set_gearing(m_config.gearset);
        motor.set_brake_mode(pros::E_MOTOR_BRAKE_COAST);
    }

    m_straight_pid = PIDController(m_config.straight_pid);
    m_turn_pid = PIDController(m_config.turn_pid);
}

bool DriveBase::drive_straight(double inches) {
    m_mode = DriveMode::Straight;
    m_target_distance = inches;
    m_target_heading = current_heading();
    m_start_position = average_position();

    double target_ticks = inches_to_ticks(inches);
    m_straight_pid.set_target(target_ticks);
    m_straight_pid.reset();
    m_turn_pid.set_target(m_target_heading);
    m_turn_pid.reset();

    return is_at_target();
}

bool DriveBase::turn_to_heading(double heading_deg) {
    m_mode = DriveMode::Turn;
    m_target_heading = wrap_heading(heading_deg);

    m_turn_pid.set_target(m_target_heading);
    m_turn_pid.reset();

    return is_at_target();
}

void DriveBase::stop() {
    m_mode = DriveMode::Idle;
    m_left->move(0);
    m_right->move(0);
}

void DriveBase::update() {
    if (m_mode == DriveMode::Idle) {
        return;
    }

    if (m_mode == DriveMode::Straight) {
        double traveled = average_position() - m_start_position;
        double straight_output = m_straight_pid.update(traveled);

        double raw_heading = current_heading();
        double heading_error = raw_heading - m_target_heading;
        heading_error = wrap_heading(m_target_heading + heading_error) - m_target_heading;

        double turn_output = m_turn_pid.update(
            m_target_heading + heading_error
        );

        turn_output = std::clamp(
            turn_output * m_config.heading_correction_gain,
            -m_config.max_heading_correction,
             m_config.max_heading_correction
        );

        double left_power = straight_output + turn_output;
        double right_power = straight_output - turn_output;
        double max_abs = std::max(std::fabs(left_power), std::fabs(right_power));
        if (max_abs > 127.0) {
            left_power = left_power * 127.0 / max_abs;
            right_power = right_power * 127.0 / max_abs;
        }

        m_left->move(left_power);
        m_right->move(right_power);
    } else if (m_mode == DriveMode::Turn) {
        double raw_heading = current_heading();
        double adjusted = raw_heading;

        while (adjusted - m_target_heading > 180.0) {
            adjusted -= 360.0;
        }
        while (adjusted - m_target_heading < -180.0) {
            adjusted += 360.0;
        }

        double turn_output = m_turn_pid.update(adjusted);

        m_left->move(turn_output);
        m_right->move(-turn_output);
    }
}

bool DriveBase::is_at_target() const {
    if (m_mode == DriveMode::Straight) {
        return m_straight_pid.is_settled();
    }
    if (m_mode == DriveMode::Turn) {
        return m_turn_pid.is_settled();
    }
    return true;
}

double DriveBase::current_heading() const {
    if (m_imu) {
        return m_imu->get_heading();
    }
    return 0.0;
}

double DriveBase::average_position() const {
    double sum = 0.0;
    int count = 0;
    for (auto port : m_config.left_ports) {
        sum += std::fabs(pros::Motor(port).get_position());
        count++;
    }
    for (auto port : m_config.right_ports) {
        sum += std::fabs(pros::Motor(port).get_position());
        count++;
    }
    return count > 0 ? sum / static_cast<double>(count) : 0.0;
}

double DriveBase::average_velocity() const {
    double sum = 0.0;
    int count = 0;
    for (auto port : m_config.left_ports) {
        sum += pros::Motor(port).get_actual_velocity();
        count++;
    }
    for (auto port : m_config.right_ports) {
        sum += pros::Motor(port).get_actual_velocity();
        count++;
    }
    return count > 0 ? sum / static_cast<double>(count) : 0.0;
}

void DriveBase::manual_control(double throttle, double turn) {
    m_mode = DriveMode::Idle;

    double left_power = throttle + turn;
    double right_power = throttle - turn;

    double max_abs = std::max(std::fabs(left_power), std::fabs(right_power));
    if (max_abs > 127.0) {
        left_power = left_power * 127.0 / max_abs;
        right_power = right_power * 127.0 / max_abs;
    }

    m_left->move(left_power);
    m_right->move(right_power);
}

double DriveBase::inches_to_ticks(double inches) const {
    double wheel_circumference = m_config.wheel_diameter_in * M_PI;
    double wheel_revolutions = inches / wheel_circumference;
    return wheel_revolutions * 360.0 * m_config.gear_ratio;
}

double DriveBase::ticks_to_inches(double ticks) const {
    double wheel_circumference = m_config.wheel_diameter_in * M_PI;
    double wheel_revolutions = ticks / (360.0 * m_config.gear_ratio);
    return wheel_revolutions * wheel_circumference;
}

double DriveBase::wrap_heading(double heading) const {
    double result = std::fmod(heading, 360.0);
    if (result < 0.0) {
        result += 360.0;
    }
    return result;
}

}  // namespace vex_pid