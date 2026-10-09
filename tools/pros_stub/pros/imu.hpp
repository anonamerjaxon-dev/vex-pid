#pragma once
#include <cstdint>
namespace pros {
struct imu_accel_s_t { double x = 0.0, y = 0.0, z = 0.0; };
struct imu_gyro_s_t { double x = 0.0, y = 0.0, z = 0.0; };
enum imu_status_e_t { E_IMU_STATUS_CALIBRATING = 1, E_IMU_STATUS_ERROR = 2 };
class Imu {
public:
    explicit Imu(std::int8_t port = 0);
    std::int32_t reset(bool wait_for_calibration = true);
    double get_heading() const;
    double get_rotation() const;
    double get_pitch() const;
    double get_roll() const;
    double get_yaw() const;
    imu_accel_s_t get_accel() const;
    imu_gyro_s_t get_gyro_rate() const;
    std::int32_t get_status() const;
    bool is_calibrating() const;
    std::int32_t tare_heading();
    std::int32_t tare_rotation();
};
}  // namespace pros
