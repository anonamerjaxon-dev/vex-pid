#pragma once

// ---------------------------------------------------------------------------
// This file was missing from the project.
//
// src/main.cpp starts with #include "main.h", and with no main.h anywhere the
// compiler stopped on its very first line with
//
//     fatal error: 'main.h' file not found
//
// which is why this project had never once built. This is that file, modelled
// on the one PROS ships with its own project template.
//
// The include below is "api.h", NOT "pros/api.h". The PROS kernel installs a
// top-level api.h and there is no pros/api.h at all, so the longer path fails
// with 'pros/api.h' file not found. api.h is the master header that pulls in
// the whole PROS API.
//
// The PROS template also defines PROS_USE_LITERALS here, which turns on the
// C++ unit literals such as 4_mtr. Nothing in this project uses them, but
// keeping the define means the file matches the template.
// ---------------------------------------------------------------------------

#define PROS_USE_LITERALS

#include "api.h"

// Project-wide includes can go below this line.
#include "control_feel.hpp"
