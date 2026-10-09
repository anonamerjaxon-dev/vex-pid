#pragma once

// This file has to exist and it has to be called main.h: every source file in
// a PROS project starts with #include "main.h", and src/main.cpp:1 is no
// exception.
//
// It had gone missing, which is why this project has never once built. The
// compiler stopped on line 1 with
//
//     fatal error: 'main.h' file not found
//
// before it ever read a line of the robot code, so none of the rest of this
// folder has ever been checked by a compiler either.
//
// PROS 4 ships the whole robot API behind one header, so pulling that in is
// all this needs to do. If your kernel does not have pros/api.h, replace the
// line below with the headers the rest of the project already uses, which are
// known to resolve here:
//
//     #include "pros/motors.hpp"     // Motor, Motor_Group, gearset, brake modes
//     #include "pros/imu.hpp"        // Imu
//     #include "pros/misc.h"          // Controller, E_CONTROLLER_DIGITAL_*
//     #include "pros/rtos.hpp"        // millis(), delay(), Task
#include "pros/api.h"
