#pragma once
#include "subsystems/drive_base.hpp"
#include "subsystems/cascade.hpp"
#include "subsystems/claw.hpp"
#include "subsystems/toggle.hpp"
#include "data_logger.hpp"
#include "pros/imu.hpp"
#include <cstdint>

namespace vex_pid {

enum class AutonState {
    Idle,
    WaitForImu,
    DriveToGoal1,
    ScorePreload,
    DriveToPin,
    GrabPin,
    DriveToGoal2,
    ScorePin,
    DriveToMidfield,
    Done,
};

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
    void auton_tick();
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

    AutonState auton_state() const { return m_auton_state; }
    void set_auton_routine(int index) { m_auton_routine = index; }

private:
    void start_auton();
    void run_auton_state();

    // The full stop button. stop_button_held() just reads the controller;
    // stop_requested() also remembers that it has been pressed, so that the
    // PIDs (which run in subsystems_tick) can be held off.
    bool stop_button_held() const;
    bool stop_requested();

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

    AutonState m_auton_state = AutonState::Idle;
    int m_auton_routine = 0;
    std::uint32_t m_auton_start_ms = 0;
    std::uint32_t m_state_start_ms = 0;

    // True while the full stop button is held. Set by stop_requested(), and
    // read by subsystems_tick(), which is where the PIDs write the motors.
    bool m_estopped = false;

    int m_tick_counter = 0;
};

}  // namespace vex_pid