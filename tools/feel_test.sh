#!/usr/bin/env bash
# Builds and runs tools/feel_test.cpp on this machine.
#
# The stick curve and the strain guard deliberately have no PROS dependency, so
# their arithmetic can be checked here, on the laptop, before it ever reaches a
# robot. That matters: none of this can be debugged by watching the robot, and
# a curve with a step in it is felt rather than seen.
set -euo pipefail
cd "$(dirname "$0")/.."

OUT="${TMPDIR:-/tmp}/feel_test"
clang++ -std=c++20 -Wall -Wextra -Iinclude tools/feel_test.cpp -o "$OUT"
"$OUT"
