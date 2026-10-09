#pragma once
#include "pid.hpp"
#include "strain_guard.hpp"
#include "pros/motor_group.hpp"  // Motor, MotorGroup, gearsets and brake modes
#include <vector>
#include <cstdint>

namespace vex_pid {

struct CascadeConfig {
    std::vector<std::int8_t> motor_ports;

    pros::motor_gearset_e gearset = pros::E_MOTOR_GEARSET_36;

    double max_extension_inches = 24.0;

    double inches_per_encoder_degree = 0.01;

    // The free speed of the motor for its cartridge: 100 RPM for red 36:1,
    // 200 for green 18:1, 600 for blue 6:1. Only the driver's feel and the
    // software floor use it - it is how a command written as a percentage is
    // turned back into "how far will the arm have travelled by the next pass".
    double motor_max_rpm = 100.0;

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

    // --- the driver's feel, and the software limits -------------------------

    // The most a held button may ask for. 40% is the ceiling from the Python
    // drive program, and the same number the amps tool measures at.
    double manual_speed_percent = 40.0;

    // Within `creep_band_inches` of the floor the arm is asked for this instead,
    // so it can settle onto the limit rather than stopping a full-speed step
    // above it.
    double creep_speed_percent = 10.0;
    double creep_band_inches = 0.25;

    // How fast a held button may wind the arm up. One step per pass, measured
    // from the clock rather than assumed, so a slow loop cannot outrun it.
    double ramp_percent_per_second = 250.0;

    // Where the bottom of the travel is, in inches, measured from where the arm
    // was tared at start-up. There is no physical stopper: driven past this the
    // chain can come off the sprocket, so the software refuses to go below it.
    double floor_inches = -0.02;

    // Assume the arm travels this much further than the arithmetic says. The
    // program reads the position, then commands, then waits, and the arm keeps
    // moving through all of it - so the estimate of where it will be next is
    // deliberately pessimistic.
    double safety_factor = 1.5;

    StrainGuardSettings guard;
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

    // The driver's command: +1 is all the way up, -1 all the way down, 0 is
    // "let go". Only the sign and the size of the demand matter; the speed
    // comes from the config. Nothing moves here - update() does the ramping,
    // the easing and the floor, so there is exactly one place that talks to the
    // motors.
    void manual_control(double demand);

    void home();

    // True while the arm is being driven by hand (a button held) rather than by
    // the position PID. The driver has to ask, because update() runs the manual
    // path instead of the PID while this is set.
    bool in_manual_mode() const { return m_manual_mode; }

    // What the guard can see, for the brain screen and the log.
    double current_draw_amps() const;
    double measured_speed() const;
    double guard_factor() const { return m_guard.factor(); }
    bool is_straining() const { return m_guard.straining(); }
    bool is_blocked() const { return m_guard.blocked(); }

    const CascadeConfig& config() const { return m_config; }

private:
    double inches_to_ticks(double inches) const;
    double ticks_to_inches(double ticks) const;

    // How far the arm travels in one second at full command, in inches.
    double travel_per_second() const;

    void update_manual(double dt_seconds);
    void apply_current_limit();

    CascadeConfig m_config;
    pros::MotorGroup* m_motors = nullptr;
    PIDController m_position_pid;

    double m_target_inches = 0.0;
    bool m_manual_mode = false;

    // The manual path's own state.
    double m_manual_demand = 0.0;  // -1..1, what the driver asked for
    double m_ramped = 0.0;         // the command after ramping and easing
    std::uint32_t m_last_ms = 0;   // when update() last ran

    StrainGuard m_guard;
};

}  // namespace vex_pid
