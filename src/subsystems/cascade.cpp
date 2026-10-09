#include "subsystems/cascade.hpp"
#include "pros/rtos.hpp"
#include <algorithm>
#include <cmath>

namespace vex_pid {

namespace {
// The arm must never be driven below where it started: there is no physical
// stopper, so if it is driven down past the bottom of its travel the chain can
// come off. This is the same guard the Python drivers use, where it is called
// CASCADE_LOWER_LIMIT = -2.0 degrees of encoder; at the default
// inches_per_encoder_degree of 0.01 that is 0.02 inches.
constexpr double kCascadeFloorInches = -0.02;

// A hard cap on how long Cascade::home() will drive the arm downwards. It is
// also bounded by the stall detector, but a slipping chain or a motor that
// reads no current would never stall and the loop would never end.
constexpr std::uint32_t kHomeTimeoutMs = 2000;
}  // namespace

void Cascade::initialize(const CascadeConfig& config) {
    m_config = config;

    m_motors = new pros::Motor_Group(
        std::vector<std::int8_t>(
            m_config.motor_ports.begin(),
            m_config.motor_ports.end()
        )
    );

    for (auto port : m_config.motor_ports) {
        pros::Motor motor(port);
        motor.set_gearing(m_config.gearset);
        motor.set_brake_mode(pros::E_MOTOR_BRAKE_HOLD);
    }

    // Zero the encoder here, so that "the bottom of the travel" means the same
    // thing on every run and not just on the first run after a power cycle.
    // The arm has to be resting at its bottom when the program starts, which is
    // exactly what the Python drivers require too.
    m_motors->tare_position();

    m_position_pid = PIDController(m_config.position_pid);
    m_target_inches = 0.0;
    m_position_pid.set_target(0.0);
}

bool Cascade::move_to(double inches) {
    m_manual_mode = false;

    double clamped = std::clamp(inches, 0.0, m_config.max_extension_inches);
    m_target_inches = clamped;

    m_position_pid.set_target(inches_to_ticks(clamped));
    m_position_pid.reset();

    return is_at_target();
}

bool Cascade::move_to_preset(int index) {
    if (index < 0 || index >= 4) {
        return false;
    }
    return move_to(m_config.presets[index]);
}

void Cascade::stop() {
    m_manual_mode = false;
    m_motors->move(0);

    // Adopt where the arm actually is, and point the PID there too. Setting
    // only m_target_inches was not enough: update() runs the PID, and the PID
    // still had whatever target it was last given, so the next update() would
    // have driven the arm back towards it after the stop.
    m_target_inches = current_extension();
    m_position_pid.set_target(inches_to_ticks(m_target_inches));
    m_position_pid.reset();
}

void Cascade::update() {
    if (m_manual_mode) {
        return;
    }

    double current = current_extension();
    double pid_output = m_position_pid.update(inches_to_ticks(current));

    if (m_config.gravity_feedforward > 0.0) {
        pid_output += m_config.gravity_feedforward;
    }

    // Clamp AFTER the feedforward has been added, not before it.
    //
    // The PID's own output is already limited to +/-127, so adding the
    // feedforward on top could reach 135. Motor_Group::move() takes a
    // std::int8_t, and 135 does not fit in one: that is undefined behaviour,
    // and on the ARM it wraps round to -121. The effect would be that whenever
    // the PID saturates upwards - which is what it does any time the arm is
    // far below its target, e.g. the moment you press a preset from rest - the
    // arm is driven DOWN at almost full power instead of up.
    pid_output = std::clamp(pid_output, -127.0, 127.0);

    // And never drive the arm below the bottom of its travel. This is the
    // software floor that stands in for the missing physical stopper.
    if (current <= kCascadeFloorInches && pid_output < 0.0) {
        pid_output = 0.0;
    }

    m_motors->move(static_cast<std::int8_t>(pid_output));
}

bool Cascade::is_at_target() const {
    return m_position_pid.is_settled();
}

double Cascade::current_extension() const {
    return ticks_to_inches(m_motors->get_position());
}

void Cascade::manual_control(int power) {
    m_manual_mode = true;

    double current = current_extension();
    if (current <= 0.0 && power < 0) {
        power = 0;
    }
    if (current >= m_config.max_extension_inches && power > 0) {
        power = 0;
    }

    m_motors->move(power);
}

void Cascade::home() {
    m_manual_mode = true;

    m_motors->move(-30);

    int stall_time = 0;
    const int stall_threshold_ms = 300;
    const int check_interval_ms = 20;
    const std::uint32_t started_ms = pros::millis();

    // Two ways out, not one. The stall detector alone is not enough: a chain
    // that slips, a motor that has come unplugged and reads no current, or an
    // arm that is already at the bottom without hitting the threshold would
    // never register a stall, and this would drive the arm down for ever.
    while (stall_time < stall_threshold_ms &&
           pros::millis() - started_ms < kHomeTimeoutMs) {
        pros::delay(check_interval_ms);

        double max_current = 0.0;
        for (auto port : m_config.motor_ports) {
            double c = pros::Motor(port).get_current_draw();
            if (c > max_current) {
                max_current = c;
            }
        }

        if (max_current > 1500.0) {
            stall_time += check_interval_ms;
        } else {
            stall_time = 0;
        }
    }

    m_motors->move(0);
    m_motors->tare_position();

    m_target_inches = 0.0;
    m_position_pid.set_target(0.0);
    m_position_pid.reset();
    m_manual_mode = false;
}

double Cascade::inches_to_ticks(double inches) const {
    return inches / m_config.inches_per_encoder_degree;
}

double Cascade::ticks_to_inches(double ticks) const {
    return ticks * m_config.inches_per_encoder_degree;
}

}  // namespace vex_pid