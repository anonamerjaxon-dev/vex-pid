#pragma once
#include "pid.hpp"
#include "pros/motor_group.hpp"  // Motor, MotorGroup, gearsets and brake modes
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

    // --- the driver's hold buttons ------------------------------------------
    // The toggle is the one mechanism that does not get the stick curve: it is
    // a "go that way" button, not a speed control, so a straight percentage
    // with a ramp is easier to aim. It also has its own hard stops at the end
    // of its travel, so it needs no strain guard and no software floor.
    double manual_speed_percent = 40.0;

    // Still ramped, so a tapped bumper does not slam it from rest to full.
    double ramp_percent_per_second = 250.0;
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

    // The driver's command: +1 one way, -1 the other, 0 is "let go". Which
    // direction is which is the driver's business - the mechanism is
    // symmetrical. Held, not latched: letting go stops asking for movement.
    void manual_control(double demand);

    bool in_manual_mode() const { return m_manual_mode; }

    bool is_at_target() const;
    ToggleState target_state() const { return m_target_state; }
    double current_position() const;

private:
    void update_manual(double dt_seconds);

    ToggleConfig m_config;
    pros::MotorGroup* m_motors = nullptr;
    PIDController m_position_pid;

    ToggleState m_target_state = ToggleState::Yellow;
    double m_target_angle = 0.0;

    bool m_manual_mode = false;
    double m_manual_demand = 0.0;
    double m_ramped = 0.0;
    std::uint32_t m_last_ms = 0;
};

}  // namespace vex_pid
