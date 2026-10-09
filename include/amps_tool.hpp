#pragma once
#include "robot.hpp"

namespace vex_pid {

// The amps measuring program.
//
// Every current threshold in this project - CASCADE_STRAIN_AMPS and friends -
// is a guess until it has been measured on the real robot. This is the program
// that measures them: it holds one mechanism at a time at the same speed the
// driver uses and shows what that mechanism really draws.
//
// It is NOT a driving program. Select it in src/main.cpp with kRunAmpsTool and
// nothing else runs. On the brain:
//
//     hold A   cascade, lifting up
//     hold B   claw, closing
//     hold X   toggle, turning
//     hold Y   drive, forwards
//
// and hold any d-pad button to stop everything. Letting go of the test button
// stops that mechanism at once.
//
// It deliberately sets no current ceiling on the motors: a ceiling is a clamp,
// so once a motor reaches it the reading stops climbing and "working hard"
// cannot be told apart from "about to stall".
void amps_tool(Robot& robot);

}  // namespace vex_pid
