#include "32blit.hpp"

const uint32_t VBUS_DETECT_PIN = 2;
const uint32_t CHARGE_STATUS_PIN = 24;
const uint32_t ADC_BAT_SENSE_PIN = 26;

const uint16_t PICOSYSTEM_ALL_BUTTONS = 0b11111111;

enum state {
    START,
    TEST_BATTERY,
    TEST_BUTTONS,
    PASS,
    FAIL,
    REMOVE_USB
};