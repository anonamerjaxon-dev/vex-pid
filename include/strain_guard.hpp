#pragma once
#include "control_feel.hpp"

#include <algorithm>
#include <cmath>

// One mechanism's strain guard, ported from the Python drive program
// (vexcode-python/cascade_robot_drive_v2.py, class StrainGuard). The reasoning
// is written up in vexcode-python/CONTROL_FEEL.md; this is the same idea in
// C++.
//
// The guard watches two things and eases the mechanism back when either says
// "something is wrong":
//
//   * the current. A V5 motor that is straining draws a lot of current before
//     it actually stops, so an amp reading catches a mechanism being loaded.
//   * the encoder. A mechanism that has been told to move and is not moving is
//     in trouble even if the current looks perfectly ordinary - a gear that has
//     slipped, a chain dragging, a part wedged against something.
//
// Neither one alone is enough, which is why both are here. The amp thresholds
// are guesses until they are measured on the real robot with
// vexcode-python/cascade_robot_amps.py; the encoder half needs no measuring,
// because its numbers are fractions of what was asked for.
//
// Nothing in this file talks to PROS, so it can be read, reasoned about and
// tested on a laptop (see tools/feel_test.cpp).
namespace vex_pid {

struct StrainGuardSettings {
    // The group TOTAL that counts as straining, in amps. Note this is the whole
    // group: get_current_draw_all() on a MotorGroup returns one reading per
    // motor and the caller adds them up. The Python program's strain threshold
    // is a pair total in the same way.
    double strain_amps = 2.0;

    // The per-motor current ceiling, in amps, used while straining and while
    // relaxed. PROS takes milliamps and applies the limit to EACH motor of a
    // group, never as a shared budget, so both of these are per motor. The
    // kernel's own default is 2.5 A per motor.
    double ease_amps = 1.0;
    double relaxed_amps = 2.5;

    // The lowest the gentle factor may fall, as a fraction of what was asked
    // for. It never reaches zero: a mechanism that is eased right down and
    // still cannot move is a mechanism that needs looking at, and a stall at
    // 35% is far kinder than a stall at 100%.
    double ease_speed = 0.35;

    // How many passes in a row the current must be over the threshold before
    // the guard believes it. A single spike - the moment a motor starts, or a
    // gear taking up its slack - is not a strain.
    int strain_loops = 3;

    // How much of the factor is given up per pass while straining, and how
    // fast it is given back once the mechanism is free again. Recovery is much
    // slower than easing: it is better to be gentle for a moment too long than
    // to bounce straight back into the thing that was straining.
    double ease_step = 0.06;
    double recover_step = 0.02;

    // The encoder half. Moving slower than this share of what was asked for,
    // for this many passes in a row, counts as "told to move and not moving".
    // The count is deliberately longer than strain_loops: a motor takes a
    // moment to spin up, and that pause must not be read as a fault.
    double blocked_fraction = 0.25;
    int blocked_loops = 6;

    // Below this ask the speed reading is noise, so small deliberate crawl
    // commands are left alone and never flagged as blocked.
    double blocked_min_ask = 5.0;
};

class StrainGuard {
public:
    // The ceiling is worked out in the constructor, not on the first update().
    // Otherwise a guard that had never been updated would report its "never
    // set" sentinel of -1 mA as the limit - and a caller that trusted it would
    // try to set a motor's current limit to minus one milliamp.
    StrainGuard() { want_limit(m_settings.relaxed_amps); }
    explicit StrainGuard(const StrainGuardSettings& settings) : m_settings(settings) {
        want_limit(m_settings.relaxed_amps);
    }

