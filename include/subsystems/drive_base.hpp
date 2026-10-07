#pragma once
#include "pid.hpp"
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

    double wrap_heading(double heading) const;
};

}  // namespace vex_pid