#pragma once
#include "subsystems/drive_base.hpp"
#include "subsystems/cascade.hpp"
#include "subsystems/claw.hpp"
#include "subsystems/toggle.hpp"
#include "data_logger.hpp"
#include "pros/imu.hpp"
#include <cstdint>

namespace vex_pid {

// There is deliberately no autonomous routine.
//
// This project is driver control only. `main.cpp` still defines the PROS entry
// points (the linker needs them), but `autonomous()` does nothing. What is left
// of the "autonomous" story is the subsystems' own positional moves -
// move_to_preset(), home(), claw open()/close() - which are kept because they
// are part of the mechanism APIs, not because anything here calls them.

struct RobotConfig {
    DriveConfig drive;
    CascadeConfig cascade;
    ClawConfig claw;
    ToggleConfig toggle;
    LoggerConfig logger;

    std::uint8_t imu_port = 0;
};

class Robot {
public:
    Robot() = default;

    void initialize(const RobotConfig& config);

    void subsystems_tick();
    void driver_tick();
    void disabled_tick();

    // Stops every subsystem at once. Everything the robot can move goes
    // through here, so there is exactly one definition of "stopped".
    void stop_all();

    DriveBase& drive() { return m_drive; }
    Cascade& cascade() { return m_cascade; }
    Claw& claw() { return m_claw; }
    Toggle& toggle() { return m_toggle; }
    DataLogger& logger() { return m_logger; }
    pros::Imu& imu() { return *m_imu; }
    bool imu_ready() const { return m_imu_ready; }

    // The amps measuring tool reads the port lists and the driver's own speeds
    // from here, so it never repeats a port and never picks its own speed.
    const RobotConfig& config() const { return m_config; }

private:
    // The full stop button. stop_requested() reads A as a toggle: it flips
    // m_estopped on each new press, stops everything on the way in and reports
    // whether the robot is stopped. subsystems_tick() holds the PIDs off while
    // it says so.
    bool stop_requested();
    void show_stop_screen();
    void clear_stop_screen();

    // The live current/ease readout along the top of the brain screen. It is
    // how the strain easing is watched on the real robot.
    void show_guard_readout();

    RobotConfig m_config;

    DriveBase m_drive;
    Cascade m_cascade;
    Claw m_claw;
    Toggle m_toggle;
    DataLogger m_logger;

    // Held by pointer, not by value.
    //
    // pros::Imu deletes its copy assignment operator, so a member `pros::Imu
    // m_imu;` cannot be assigned to after construction - and, worse, the
    // generated Robot constructor is deleted with it, so `Robot robot;` in
    // main.cpp would not even compile. A pointer sidesteps both: nothing is
    // copied, and the port (which only main.cpp knows) is passed to the
    // constructor at initialize() time.
    pros::Imu* m_imu = nullptr;

    bool m_imu_ready = false;
    int m_imu_start_ms = 0;
    const int kImuCalibrateMs = 2500;

    // True while the robot is stopped. A is a TOGGLE: one press stops every
    // motor and leaves the robot stopped even after the button is let go, and
    // the next press hands control back. Set by stop_requested(), and read by
    // subsystems_tick(), which is where the PIDs write the motors.
    bool m_estopped = false;

    // When the current/ease readout was last drawn, so it goes at a readable
    // five times a second rather than every loop.
    std::uint32_t m_last_readout_ms = 0;

    int m_tick_counter = 0;
};

}  // namespace vex_pid