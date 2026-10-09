#pragma once
#include "pros/imu.hpp"
#include "pros/motors.hpp"
#include <cstdio>
#include <cstdint>

namespace vex_pid {

class DriveBase;
class Cascade;
class Claw;
class Toggle;

struct LoggerConfig {
    const char* filename = "/usd/vex_log.csv";
    int log_rate_hz = 50;
};

class DataLogger {
public:
    DataLogger() = default;

    void initialize(
        const LoggerConfig& config,
        DriveBase* drive,
        Cascade* cascade,
        Claw* claw,
        Toggle* toggle,
        pros::Imu* imu
    );

    void log_sample(std::uint32_t timestamp_ms);

    void start_session();
    void stop_session();

    bool is_active() const { return m_active; }

private:
    LoggerConfig m_config;
    DriveBase* m_drive = nullptr;
    Cascade* m_cascade = nullptr;
    Claw* m_claw = nullptr;
    Toggle* m_toggle = nullptr;
    pros::Imu* m_imu = nullptr;

    FILE* m_file = nullptr;
    bool m_active = false;
    bool m_header_written = false;

    void write_header();
};

}  // namespace vex_pid