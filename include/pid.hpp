#pragma once
#include <algorithm>
#include <cmath>

namespace vex_pid {

struct PIDGains {
    double kP = 0.0;
    double kI = 0.0;
    double kD = 0.0;
    double kF = 0.0;

    double integral_limit = 0.0;
    double output_min = -127.0;
    double output_max = 127.0;

    double settle_error = 0.0;
    int settle_ticks = 15;
};

class PIDController {
public:
    PIDController() = default;

    explicit PIDController(const PIDGains& gains)
        : m_gains(gains) {}

    void set_gains(const PIDGains& gains) {
        m_gains = gains;
        reset();
    }

    const PIDGains& gains() const { return m_gains; }

    void set_target(double target) {
        m_target = target;
    }

    double target() const { return m_target; }

    void reset() {
        m_integral = 0.0;
        m_prev_error = 0.0;
        m_settle_timer = 0;
    }

    double update(double current) {
        double error = m_target - current;

        double p_term = m_gains.kP * error;

        m_integral += error;
        if (m_gains.integral_limit > 0.0) {
            m_integral = std::clamp(m_integral,
                                    -m_gains.integral_limit,
                                     m_gains.integral_limit);
        }
        double i_term = m_gains.kI * m_integral;

        double derivative = error - m_prev_error;
        m_prev_error = error;
        double d_term = m_gains.kD * derivative;

        double f_term = m_gains.kF * m_target;

        double output = p_term + i_term + d_term + f_term;
        output = std::clamp(output, m_gains.output_min, m_gains.output_max);

        if (std::fabs(error) < m_gains.settle_error) {
            m_settle_timer++;
        } else {
            m_settle_timer = 0;
        }

        return output;
    }

    bool is_settled() const {
        return m_settle_timer >= m_gains.settle_ticks;
    }

    double error() const {
        return m_prev_error;
    }

private:
    PIDGains m_gains;
    double m_target = 0.0;
    double m_integral = 0.0;
    double m_prev_error = 0.0;
    int m_settle_timer = 0;
};

}  // namespace vex_pid