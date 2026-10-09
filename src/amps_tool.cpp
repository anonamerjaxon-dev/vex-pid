#include "amps_tool.hpp"

#include "pros/misc.h"
#include "pros/rtos.hpp"
#include "pros/screen.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

namespace vex_pid {
namespace {

// The loop the tool polls at. The mechanisms are driven straight from
// test_raw(), which has no ramp, so this is only the screen refresh rate.
constexpr std::uint32_t kLoopMs = 20;

// How long one measurement runs. Five seconds is long enough for a motor to
// reach its steady current and short enough that nothing gets hot.
constexpr double kTestSeconds = 5.0;

// How many readings the "steady" figure averages over.
constexpr std::size_t kSteadySamples = 50;

// The measuring tool opens the current ceiling to this. 2500 mA is the V5
// smart motor's own maximum, so this is "no software ceiling at all".
constexpr std::int32_t kOpenCurrentLimitMa = 2500;

constexpr int kTestCount = 4;

enum class Mechanism { Cascade, Claw, Toggle, Drive };

struct AmpsTest {
    Mechanism what;
    char key;             // which face button, for the peak table
    const char* title;
    const char* hint_a;
    const char* hint_b;
    pros::controller_digital_e_t button;
};

// One entry per test. The cascade and the claw are the two that matter most:
// their thresholds are the ones the driver program acts on.
const AmpsTest kTests[kTestCount] = {
    {Mechanism::Cascade, 'A', "CASCADE  lifting up", "Load the arm by hand",
     "while it runs.", pros::E_CONTROLLER_DIGITAL_A},
    {Mechanism::Claw, 'B', "CLAW  closing", "Hold something in it",
     "so it grips.", pros::E_CONTROLLER_DIGITAL_B},
    {Mechanism::Toggle, 'X', "TOGGLE  turning", "Hold it back gently",
     "if you like.", pros::E_CONTROLLER_DIGITAL_X},
    {Mechanism::Drive, 'Y', "DRIVE  forwards", "Hold a wheel gently",
     "if you like.", pros::E_CONTROLLER_DIGITAL_Y},
};

// How many motors a test drives. Read from the config, so a port change in
// main.cpp is picked up here for free and the "each" figure is never wrong.
int motor_count(Robot& robot, Mechanism what) {
    const RobotConfig& config = robot.config();
    switch (what) {
        case Mechanism::Cascade:
            return static_cast<int>(config.cascade.motor_ports.size());
        case Mechanism::Claw:
            return 1;
        case Mechanism::Toggle:
            return static_cast<int>(config.toggle.motor_ports.size());
        case Mechanism::Drive:
            return static_cast<int>(config.drive.left_ports.size() +
                                    config.drive.right_ports.size());
    }
    return 1;
}

// The speed the DRIVER uses, taken from the same config, so the reading
// predicts what the driver program will draw.
double driver_speed(Robot& robot, Mechanism what) {
    const RobotConfig& config = robot.config();
    switch (what) {
        case Mechanism::Cascade:
            return config.cascade.manual_speed_percent;
        case Mechanism::Claw:
            return config.claw.manual_speed_percent;
        case Mechanism::Toggle:
            return config.toggle.manual_speed_percent;
        case Mechanism::Drive:
            return config.drive.drive_speed_max;
    }
    return 0.0;
}

void command(Robot& robot, Mechanism what, double percent) {
    switch (what) {
        case Mechanism::Cascade: robot.cascade().test_raw(percent); break;
        case Mechanism::Claw:    robot.claw().test_raw(percent);    break;
        case Mechanism::Toggle:  robot.toggle().test_raw(percent);  break;
        case Mechanism::Drive:   robot.drive().test_raw(percent);   break;
    }
}

double amps_of(Robot& robot, Mechanism what) {
    switch (what) {
        case Mechanism::Cascade: return robot.cascade().current_draw_amps();
        case Mechanism::Claw:    return robot.claw().current_draw_amps();
        case Mechanism::Toggle:  return robot.toggle().current_draw_amps();
        case Mechanism::Drive:   return robot.drive().current_draw_amps();
    }
    return 0.0;
}

// A ceiling is a clamp: once a motor reaches it the reading stops climbing, so
// "working hard" and "about to stall" look identical. The tool therefore opens
// the ceiling to the hardware maximum for the measurement and puts the
// configured value back afterwards.
void open_current_ceiling(Robot& robot, Mechanism what) {
    if (what == Mechanism::Cascade) {
        robot.cascade().test_current_limit_ma(kOpenCurrentLimitMa);
    } else if (what == Mechanism::Claw) {
        robot.claw().test_current_limit_ma(kOpenCurrentLimitMa);
    }
}

void restore_current_ceiling(Robot& robot, Mechanism what) {
    if (what == Mechanism::Cascade) {
        robot.cascade().test_current_limit_ma(
            robot.cascade().configured_current_limit_ma());
    } else if (what == Mechanism::Claw) {
        robot.claw().test_current_limit_ma(
            robot.claw().configured_current_limit_ma());
    }
}

bool stop_held(pros::Controller& master) {
    return master.get_digital(pros::E_CONTROLLER_DIGITAL_UP) != 0 ||
           master.get_digital(pros::E_CONTROLLER_DIGITAL_DOWN) != 0 ||
           master.get_digital(pros::E_CONTROLLER_DIGITAL_LEFT) != 0 ||
           master.get_digital(pros::E_CONTROLLER_DIGITAL_RIGHT) != 0;
}

class Reading {
public:
    void reset() {
        m_now = 0.0;
        m_peak = 0.0;
        m_count = 0;
        m_next = 0;
    }

