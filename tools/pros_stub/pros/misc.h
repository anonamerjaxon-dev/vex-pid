#pragma once
#include <cstdint>
namespace pros {
enum controller_id_e_t { E_CONTROLLER_MASTER = 0, E_CONTROLLER_PARTNER = 1 };
enum controller_digital_e_t {
    E_CONTROLLER_DIGITAL_L1 = 0, E_CONTROLLER_DIGITAL_L2 = 1,
    E_CONTROLLER_DIGITAL_R1 = 2, E_CONTROLLER_DIGITAL_R2 = 3,
    E_CONTROLLER_DIGITAL_UP = 4, E_CONTROLLER_DIGITAL_DOWN = 5,
    E_CONTROLLER_DIGITAL_LEFT = 6, E_CONTROLLER_DIGITAL_RIGHT = 7,
    E_CONTROLLER_DIGITAL_A = 8, E_CONTROLLER_DIGITAL_B = 9,
    E_CONTROLLER_DIGITAL_X = 10, E_CONTROLLER_DIGITAL_Y = 11
};
enum controller_analog_e_t {
    E_CONTROLLER_ANALOG_LEFT_X = 0, E_CONTROLLER_ANALOG_LEFT_Y = 1,
    E_CONTROLLER_ANALOG_RIGHT_X = 2, E_CONTROLLER_ANALOG_RIGHT_Y = 3
};
class Controller {
public:
    explicit Controller(controller_id_e_t id);
    std::int32_t get_analog(controller_analog_e_t channel) const;
    std::int32_t get_digital(controller_digital_e_t button) const;
    bool get_digital_new_press(controller_digital_e_t button) const;
    void rumble(const char* pattern) const;
};
}  // namespace pros
