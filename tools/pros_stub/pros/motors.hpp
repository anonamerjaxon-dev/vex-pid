#pragma once
//
// A STAND-IN for pros/motors.hpp, used only when the real PROS headers are not
// available (see tools/get_pros_headers.sh and tools/syntax_check.sh).
//
// ---------------------------------------------------------------------------
// READ THIS BEFORE TRUSTING IT.
//
// The first version of this file was written by looking at the project's own
// code and declaring whatever it appeared to use. That made the syntax check
// circular: it could never catch a wrong API name, because the stub had been
// built from the same mistake. It declared `class Motor_Group` and a
// `move(std::int8_t)`, neither of which exists in PROS - and so it happily
// "checked" a project that would not have compiled on the brain.
//
// Every declaration below is now copied from the real PROS 4.1.0 headers
// (include/pros/motors.hpp and include/pros/motor_group.hpp). The real check is
// tools/syntax_check.sh with the real headers; this is the fallback.
// ---------------------------------------------------------------------------
//
#include <cstdint>
#include <vector>

namespace pros {

enum motor_gearset_e {
    E_MOTOR_GEARSET_36 = 0,
    E_MOTOR_GEARSET_18 = 1,
    E_MOTOR_GEARSET_06 = 2,
    E_MOTOR_GEARSET_INVALID = 3
};
typedef motor_gearset_e motor_gearset_e_t;

enum motor_brake_mode_e {
    E_MOTOR_BRAKE_INVALID = -1,
    E_MOTOR_BRAKE_COAST = 0,
    E_MOTOR_BRAKE_BRAKE = 1,
    E_MOTOR_BRAKE_HOLD = 2
};
typedef motor_brake_mode_e motor_brake_mode_e_t;

enum motor_encoder_units_e {
    E_MOTOR_ENCODER_DEGREES = 0,
    E_MOTOR_ENCODER_ROTATIONS = 1,
    E_MOTOR_ENCODER_COUNTS = 2
};
typedef motor_encoder_units_e motor_encoder_units_e_t;

class Motor {
public:
    explicit Motor(
        const std::int8_t port,
        const motor_gearset_e_t gearset = E_MOTOR_GEARSET_INVALID,
        const motor_encoder_units_e_t encoder_units = E_MOTOR_ENCODER_DEGREES
    );

    std::int32_t move(std::int32_t voltage) const;
    std::int32_t move_voltage(std::int32_t voltage) const;
    std::int32_t brake() const;

    std::int32_t set_current_limit(std::int32_t limit, std::uint8_t index = 0) const;
    std::int32_t set_current_limit_all(std::int32_t limit) const;
    std::int32_t get_current_draw(std::uint8_t index = 0) const;
    std::vector<std::int32_t> get_current_draw_all() const;

    double get_position(std::uint8_t index = 0) const;
    std::int32_t tare_position(std::uint8_t index = 0) const;
    double get_actual_velocity(std::uint8_t index = 0) const;
    std::int32_t get_voltage(std::uint8_t index = 0) const;
    std::int8_t get_port(std::uint8_t index = 0) const;
    double get_temperature(std::uint8_t index = 0) const;
    std::int32_t is_over_current(std::uint8_t index = 0) const;

    std::int32_t set_gearing(motor_gearset_e_t gearset, std::uint8_t index = 0) const;
    std::int32_t set_brake_mode(motor_brake_mode_e_t mode, std::uint8_t index = 0) const;
};

class MotorGroup {
public:
    MotorGroup(
        const std::vector<std::int8_t>& ports,
        const motor_gearset_e_t gearset = E_MOTOR_GEARSET_INVALID,
        const motor_encoder_units_e_t encoder_units = E_MOTOR_ENCODER_DEGREES
    );

    std::int32_t move(std::int32_t voltage) const;
    std::int32_t move_voltage(std::int32_t voltage) const;
    std::int32_t brake() const;

    std::uint8_t size() const;

    std::int32_t set_current_limit(std::int32_t limit, std::uint8_t index = 0) const;
    std::int32_t set_current_limit_all(std::int32_t limit) const;
    std::int32_t get_current_draw(std::uint8_t index = 0) const;
    std::vector<std::int32_t> get_current_draw_all() const;

    double get_position(std::uint8_t index = 0) const;
    std::int32_t tare_position(std::uint8_t index = 0) const;
    double get_actual_velocity(std::uint8_t index = 0) const;
    std::int32_t set_gearing(motor_gearset_e_t gearset, std::uint8_t index = 0) const;
    std::int32_t set_brake_mode(motor_brake_mode_e_t mode, std::uint8_t index = 0) const;
    std::int8_t get_port(std::uint8_t index = 0) const;
    std::vector<std::int8_t> get_port_all() const;
};

inline double to_degrees(double v) { return v; }

}  // namespace pros