    // Call once per pass, then use factor() for this pass's command.
    //
    //   amps      the group total, in amps
    //   velocity  what the mechanism is really doing, on the same -127..127
    //             scale as a command (see velocity_fraction)
    //   commanded the signed command the mechanism has been given, same scale
    //
    // Returns the factor to multiply the wanted speed by. Comparing the
    // velocity against the eased command rather than the raw one matters:
    // easing the speed lowers the bar as well, so the guard cannot chase itself
    // into a stall it invented.
    double update(double amps, double velocity, double commanded) {
        if (std::fabs(commanded) < m_settings.blocked_min_ask) {
            // Barely being asked to move, so there is nothing to judge. The
            // eased factor is deliberately NOT reset here - a mechanism that
            // has just been working hard should not snap back to full the
            // instant the button is released.
            m_counting = 0;
            m_straining = false;
            clear_block();
            want_limit(m_settings.relaxed_amps);
            return m_factor;
        }

        const bool over_amps = amps >= m_settings.strain_amps;
        const bool not_moving = blocked(commanded, velocity);

        if (over_amps || not_moving) {
            m_counting++;
            if (m_counting >= m_settings.strain_loops) {
                m_straining = true;
            }
            if (m_straining) {
                m_factor = std::max(m_factor - m_settings.ease_step,
                                    m_settings.ease_speed);
                want_limit(m_settings.ease_amps);
            }
        } else {
            m_counting = 0;
            m_straining = false;
            m_factor = std::min(m_factor + m_settings.recover_step, 1.0);
            want_limit(m_settings.relaxed_amps);
        }

        return m_factor;
    }

    // Being stopped on purpose is not a mechanism in trouble, so the driver
    // clears the block whenever it parks something (the cascade on its floor,
    // or any mechanism under the full stop button). The eased factor is left
    // alone by this; only the "told to move and not moving" count is cleared.
    void clear_block() {
        m_blocked_count = 0;
        m_blocked = false;
    }

    // Back to full speed and the relaxed ceiling - used when everything starts
    // over, e.g. the robot is disabled.
    void reset() {
        m_factor = 1.0;
        m_counting = 0;
        m_straining = false;
        clear_block();
        m_speed = 0.0;
        want_limit(m_settings.relaxed_amps);
    }

    double factor() const { return m_factor; }
    bool straining() const { return m_straining; }
    bool blocked() const { return m_blocked; }

    // The last velocity the guard was shown, on the -127..127 scale.
    double speed() const { return m_speed; }

    // The per-motor current ceiling the guard wants right now, in milliamps,
    // and whether it has changed since the last time it was taken. Talking to
    // a motor is not free, so the caller only does it when this says so.
    int limit_ma() const { return m_limit_ma; }

    bool take_limit_change() {
        const bool changed = m_limit_dirty;
        m_limit_dirty = false;
        return changed;
    }

    const StrainGuardSettings& settings() const { return m_settings; }

private:
    bool blocked(double commanded, double velocity) {
        m_speed = velocity;
        if (std::fabs(velocity) < std::fabs(commanded) * m_settings.blocked_fraction) {
            m_blocked_count++;
        } else {
            m_blocked_count = 0;
        }
        m_blocked = m_blocked_count >= m_settings.blocked_loops;
        return m_blocked;
    }

    void want_limit(double amps) {
        const int ma = static_cast<int>(amps * 1000.0 + 0.5);
        if (ma != m_limit_ma) {
            m_limit_ma = ma;
            m_limit_dirty = true;
        }
    }

    StrainGuardSettings m_settings;

    double m_factor = 1.0;
    int m_counting = 0;
    bool m_straining = false;

    int m_blocked_count = 0;
    bool m_blocked = false;
    double m_speed = 0.0;

    int m_limit_ma = -1;      // deliberately impossible, so the first take
    bool m_limit_dirty = true;// always reports a change
};

// A motor's speed, in RPM, written on the same -127..127 scale as a command, so
// that "is it moving as fast as it was asked to?" is a like-for-like
// comparison. `max_rpm` is the motor's free speed for its cartridge: 100 for
// red 36:1, 200 for green 18:1, 600 for blue 6:1.
inline double velocity_fraction(double rpm, double max_rpm) {
    if (max_rpm <= 0.0) {
        return 0.0;
    }
    return rpm * kStickFull / max_rpm;
}

}  // namespace vex_pid
