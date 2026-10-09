#pragma once
#include "strain_guard.hpp"
#include "pros/motors.hpp"
#include <cstdint>

namespace vex_pid {

struct ClawConfig {
    std::int8_t motor_port = 0;

    pros::motor_gearset_e gearset = pros::E_MOTOR_GEARSET_18;

    // --- the autonomous actions ---------------------------------------------
    // open()/close() are one-shot: they drive until they are done and stop.
    // The driver's held buttons use the manual path further down instead.
    int open_power = 127;
    int close_power = 127;
    int hold_power = 15;

    int open_time_ms = 400;

    int stall_current_ma = 1200;
    int stall_check_after_ms = 200;

    // How long to keep squeezing before giving up and holding, if the stall
    // current never appears. Without an upper bound a claw closed on something
    // soft (or on nothing, with a slipping gear) would push for ever.
    int close_timeout_ms = 2000;

    // --- the driver's feel ---------------------------------------------------
    // The most a held button may ask for. A claw does not need to slam shut.
    double manual_speed_percent = 40.0;

    // Ramped like everything else, so the claw cannot jump straight to full
    // speed the instant a bumper is touched.
    double ramp_percent_per_second = 250.0;

    // The free speed of the motor for its cartridge (200 RPM for green 18:1).
    // Used to turn the measured speed into a fraction of the command.
    double motor_max_rpm = 200.0;

    // A claw closed on a game object is a motor that has been told to turn and
    // has stopped. That is not a fault - it is the grip - so the guard is used
    // to hold the squeeze down to a current the motor can sit at for as long as
    // the match lasts, rather than to back the claw off.
    StrainGuardSettings guard;
};

class Claw {
public:
    Claw() = default;

    void initialize(const ClawConfig& config);

    // The autonomous actions. Both set the manual path off, so a held button
    // and an autonomous action can never fight over the motor.
    void open();
    void close();
    void stop();

    void update();

    // The driver's command: +1 closes, -1 opens, 0 is "let go". As with the
    // cascade, nothing moves here - update() does the ramping, the easing and
    // the current limit.
    void manual_control(double demand);

    bool in_manual_mode() const { return m_manual_mode; }

    bool is_open() const { return m_is_open; }
    bool is_busy() const { return m_busy; }
    int current_draw_ma() const;

    double current_draw_amps() const;
    double measured_speed() const;
    double guard_factor() const { return m_guard.factor(); }
    bool is_straining() const { return m_guard.straining(); }
    bool is_blocked() const { return m_guard.blocked(); }

private:
    void update_manual(double dt_seconds);
    void apply_current_limit();

    ClawConfig m_config;
    pros::Motor* m_motor = nullptr;

    // True when the claw is OPEN. open()/close() set it to say which action is
    // under way, update() uses it to pick the branch, and the driver toggles
    // on it. It therefore must not be flipped when an action finishes: if
    // update() were to flip it, the claw would report the opposite of where it
    // really is and the next press would run the wrong way.
    bool m_is_open = true;
    bool m_busy = false;

    // When the current action started, in pros::millis(). This used to be a
    // count of update() calls multiplied by 10, which is only the same thing
    // if the loop always runs at exactly 100 Hz - and it does not.
    std::uint32_t m_start_ms = 0;

    bool m_stall_check_active = false;

    // The manual path's own state.
    bool m_manual_mode = false;
    double m_manual_demand = 0.0;  // -1..1, what the driver asked for
    double m_ramped = 0.0;         // the command after ramping and easing
    std::uint32_t m_last_ms = 0;   // when update() last ran

    StrainGuard m_guard;
};

}  // namespace vex_pid
