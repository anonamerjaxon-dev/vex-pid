#include "subsystems/cascade.hpp"
#include "pros/rtos.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace vex_pid {

namespace {
// The loop the driver is meant to run at. A measured pass is never believed to
// have been shorter than this - see loop_seconds().
constexpr std::uint32_t kNominalLoopMs = 20;

// A hard cap on how long Cascade::home() will drive the arm downwards. It is
// also bounded by the stall detector, but a slipping chain or a motor that
// reads no current would never stall and the loop would never end.
constexpr std::uint32_t kHomeTimeoutMs = 2000;

// The current that counts as "the arm has reached the bottom and is pushing".
// PROS reports motor current in milliamps.
constexpr double kHomeStallCurrentMa = 1500.0;
}  // namespace

void Cascade::initialize(const CascadeConfig& config) {
    m_config = config;

    m_motors = new pros::MotorGroup(
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

    m_manual_demand = 0.0;
    m_ramped = 0.0;
    m_last_ms = 0;

    m_guard = StrainGuard(m_config.guard);
    m_guard.reset();
    apply_current_limit();
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
    m_manual_demand = 0.0;
    m_ramped = 0.0;
    m_guard.clear_block();
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
    const std::uint32_t now = pros::millis();
    const double dt = loop_seconds(now, m_last_ms, kNominalLoopMs);
    m_last_ms = now;

    if (m_manual_mode) {
        update_manual(dt);
        return;
    }

    double current = current_extension();
    double pid_output = m_position_pid.update(inches_to_ticks(current));

    if (m_config.gravity_feedforward > 0.0) {
        pid_output += m_config.gravity_feedforward;
    }

    // Clamp AFTER the feedforward has been added, not before it.
    //
    // MotorGroup::move() takes a std::int32_t and clamps its argument to
    // +/-127 itself, so an overshoot here would have been clamped to full speed
    // rather than doing anything worse - but relying on that would mean the
    // command is quietly out of range, which is exactly the sort of thing that
    // stops being true on the next kernel update. Being explicit costs nothing.
    pid_output = std::clamp(pid_output, -127.0, 127.0);

    // And never drive the arm below the bottom of its travel. This is the
    // software floor that stands in for the missing physical stopper.
    if (current <= m_config.floor_inches && pid_output < 0.0) {
        pid_output = 0.0;
    }

    m_motors->move(static_cast<std::int32_t>(std::lround(pid_output)));
}

void Cascade::update_manual(double dt_seconds) {
    const double here = current_extension();

    double wanted = m_manual_demand * m_config.manual_speed_percent;

    // Coming down near the bottom, crawl. The arm can then settle onto the
    // floor rather than stopping a full-speed step above it.
    if (wanted < 0.0 &&
        (here - m_config.floor_inches) < m_config.creep_band_inches) {
        wanted = m_manual_demand * m_config.creep_speed_percent;
    }

    // The strain guard judges the command the arm is really being given, not
    // the raw one. Easing the speed lowers the bar as well, so the guard cannot
    // chase itself into a stall it invented.
    const double judged = wanted * m_guard.factor();
    const double factor = m_guard.update(
        current_draw_amps(), measured_speed(), judged
    );
    wanted *= factor;

    m_ramped = rate_limit(
        m_ramped, wanted, ramp_step(m_config.ramp_percent_per_second, dt_seconds)
    );

    // Where the arm will be by the next pass, assuming it travels further than
    // the arithmetic says. If that lands below the floor, stop this pass.
    const double travel = travel_per_second() * (m_ramped / kStickFull)
                          * dt_seconds * m_config.safety_factor;
    const double predicted = here + travel;

    double send = m_ramped;
    if (m_ramped < 0.0 && predicted <= m_config.floor_inches) {
        send = 0.0;

        // Pin the ramp to a crawl rather than leaving it or zeroing it.
        // Zeroing makes the ramp start over from nothing and nudge the arm
        // down again, which stutters against the floor; leaving it alone lets
        // the arm set off again at whatever number it happened to be holding
        // when the floor stopped it. A crawl is the ceiling from here, so
        // setting off again can never be faster than one.
        m_ramped = -m_config.creep_speed_percent;

        // Being parked on purpose is not a mechanism in trouble, so the
        // encoder check is cleared: otherwise sitting on the floor with the
        // button held would slowly ease the arm for no reason.
        m_guard.clear_block();
    }

    apply_current_limit();
    m_motors->move(static_cast<std::int32_t>(std::lround(send)));
}

double Cascade::travel_per_second() const {
    // Full command (127) is full motor speed: rpm -> degrees per second ->
    // inches of arm travel, using the same inches-per-encoder-degree the
    // position PID uses.
    return (m_config.motor_max_rpm / 60.0) * 360.0
           * m_config.inches_per_encoder_degree;
}

double Cascade::current_draw_amps() const {
    if (!m_motors) {
        return 0.0;
    }
    const std::vector<std::int32_t> all = m_motors->get_current_draw_all();
    double total = 0.0;
    for (std::int32_t milliamps : all) {
        total += static_cast<double>(milliamps);
    }
    return total / 1000.0;
}

double Cascade::measured_speed() const {
    if (!m_motors) {
        return 0.0;
    }
    return velocity_fraction(
        m_motors->get_actual_velocity(), m_config.motor_max_rpm
    );
}

void Cascade::apply_current_limit() {
    if (m_motors && m_guard.take_limit_change()) {
        // Applied to every motor of the group, each in milliamps. NOTE this is
        // a per-motor ceiling, not a budget shared out across the group.
        m_motors->set_current_limit_all(m_guard.limit_ma());
    }
}

bool Cascade::is_at_target() const {
    return m_position_pid.is_settled();
}

double Cascade::current_extension() const {
    return ticks_to_inches(m_motors->get_position());
}

void Cascade::manual_control(double demand) {
    m_manual_mode = true;
    m_manual_demand = std::clamp(demand, -1.0, 1.0);
}

void Cascade::home() {
    m_manual_mode = true;
    m_manual_demand = 0.0;
    m_ramped = 0.0;

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

        if (max_current > kHomeStallCurrentMa) {
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
