#pragma once
#include "pros/motors.hpp"
#include <cstdint>

namespace vex_pid {

struct ClawConfig {
    std::int8_t motor_port = 0;

    pros::motor_gearset_e gearset = pros::E_MOTOR_GEARSET_18;

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
};

class Claw {
public:
    Claw() = default;

    void initialize(const ClawConfig& config);

    void open();
    void close();
    void stop();

    void update();

    bool is_open() const { return m_is_open; }
    bool is_busy() const { return m_busy; }
    int current_draw_ma() const;

private:
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
};

}  // namespace vex_pid