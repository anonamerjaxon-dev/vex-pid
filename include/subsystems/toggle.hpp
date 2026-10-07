#pragma once
#include "pid.hpp"
#include "pros/motors.hpp"
#include <vector>
#include <cstdint>

namespace vex_pid {

enum class ToggleState {
    Yellow,
    Red,
    Blue,
};

struct ToggleConfig {
    std::vector<std::int8_t> motor_ports;

    PIDGains position_pid = {
        .kP = 2.0,
        .kI = 0.005,
        .kD = 0.5,
        .kF = 0.0,
        .integral_limit = 30.0,
        .output_min = -127.0,
        .output_max = 127.0,
        .settle_error = 2.0,
        .settle_ticks = 10
    };

    double yellow_angle = 0.0;
    double red_angle = 90.0;
    double blue_angle = -90.0;
};

class Toggle {
public:
    Toggle() = default;

    void initialize(const ToggleConfig& config);

    bool flip_to(ToggleState state);
    bool flip_to_yellow();
    bool flip_to_red();
    bool flip_to_blue();
    void stop();

    void update();

    bool is_at_target() const;
    ToggleState target_state() const { return m_target_state; }
    double current_position() const;

private:
    ToggleConfig m_config;
    pros::Motor_Group* m_motors = nullptr;
    PIDController m_position_pid;

    ToggleState m_target_state = ToggleState::Yellow;
    double m_target_angle = 0.0;
};

}  // namespace vex_pid