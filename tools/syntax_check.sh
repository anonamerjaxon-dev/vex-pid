#!/usr/bin/env bash
# Compile-checks the PROS sources with clang, without the PROS toolchain.
#
# There is no VEX kernel and no ARM toolchain on a Mac, so this is the only
# check that can be run here.
#
# It uses the REAL PROS headers if tools/.pros_headers/include exists (run
# tools/get_pros_headers.sh once to fetch them). If it does not, it falls back
# to the hand-written stand-ins in tools/pros_stub/, and says so loudly.
#
# What that means, honestly:
#   * with the real headers it IS a real check of this project's own code -
#     syntax, typos, missing includes, and every API name and signature. It
#     still does NOT link or run, so it is not a substitute for building in
#     PROS;
#   * with the stand-ins it is a weaker check: they are copied from the real
#     headers, but only the slice this project uses, and they can drift.
#
# The first version of this script used stand-in headers that had been written
# from the project's own code. That was circular - it could not catch a wrong
# API name, because the stub had been built from the same mistake - and it
# passed a project that used pros::Motor_Group, which does not exist. Hence the
# real headers, and hence the warning below.
set -euo pipefail
cd "$(dirname "$0")/.."

REAL="tools/.pros_headers/include"
if [ -d "$REAL/pros" ]; then
    MODE="the real PROS headers in $REAL"
    HEADERS=(-Iinclude -I"$REAL")
else
    MODE="STAND-IN headers in tools/pros_stub (run tools/get_pros_headers.sh for the real API)"
    HEADERS=(-Iinclude -Itools/pros_stub)
fi

# -Iinclude MUST come first: the kernel ships its own main.h, and it would
# otherwise shadow this project's.
clang++ -std=c++20 -Wall -Wextra -fsyntax-only \
    -Wno-deprecated-literal-operator \
    "${HEADERS[@]}" \
    src/main.cpp \
    src/robot.cpp \
    src/data_logger.cpp \
    src/subsystems/cascade.cpp \
    src/subsystems/claw.cpp \
    src/subsystems/drive_base.cpp \
    src/subsystems/toggle.cpp

echo "syntax check: OK using $MODE"

if [ ! -d "$REAL/pros" ]; then
    echo "NOTE: this was NOT checked against the real PROS API. To do that:"
    echo "      tools/get_pros_headers.sh"
    echo "      tools/syntax_check.sh"
fi
