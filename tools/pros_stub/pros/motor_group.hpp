#pragma once
//
// A STAND-IN for pros/motor_group.hpp.
//
// In the real PROS 4 kernel pros::MotorGroup lives in its own header, and
// pros/motors.hpp does NOT include it. That is not a detail: a translation unit
// that includes only "pros/motors.hpp" and then names pros::MotorGroup does not
// compile. This stand-in mirrors that, so the fallback check behaves like the
// real one.
//
#include "pros/motors.hpp"
