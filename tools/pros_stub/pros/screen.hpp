#pragma once
// Stand-in for pros/screen.hpp and pros/colors.hpp (fallback only).
//
// The real V5 screen is part of the kernel, not a template: pros::screen::print
// and pros::screen::erase work without any extra template installed, which is
// why the full stop uses them rather than the emulated 3-button LCD in
// pros/llemu.hpp. The declarations here are the shape tools/syntax_check.sh
// needs to check the call sites.

#include <cstdint>

namespace pros {

// Subset of pros::Color (the real enum is lowercase, e.g. pros::Color::red).
enum class Color : std::uint32_t {
    black = 0x00000000,
    white = 0x00FFFFFF,
    red = 0x00FF0000,
    green = 0x00008000,
    blue = 0x000000FF,
    yellow = 0x00FFFF00,
};

enum text_format_e_t {
    E_TEXT_SMALL = 0,
    E_TEXT_MEDIUM,
    E_TEXT_LARGE,
    E_TEXT_MEDIUM_CENTER,
    E_TEXT_LARGE_CENTER,
};

namespace screen {
std::uint32_t set_pen(Color color);
std::uint32_t set_pen(std::uint32_t color);
std::uint32_t set_eraser(Color color);
std::uint32_t set_eraser(std::uint32_t color);
std::uint32_t erase();
void print(text_format_e_t txt_fmt, const std::int16_t line, const char* text, ...);
void print(text_format_e_t txt_fmt, const std::int16_t x, const std::int16_t y,
           const char* text, ...);
}  // namespace screen

}  // namespace pros
