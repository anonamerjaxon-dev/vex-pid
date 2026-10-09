#include "data_logger.hpp"
#include "subsystems/drive_base.hpp"
#include "subsystems/cascade.hpp"
#include "subsystems/claw.hpp"
#include "subsystems/toggle.hpp"
#include "pros/rtos.hpp"
#include <cstdarg>

namespace vex_pid {

void DataLogger::initialize(
    const LoggerConfig& config,
    DriveBase* drive,
    Cascade* cascade,
    Claw* claw,
    Toggle* toggle,
    pros::Imu* imu
) {
    m_config = config;
    m_drive = drive;
    m_cascade = cascade;
    m_claw = claw;
    m_toggle = toggle;
    m_imu = imu;
}

static void write_line(FILE* f, const char* fmt, ...) {
    if (!f) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(f, fmt, args);
    va_end(args);
    fputs("\n", f);
}

void DataLogger::write_header() {
    if (!m_file) return;

    write_line(m_file,
        "timestamp_ms,drive_pos_avg,drive_vel_avg,drive_current_max,"
        "cascade_pos,cascade_current,claw_current,toggle_pos,"
        "imu_heading,imu_accel_x,imu_accel_y,imu_gyro_z,"
        "left_motor_temp,battery_mv"
    );
    m_header_written = true;
}

void DataLogger::log_sample(std::uint32_t timestamp_ms) {
    if (!m_active || !m_file) return;

    if (!m_header_written) {
        write_header();
    }

    double drive_pos = 0.0;
    double drive_vel = 0.0;
    double drive_current = 0.0;
    if (m_drive) {
        drive_pos = m_drive->average_position();
        drive_vel = m_drive->average_velocity();

        auto& left = m_drive->left();
        auto& right = m_drive->right();

        (void)right;
        for (auto port : {static_cast<std::int8_t>(left.get_port(0)),
                          static_cast<std::int8_t>(left.get_port(1))}) {
            double c = pros::Motor(port).get_current_draw();
            if (c > drive_current) drive_current = c;
        }
    }

    double cascade_pos = 0.0;
    double cascade_current = 0.0;
    if (m_cascade) {
        cascade_pos = m_cascade->current_extension();
        for (auto port : m_cascade->config().motor_ports) {
            double c = pros::Motor(port).get_current_draw();
            if (c > cascade_current) cascade_current = c;
        }
    }

    double claw_current = 0.0;
    if (m_claw) {
        claw_current = static_cast<double>(m_claw->current_draw_ma());
    }

    double toggle_pos = 0.0;
    if (m_toggle) {
        toggle_pos = m_toggle->current_position();
    }

    double imu_heading = 0.0;
    double imu_accel_x = 0.0;
    double imu_accel_y = 0.0;
    double imu_gyro_z = 0.0;
    if (m_imu) {
        imu_heading = m_imu->get_heading();
        imu_accel_x = m_imu->get_accel().x;
        imu_accel_y = m_imu->get_accel().y;
        imu_gyro_z = m_imu->get_gyro_rate().z;
    }

    double left_temp = 0.0;
    double battery_mv = 0.0;
    if (m_drive) {
        auto& left = m_drive->left();
        left_temp = pros::Motor(left.get_port(0)).get_temperature();
        battery_mv = pros::Motor(left.get_port(0)).get_voltage();
    }

    fprintf(m_file,
        "%lu,%.2f,%.2f,%.1f,"
        "%.2f,%.1f,%.1f,%.2f,"
        "%.2f,%.3f,%.3f,%.3f,"
        "%.1f,%.1f\n",
        static_cast<unsigned long>(timestamp_ms),
        drive_pos, drive_vel, drive_current,
        cascade_pos, cascade_current, claw_current, toggle_pos,
        imu_heading, imu_accel_x, imu_accel_y, imu_gyro_z,
        left_temp, battery_mv
    );
}

void DataLogger::start_session() {
    m_file = fopen(m_config.filename, "w");
    if (m_file) {
        m_active = true;
        m_header_written = false;
    }
}

void DataLogger::stop_session() {
    m_active = false;
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }
}

}  // namespace vex_pid