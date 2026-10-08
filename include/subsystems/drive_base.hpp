#pragma once
#include "pid.hpp"
#include "control_feel.hpp"
#include "pros/imu.hpp"
#include "pros/motors.hpp"
#include <vector>
#include <cstdint>

namespace vex_pid {

enum class DriveMode {
    Idle,
    Straight,
    Turn,
};

struct DriveConfig {
    std::vector<std::int8_t> left_ports;
    std::vector<std::int8_t> right_ports;

    double wheel_diameter_in = 3.25;
    double gear_ratio = 60.0 / 18.0;

    pros::motor_gearset_e gearset = pros::E_MOTOR_GEARSET_18;

    PIDGains straight_pid = {
        .kP = 0.8,
        .kI = 0.002,
        .kD = 0.4,
        .kF = 0.0,
        .integral_limit = 40.0,
        .output_min = -127.0,
        .output_max = 127.0,
        .settle_error = 1.0,
        .settle_ticks = 15
    };

    PIDGains turn_pid = {
        .kP = 2.0,
        .kI = 0.005,
        .kD = 0.8,
        .kF = 0.0,
        .integral_limit = 30.0,
        .output_min = -127.0,
        .output_max = 127.0,
        .settle_error = 2.0,
        .settle_ticks = 15
    };

    double heading_correction_gain = 0.4;
    double max_heading_correction = 40.0;

    // ---- The driver's stick feel (see control_feel.hpp) -------------------
    // Ported from the Python drive program, in the -127..127 units the motors
    // take. 51 is 40% of 127, which is where the Python program sits.
    double drive_speed_max = 51.0;   // the most either wheel is ever asked for
    double turn_speed_max = 51.0;    // the most the turn stick is worth
    StickSettings stick;
    double ramp_percent_per_second = 250.0;   // how fast a command may change
};

class DriveBase {
public:
    DriveBase() = default;

    void initialize(const DriveConfig& config, pros::Imu* imu);

    bool drive_straight(double inches);
    bool turn_to_heading(double heading_deg);
    void stop();

    void update();

    bool is_at_target() const;
    DriveMode mode() const { return m_mode; }

    double current_heading() const;
    double average_position() const;
    double average_velocity() const;

    pros::Motor_Group& left() { return *m_left; }
    pros::Motor_Group& right() { return *m_right; }

    // Raw stick values in -127..127. The stick curve, the split arcade mix,
    // the ceiling and the ramp all happen in here; callers pass the sticks
    // straight through.
    void manual_control(double throttle, double turn);

private:
    double inches_to_ticks(double inches) const;
    double ticks_to_inches(double ticks) const;

    DriveConfig m_config;
    DriveMode m_mode = DriveMode::Idle;

    pros::Motor_Group* m_left = nullptr;
    pros::Motor_Group* m_right = nullptr;
    pros::Imu* m_imu = nullptr;

    PIDController m_straight_pid;
    PIDController m_turn_pid;

    double m_target_distance = 0.0;
    double m_target_heading = 0.0;
    double m_start_position = 0.0;

    // Manual drive state. The ramp lives in here rather than in the caller, so
    // the stick feel cannot be lost by calling manual_control some other way.
    double m_ramped_left = 0.0;
    double m_ramped_right = 0.0;
    std::uint32_t m_last_manual_ms = 0;

    double wrap_heading(double heading) const;
};

}  // namespace vex_pid