    void watch(double amps) {
        m_now = amps;
        m_peak = std::max(m_peak, amps);
        m_recent[m_next] = amps;
        m_next = (m_next + 1) % kSteadySamples;
        if (m_count < kSteadySamples) {
            ++m_count;
        }
    }

    double now() const { return m_now; }
    double peak() const { return m_peak; }

    double steady() const {
        if (m_count == 0) {
            return 0.0;
        }
        double total = 0.0;
        for (std::size_t i = 0; i < m_count; ++i) {
            total += m_recent[i];
        }
        return total / static_cast<double>(m_count);
    }

private:
    double m_now = 0.0;
    double m_peak = 0.0;
    double m_recent[kSteadySamples] = {};
    std::size_t m_count = 0;
    std::size_t m_next = 0;
};

// The V5 screen's printf does not handle floats on every firmware, so every
// line is built here with std::snprintf and sent as a plain string.
void show(std::int16_t row, const char* text) {
    pros::screen::print(pros::E_TEXT_MEDIUM, row, "%s", text);
}

void amps_line(char* text, std::size_t size, const char* label, double value,
               int motors) {
    if (motors > 1) {
        std::snprintf(text, size, "%-7s%4.2f A (%4.2f each)", label, value,
                      value / static_cast<double>(motors));
    } else {
        std::snprintf(text, size, "%-7s%4.2f A", label, value);
    }
}

void show_idle(const double peaks[kTestCount]) {
    static const char* const names[kTestCount] = {"cascade", "claw", "toggle",
                                                 "drive"};
    char text[48];

    pros::screen::erase();
    pros::screen::set_pen(pros::Color::white);
    pros::screen::print(pros::E_TEXT_LARGE, 0, "%s", "CASCADE ROBOT AMPS");
    show(2, "Hold A B X Y to test.");
    show(3, "Let go to stop it.");
    show(4, "Load the part by hand.");
    show(6, "peaks so far:");
    for (int i = 0; i < kTestCount; ++i) {
        if (peaks[i] < 0.0) {
            std::snprintf(text, sizeof(text), "%c %-8s   --.--", kTests[i].key,
                          names[i]);
        } else {
            std::snprintf(text, sizeof(text), "%c %-8s %6.2f A", kTests[i].key,
                          names[i], peaks[i]);
        }
        show(static_cast<std::int16_t>(7 + i), text);
    }
    show(11, "d-pad = FULL STOP");
}

void show_stopped() {
    pros::screen::erase();
    pros::screen::set_pen(pros::Color::red);
    pros::screen::print(pros::E_TEXT_LARGE_CENTER, 5, "%s", "STOPPED");
    pros::screen::print(pros::E_TEXT_MEDIUM_CENTER, 7, "%s",
                        "release the d-pad");
    pros::screen::set_pen(pros::Color::white);
}

void show_running(Robot& robot, const AmpsTest& test, const Reading& reading,
                  double left_seconds) {
    const int motors = motor_count(robot, test.what);
    char text[48];

    // No erase here: this runs every pass and erasing would flicker. The test
    // erased once when it started, and every line is re-printed in full.
    show(0, test.title);
    amps_line(text, sizeof(text), "now", reading.now(), motors);
    show(2, text);
    amps_line(text, sizeof(text), "peak", reading.peak(), motors);
    show(3, text);
    amps_line(text, sizeof(text), "steady", reading.steady(), motors);
    show(4, text);
    std::snprintf(text, sizeof(text), "%4.1f s left", left_seconds);
    show(6, text);
    show(8, test.hint_a);
    show(9, test.hint_b);
    show(11, "d-pad = FULL STOP");
}

void show_result(Robot& robot, const AmpsTest& test, const Reading& reading) {
    const int motors = motor_count(robot, test.what);
    char text[48];

    pros::screen::erase();
    pros::screen::set_pen(pros::Color::white);
    show(0, test.title);
    show(1, "  DONE");
    amps_line(text, sizeof(text), "peak", reading.peak(), motors);
    show(3, text);
    amps_line(text, sizeof(text), "steady", reading.steady(), motors);
    show(4, text);
    show(6, "Write the PEAK down.");
    show(8, "Let go, then hold");
    show(9, "another button.");
}

// Runs one measurement into `reading`. Returns false if the stop ended it
// early - a half-finished reading is deliberately not recorded.
bool run_test(Robot& robot, pros::Controller& master, const AmpsTest& test,
              Reading& reading) {
    reading.reset();
    open_current_ceiling(robot, test.what);

    pros::screen::erase();
    pros::screen::set_pen(pros::Color::white);

    const std::uint32_t start_ms = pros::millis();
    bool cut_short = false;

    while (true) {
        const double left_seconds =
            kTestSeconds - static_cast<double>(pros::millis() - start_ms) / 1000.0;

        if (left_seconds <= 0.0) {
            break;
        }
        if (master.get_digital(test.button) == 0) {
            break;   // the hand came off: stop at once
        }
        if (stop_held(master)) {
            cut_short = true;
            break;
        }

        command(robot, test.what, driver_speed(robot, test.what));
        reading.watch(amps_of(robot, test.what));
        show_running(robot, test, reading, left_seconds);
        pros::delay(kLoopMs);
    }

    command(robot, test.what, 0.0);
    robot.stop_all();
    restore_current_ceiling(robot, test.what);

    return !cut_short;
}

}  // namespace

void amps_tool(Robot& robot) {
    pros::Controller master(pros::E_CONTROLLER_MASTER);

    // -1 means "not measured yet", which is different from a genuine 0.00 A.
    double peaks[kTestCount] = {-1.0, -1.0, -1.0, -1.0};

    robot.stop_all();
    pros::screen::set_pen(pros::Color::white);
    show_idle(peaks);
    master.rumble(".");

    while (true) {
        // The stop is read first, so it works from the idle screen too.
        if (stop_held(master)) {
            robot.stop_all();
            show_stopped();
            while (stop_held(master)) {
                pros::delay(kLoopMs);
            }
            show_idle(peaks);
            continue;
        }

        int pressed = -1;
        for (int i = 0; i < kTestCount; ++i) {
            if (master.get_digital(kTests[i].button) != 0) {
                pressed = i;
                break;
            }
        }
        if (pressed < 0) {
            pros::delay(kLoopMs);
            continue;
        }

        Reading reading;
        if (!run_test(robot, master, kTests[pressed], reading)) {
            // Cut short by the stop. It does not count as a measurement.
            show_stopped();
            while (stop_held(master) ||
                   master.get_digital(kTests[pressed].button) != 0) {
                robot.stop_all();
                pros::delay(kLoopMs);
            }
            show_idle(peaks);
            continue;
        }

        peaks[pressed] = std::max(peaks[pressed], reading.peak());
        show_result(robot, kTests[pressed], reading);
        while (stop_held(master) ||
               master.get_digital(kTests[pressed].button) != 0) {
            pros::delay(kLoopMs);
        }
        show_idle(peaks);
    }
}

}  // namespace vex_pid
