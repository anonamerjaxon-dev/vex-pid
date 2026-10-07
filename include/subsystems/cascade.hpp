#pragma once
#include "pid.hpp"
#include "pros/motors.hpp"
#include <vector>
#include <cstdint>

namespace vex_pid {

struct CascadeConfig {
    std::vector<std::int8_t> motor_ports;

    pros::motor_gearset_e gearset = pros::E_MOTOR_GEARSET_36;

    double max_extension_inches = 24.0;

    double inches_per_encoder_degree = 0.01;

    PIDGains position_pid = {
        .kP = 2.0,
        .kI = 0.005,
        .kD = 0.6,
        .kF = 0.0,
        .integral_limit = 30.0,
        .output_min = -127.0,
        .output_max = 127.0,
        .settle_error = 1.0,
        .settle_ticks = 15
    };

    double gravity_feedforward = 8.0;

    double presets[4] = {0.0, 6.0, 14.0, 22.0};
};

class Cascade {
public:
    Cascade() = default;

    void initialize(const CascadeConfig& config);

    bool move_to(double inches);
    bool move_to_preset(int index);
    void stop();

    void update();

    bool is_at_target() const;
    double current_extension() const;
    double target_extension() const { return m_target_inches; }

    void manual_control(int power);
    void home();

    const CascadeConfig& config() const { return m_config; }

private:
    double inches_to_ticks(double inches) const;
    double ticks_to_inches(double ticks) const;

    CascadeConfig m_config;
    pros::Motor_Group* m_motors = nullptr;
    PIDController m_position_pid;

    double m_target_inches = 0.0;
    bool m_manual_mode = false;
};

}  // namespace vex_pid