#pragma once
#include <cstdint>
#include <vector>
namespace pros {
enum motor_gearset_e { E_MOTOR_GEARSET_36 = 0, E_MOTOR_GEARSET_18 = 1, E_MOTOR_GEARSET_06 = 2, E_MOTOR_GEARSET_INVALID = 3 };
enum motor_brake_mode_e_t { E_MOTOR_BRAKE_INVALID = -1, E_MOTOR_BRAKE_COAST = 0, E_MOTOR_BRAKE_BRAKE = 1, E_MOTOR_BRAKE_HOLD = 2 };
enum motor_encoder_units_e_t { E_MOTOR_ENCODER_DEGREES = 0, E_MOTOR_ENCODER_ROTATIONS = 1, E_MOTOR_ENCODER_COUNTS = 2 };
inline double to_degrees(double v) { return v; }
class Motor {
public:
    explicit Motor(std::int8_t port, motor_gearset_e gearset = E_MOTOR_GEARSET_18, bool reversed = false);
    void set_gearing(motor_gearset_e gearset);
    void set_brake_mode(motor_brake_mode_e_t mode);
    void move(std::int8_t voltage);
    void move_voltage(std::int32_t voltage);
    void tare_position();
    double get_position(motor_encoder_units_e_t units = E_MOTOR_ENCODER_DEGREES) const;
    double get_current_draw() const;
    double get_temperature() const;
    double get_actual_velocity() const;
    std::int32_t get_voltage() const;
    std::int8_t get_port() const;
    void set_max_torque(std::int32_t value, std::int32_t units);
};
class Motor_Group {
public:
    explicit Motor_Group(const std::vector<std::int8_t>& ports);
    void move(std::int8_t voltage);
    void move_voltage(std::int32_t voltage);
    void tare_position();
    double get_position(motor_encoder_units_e_t units = E_MOTOR_ENCODER_DEGREES) const;
    std::int8_t get_port(int index) const;
    std::int32_t size() const;
    void set_gearing(motor_gearset_e gearset);
    void set_brake_mode(motor_brake_mode_e_t mode);
};
}  // namespace pros
