#!/usr/bin/env bash
# Compile-checks the PROS sources with clang, without the PROS toolchain.
#
# There is no VEX kernel and no ARM toolchain on a Mac, so this is the only
# check that can be run here. It uses the stand-in headers in tools/pros_stub/,
# which declare just the slice of the PROS API this project actually uses.
#
# What that means, honestly:
#   * it IS a real check of this project's own code - syntax, typos, missing
#     includes, and type mistakes (e.g. a value that does not fit in the
#     std::int8_t that Motor::move() takes);
#   * it is NOT the real API, so passing does not prove the project links or
#     runs on the brain. Build it in PROS for that.
set -e
cd "$(dirname "$0")/.."
clang++ -std=c++20 -Wall -Wextra -fsyntax-only \
    -Itools/pros_stub -Iinclude \
    src/main.cpp \
    src/robot.cpp \
    src/data_logger.cpp \
    src/subsystems/cascade.cpp \
    src/subsystems/claw.cpp \
    src/subsystems/drive_base.cpp \
    src/subsystems/toggle.cpp
echo "syntax check: OK (stand-in headers, not the real PROS API)"
