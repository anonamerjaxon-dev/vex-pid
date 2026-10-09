#pragma once
#include <cstdint>
namespace pros {
std::uint32_t millis();
void delay(std::uint32_t milliseconds);
void delay(int milliseconds);
class Task {
public:
    Task(const char* name, void (*callback)(void*), void* arg, std::uint32_t priority,
         std::uint32_t stack_depth = 8192, const char* ignore = nullptr);
};
}  // namespace pros
