// Host test for the driver's feel, and for the strain guard.
//
// Neither include/control_feel.hpp nor include/strain_guard.hpp touches PROS,
// so both can be compiled and run on a laptop - which is the only way to check
// the arithmetic before it ever reaches a robot. Run it with:
//
//     tools/feel_test.sh
//
// or by hand:
//
//     clang++ -std=c++20 -Wall -Wextra -Iinclude tools/feel_test.cpp -o /tmp/feel_test
//     /tmp/feel_test
//
#include "control_feel.hpp"
#include "strain_guard.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;

const char* ok(bool condition) { return condition ? "ok  " : "FAIL"; }

void check(bool condition, const std::string& what) {
    g_checks++;
    if (!condition) {
        g_failures++;
    }
    std::printf("  [%s] %s\n", ok(condition), what.c_str());
}

bool close(double a, double b, double tolerance = 1e-6) {
    return std::fabs(a - b) <= tolerance;
}

std::string num(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.4f", value);
    return std::string(buffer);
}

}  // namespace

int main() {
    using namespace vex_pid;

    const StickSettings stick;
    const double ceiling = 51.0;   // 40% of 127, the drive cap

    // -----------------------------------------------------------------------
    std::printf("The stick curve (ceiling %.1f)\n", ceiling);

    check(close(stick_shape(0.0, ceiling, stick), 0.0),
          "a resting stick asks for nothing");

    check(close(stick_shape(stick.deadband, ceiling, stick), 0.0),
          "and still nothing right at the edge of the deadband");

    check(close(stick_shape(-20.0, ceiling, stick),
                -stick_shape(20.0, ceiling, stick)),
          "the curve is symmetrical: left is right, mirrored");

    check(close(stick_shape(127.0, ceiling, stick), ceiling, 1e-9),
          "a stick pushed all the way to the stop asks for the whole ceiling (" +
              num(stick_shape(127.0, ceiling, stick)) + ")");

    // The join between the gentle zone and the fast zone: the same number
    // reached from both sides, so there is no step where they meet.
    {
        const double at_join = stick_shape(
            stick.fine_end * (kStickFull - stick.deadband) + stick.deadband,
            ceiling, stick
        );
        const double expected = (stick.min_move_fraction
                                 + (1.0 - stick.min_move_fraction) * stick.fine_top)
                                * ceiling;
        check(close(at_join, expected, 0.01),
              "where the gentle zone ends the stick is asking for " + num(at_join)
                  + " of " + num(ceiling) + " (the two halves meet exactly)");
    }

    // Just past the deadband: the smallest ask, and it must be a small one.
    {
        const double min_ask = stick_shape(stick.deadband + 0.01, ceiling, stick);
        const double expected = stick.min_move_fraction * ceiling;
        check(close(min_ask, expected, 0.05),
              "the moment the stick leaves the deadband it asks for "
                  + num(min_ask) + ", the minimum (" + num(expected) + ")");
    }

    // Monotonic, and no jump anywhere except the deliberate first step out of
    // the deadband. Tested at every whole stick position, which is what
    // get_analog() actually returns.
    {
        bool monotonic = true;
        double biggest_step = 0.0;
        double biggest_step_at = 0.0;
        double previous = stick_shape(-127.0, ceiling, stick);
        for (int raw = -126; raw <= 127; raw++) {
            const double value = stick_shape(static_cast<double>(raw), ceiling, stick);
            if (value < previous - 1e-9) {
                monotonic = false;
            }
            // The one step we expect is crossing the deadband edge, and it is
            // the minimum move - every step after that must be small. The
            // crossing is two-sided: the value jumps up leaving the deadband on
            // the way out, and down entering it on the way in.
            const bool crossing_deadband =
                (std::fabs(previous) < 1e-9) != (std::fabs(value) < 1e-9);
            const double step = std::fabs(value - previous);
            if (!crossing_deadband && step > biggest_step) {
                biggest_step = step;
                biggest_step_at = static_cast<double>(raw);
            }
            previous = value;
        }
        check(monotonic, "the curve never goes backwards as the stick is pushed");
        check(biggest_step < 1.5,
              "and there is no jump in it: away from the deadband edge the "
              "biggest step from one stick position to the next is "
                  + num(biggest_step) + " out of " + num(ceiling)
                  + ", at stick " + num(biggest_step_at));
    }

    // The cap, proved rather than asserted. Every whole stick position from
    // well past one end to well past the other, at both ceilings the robot
    // uses, must give an answer no bigger than the ceiling - and no bigger
    // than the motors take, whatever the ceiling is.
    {
        const double ceilings[2] = {51.0, 44.0};   // drive, turn
        bool capped = true;
        bool within_motor_range = true;
        double biggest = 0.0;
        int biggest_raw = 0;
        double biggest_ceiling = 0.0;

        for (const double test_ceiling : ceilings) {
            for (int raw = -300; raw <= 300; raw++) {
                const double value = stick_shape(static_cast<double>(raw),
                                                 test_ceiling, stick);
                if (std::fabs(value) > std::fabs(biggest)) {
                    biggest = value;
                    biggest_raw = raw;
                    biggest_ceiling = test_ceiling;
                }
                if (std::fabs(value) > test_ceiling + 1e-9) {
                    capped = false;
                }
                if (std::fabs(value) > kStickFull + 1e-9) {
                    within_motor_range = false;
                }
            }
        }

        check(capped,
              "the curve never asks for more than the ceiling, even for a stick "
              "value well outside -127..127 (601 positions x 2 ceilings; the "
              "biggest answer anywhere was " + num(biggest) + " at stick "
                  + num(biggest_raw) + " with a ceiling of "
                  + num(biggest_ceiling) + ")");
        check(within_motor_range,
              "and never asks for more than the motors take, " + num(kStickFull));
        check(close(stick_shape(127.0, 51.0, stick), 51.0, 1e-9) &&
                  close(stick_shape(127.0, 44.0, stick), 44.0, 1e-9),
              "full stick asks for exactly the ceiling: "
                  + num(stick_shape(127.0, 51.0, stick)) + " of 51, "
                  + num(stick_shape(127.0, 44.0, stick)) + " of 44");
    }

    // A table, so the shape of the curve can be read at a glance. Not a check:
    // it is here to be looked at when the feel is being judged. "asked" is the
    // command in the -127..127 the motors take; the last column is how much of
    // the ceiling that is.
    std::printf("      stick    asked   of ceiling\n");
    for (int percent = 0; percent <= 100; percent += 5) {
        const double raw = kStickFull * (static_cast<double>(percent) / 100.0);
        const double value = stick_shape(raw, ceiling, stick);
        std::printf("      %3d%%   %7.2f   %6.1f%%\n", percent, value,
                    100.0 * value / ceiling);
    }

    // The ramp: 250% of full per second, at the 20 ms loop the drivers run.
    check(close(ramp_step(250.0, 0.02), 6.35, 1e-9),
          "a 250%-per-second ramp may change the command by "
              + num(ramp_step(250.0, 0.02)) + " in one 20 ms pass");

    // The measured loop time is never believed to be shorter than the nominal.
    check(close(loop_seconds(1000, 0, 20), 0.02, 1e-9),
          "the first pass assumes the nominal loop time");
    check(close(loop_seconds(1000, 980, 20), 0.02, 1e-9),
          "a short reading is rounded up to the nominal, never believed");
    check(close(loop_seconds(2000, 1000, 20), 1.0, 1e-9),
          "a pass that really took a second is measured as a second");

    // -----------------------------------------------------------------------
    std::printf("Split arcade\n");
    {
        double left = 0.0;
        double right = 0.0;

        arcade(0.0, 30.0, ceiling, left, right);
        check(close(left, 30.0) && close(right, -30.0),
              "turning on the spot turns the wheels equally opposite");

        arcade(30.0, 0.0, ceiling, left, right);
        check(close(left, 30.0) && close(right, 30.0),
              "driving straight sends both wheels the same way");

        arcade(ceiling, ceiling, ceiling, left, right);
        check(std::fabs(left) <= ceiling + 1e-9 &&
                  std::fabs(right) <= ceiling + 1e-9,
              "forward and turn together cannot ask for more than the ceiling ("
                  + num(left) + ", " + num(right) + ")");
        check(close(std::fabs(left), ceiling, 1e-9),
              "and the strongest wheel keeps the whole ceiling, so the turn is "
              "not weakened");
    }

    // -----------------------------------------------------------------------
    std::printf("A motor's speed, written the same way as a command\n");
    check(close(velocity_fraction(100.0, 100.0), kStickFull, 1e-9),
          "a red motor at its free speed reads as a full command");
    check(close(velocity_fraction(50.0, 100.0), kStickFull / 2.0, 1e-9),
          "and half that reads as half a command");
    check(close(velocity_fraction(20.0, 0.0), 0.0),
          "a nonsense cartridge speed gives zero rather than dividing by it");

    // -----------------------------------------------------------------------
    std::printf("The strain guard\n");
    {
        StrainGuardSettings settings;
        StrainGuard guard(settings);

        check(close(guard.factor(), 1.0), "a fresh guard asks for full speed");
        check(guard.take_limit_change() && guard.limit_ma() == 2500,
              "and wants the ordinary per-motor ceiling first ("
                  + std::to_string(guard.limit_ma()) + " mA)");
        check(!guard.take_limit_change(),
              "which it only reports once, so the motor is not talked to every pass");

        // A mechanism barely being asked to move is not judged at all.
        guard.update(0.0, 0.0, 2.0);
        check(close(guard.factor(), 1.0) && !guard.blocked() && !guard.straining(),
              "a tiny crawl command is left alone, however slowly it moves");

        // Ordinary running: no strain, no block.
        for (int i = 0; i < 10; i++) {
            guard.update(0.8, 40.0, 40.0);
        }
        check(close(guard.factor(), 1.0),
              "a mechanism drawing under the threshold is not touched");

        // Over the current threshold. It takes three passes to be believed,
        // so a single spike does not trigger anything.
        guard.update(3.0, 40.0, 40.0);
        check(close(guard.factor(), 1.0) && !guard.straining(),
              "one spike of current is not a strain");
        guard.update(3.0, 40.0, 40.0);
        check(close(guard.factor(), 1.0) && !guard.straining(),
              "nor is two in a row");
        guard.update(3.0, 40.0, 40.0);
        check(guard.straining() && close(guard.factor(), 0.94),
              "three in a row is: the guard starts easing it back (factor "
                  + num(guard.factor()) + ")");
        check(guard.take_limit_change() && guard.limit_ma() == 1000,
              "and lowers the per-motor ceiling to " + std::to_string(guard.limit_ma())
                  + " mA");

        // Keep going: the factor falls to the floor and stops there.
        for (int i = 0; i < 40; i++) {
            guard.update(3.0, 40.0, 40.0);
        }
        check(close(guard.factor(), settings.ease_speed),
              "eased right down it stops at " + num(guard.factor())
                  + ", never at zero, so it still tries");

        // Freed again: recovery is slower than easing, deliberately.
        guard.update(0.5, 40.0, 40.0);
        check(close(guard.factor(), settings.ease_speed + settings.recover_step),
              "a freed mechanism gets a little speed back (" + num(guard.factor())
                  + "), more slowly than it was taken");
        for (int i = 0; i < 100; i++) {
            guard.update(0.5, 40.0, 40.0);
        }
        check(close(guard.factor(), 1.0), "and in time gets all of it back");
        check(guard.take_limit_change() && guard.limit_ma() == 2500,
              "and the ceiling goes back up to normal");
    }

    // The encoder half: the current looks perfectly ordinary, but the mechanism
    // has been told to go and is not going.
    {
        StrainGuardSettings settings;
        StrainGuard guard(settings);

        for (int i = 0; i < settings.blocked_loops - 1; i++) {
            guard.update(0.5, 0.0, 40.0);
        }
        check(!guard.blocked() && close(guard.factor(), 1.0),
              "a mechanism that has not started moving yet is given "
                  + std::to_string(settings.blocked_loops - 1)
                  + " passes before it is doubted, because a motor takes a "
                    "moment to spin up");

        guard.update(0.5, 0.0, 40.0);
        check(guard.blocked(),
              "after " + std::to_string(settings.blocked_loops)
                  + " passes of being told to go and not going, it is blocked");

        for (int i = 0; i < settings.strain_loops; i++) {
            guard.update(0.5, 0.0, 40.0);
        }
        check(guard.factor() < 1.0,
              "and the guard eases it back on the encoder alone, with the "
              "current never over the threshold (factor " + num(guard.factor()) + ")");

        // Being parked on purpose is not a fault.
        guard.clear_block();
        check(!guard.blocked(),
              "clearing the block (parking something on purpose) stops it");
    }

    // The guard judges the eased command, not the raw one - otherwise easing
    // would make the mechanism look even more blocked, and it would chase
    // itself down to the floor for no reason.
    {
        StrainGuardSettings settings;
        StrainGuard guard(settings);
        // Half the eased command counts as moving.
        for (int i = 0; i < 20; i++) {
            guard.update(0.5, 10.0, 20.0);
        }
        check(!guard.blocked(),
              "moving at half of what was asked is not blocked, even though it "
              "is well under the raw command");
    }

    // -----------------------------------------------------------------------
    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
