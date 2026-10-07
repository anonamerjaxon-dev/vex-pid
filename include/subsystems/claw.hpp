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

    bool m_is_open = true;
    bool m_busy = false;
    int m_timer = 0;
    bool m_stall_check_active = false;
};

}  // namespace vex_pid