#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

// The driver's stick feel, ported from the Python drive program
// (vexcode-python/cascade_robot_drive_v2.py). The numbers here are the same
// ones, and the reasoning is written up in vexcode-python/CONTROL_FEEL.md.
//
// The one idea behind all of it: rate-limit the OUTPUT, do not smooth the
// INPUT. Averaging the stick over time would soften it, but it would also wash
// out the fine detail and add lag, so the robot would feel heavy and late. A
// slew rate limiter caps how fast the output may change instead, which keeps
// the stick crisp and still stops the robot from lurching.
//
// Everything is a plain function with no PROS dependency, so it can be read,
// reasoned about and unit-tested on a laptop.
namespace vex_pid {

// The full travel of a V5 controller stick, in the same -127..127 units the
// motors take. `get_analog()` returns exactly this range.
inline constexpr double kStickFull = 127.0;

// How the sticks are bent. A deadband of 6.35 is 5% of 127.
struct StickSettings {
    double deadband = 6.35;           // ignored at the centre, so a resting
                                      // stick does not creep
    double fine_end = 0.85;           // where the gentle zone ends (0..1)
    double fine_top = 0.40;           // how much of the ceiling it uses
    double expo = 2.0;                // >1 makes the middle gentler still
    double min_move_fraction = 0.15;  // the slowest a moving stick ever asks
                                      // for, so the robot never refuses to
                                      // start moving
};

// 0 inside the deadband. Past it the value is rescaled so that the edge of the
// deadband is 0 and a full stick is 127 - without that there would be a step
// at the edge, which is exactly the jolt the deadband was meant to avoid.
inline double apply_deadband(double value, double deadband) {
    if (std::fabs(value) <= deadband) {
        return 0.0;
    }
    double sign = value > 0.0 ? 1.0 : -1.0;
    return sign * (std::fabs(value) - deadband) * kStickFull
           / (kStickFull - deadband);
}

// Bend a stick into a motor command.
//
// Gentle over the first `fine_end` of the travel, then the rest of the range.
// The two halves meet exactly, so there is no step anywhere in the curve. The
// result is scaled to `ceiling`, and only a stick pushed all the way to the
// stop reaches all of it.
inline double stick_shape(double value, double ceiling,
                          const StickSettings& settings) {
    double shaped_input = apply_deadband(value, settings.deadband);
    if (shaped_input == 0.0) {
        return 0.0;
    }

    double sign = shaped_input > 0.0 ? 1.0 : -1.0;
    double magnitude = std::fabs(shaped_input) / kStickFull;   // 0..1

    // Never trust the caller to have stayed in range. The deadband rescale
    // multiplies by kStickFull / (kStickFull - deadband), which is slightly
    // more than 1, so a value already outside -127..127 comes out further
    // outside - and the fast zone would then carry it straight past the
    // ceiling. get_analog() cannot do that, but the cap below is the whole
    // point of this function, so it is enforced rather than assumed.
    if (magnitude > 1.0) {
        magnitude = 1.0;
    }

    double shaped;
    if (magnitude <= settings.fine_end) {
        // The fine zone: a power curve, so the first few millimetres of travel
        // move the robot a tiny amount.
        shaped = settings.fine_top
                 * std::pow(magnitude / settings.fine_end, settings.expo);
    } else {
        // The fast zone: straight from the top of the fine zone to the ceiling.
        double over = (magnitude - settings.fine_end) / (1.0 - settings.fine_end);
        shaped = settings.fine_top + (1.0 - settings.fine_top) * over;
    }

    double power = settings.min_move_fraction
                   + (1.0 - settings.min_move_fraction) * shaped;
    return sign * power * ceiling;
}

// Move `current` toward `target` by at most `max_step`. This is the slew rate
// limiter. Calling it once per driver loop caps how fast any motor command can
// change, which is what stops a full-stick flick from slamming the drive.
//
// One limiter per signal: never share one between the left and right wheels,
// or a hard turn would make one wheel ration the other.
inline double rate_limit(double current, double target, double max_step) {
    double change = target - current;
    if (change > max_step) {
        return current + max_step;
    }
    if (change < -max_step) {
        return current - max_step;
    }
    return target;
}

// How long the last pass round the driver loop actually took, in seconds.
//
// This is measured rather than assumed. The program reads the clock, decides
// what to do and waits; if a pass overruns, every ramp step computed from a
// nominal 20 ms is too small and the mechanisms move further between passes
// than the arithmetic believes. Reading the clock is what keeps the ramp and
// the software floor honest when the brain is busy.
//
// A reading shorter than the nominal loop time is not believed: the robot may
// have been busy elsewhere, and under-estimating the time under-estimates how
// far a mechanism has travelled, which is the unsafe direction to be wrong in.
inline double loop_seconds(std::uint32_t now_ms, std::uint32_t last_ms,
                           std::uint32_t nominal_ms) {
    double elapsed = (last_ms == 0)
                         ? static_cast<double>(nominal_ms)
                         : static_cast<double>(now_ms - last_ms);
    if (elapsed < static_cast<double>(nominal_ms)) {
        elapsed = static_cast<double>(nominal_ms);
    }
    return elapsed / 1000.0;
}

// The most a motor command may change in one pass, for a ramp written as
// "percent of full per second" - the unit the Python programs use.
inline double ramp_step(double percent_per_second, double dt_seconds) {
    return (percent_per_second / 100.0) * kStickFull * dt_seconds;
}

// Split arcade: one stick forward and back, the other left and right. Each
// wheel is asked for forward plus or minus turn, and because adding the two
// can ask for more than the ceiling, the excess is shared out across both
// wheels instead of clipping one - clipping one would drag the robot sideways.
inline void arcade(double forward, double turn, double ceiling,
                   double& out_left, double& out_right) {
    double left = forward + turn;
    double right = forward - turn;

    double biggest = std::max(std::fabs(left), std::fabs(right));
    if (biggest > ceiling && biggest > 0.0) {
        left = left * ceiling / biggest;
        right = right * ceiling / biggest;
    }

    out_left = left;
    out_right = right;
}

}  // namespace vex_pid
