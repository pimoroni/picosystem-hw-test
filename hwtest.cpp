#include "hwtest.hpp"
#ifdef PICO_BOARD
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#else
bool gpio_get(uint16_t pin) {
    return false;
}
uint16_t adc_read() {
    return 0;
}
#endif

using namespace blit;

std::string text = "";
state current_state = START;
state next_state = START;
Timer t_advance_state;
uint16_t pressed_buttons = 0;

void goto_state(state state, uint32_t timeout=500) {
    if(next_state == state) return; // Already requested!
    next_state = state;
    t_advance_state.init([](Timer &t) {
        current_state = next_state;
    }, timeout, 1);
    t_advance_state.start();
}

void init() {
    set_screen_mode(ScreenMode::lores);

#ifdef PICO_BOARD
    adc_init();
    adc_gpio_init(ADC_BAT_SENSE_PIN);
    adc_select_input(0);

    gpio_init(VBUS_DETECT_PIN);
    gpio_set_dir(VBUS_DETECT_PIN, GPIO_IN);
    gpio_init(CHARGE_STATUS_PIN);
    gpio_set_dir(CHARGE_STATUS_PIN, GPIO_IN);
#endif

    channels[0].waveforms   = Waveform::SQUARE;
    channels[0].attack_ms   = 250;
    channels[0].decay_ms    = 250;
    channels[0].sustain     = 0;
    channels[0].release_ms  = 0;
}

void render(uint32_t time) {
    screen.alpha = 255;
    screen.mask = nullptr;

    screen.pen = LED;
    screen.clear();

    screen.pen = {255, 255, 255};
    screen.text(text, minimal_font, Rect(Point(0, 0), screen.bounds), false, TextAlign::center_center);
}

void buzz(uint16_t frequency) {
    channels[0].frequency = frequency;
    channels[0].trigger_attack();
}

void test_buttons() {
    pressed_buttons |= buttons.pressed;

    if(buttons.pressed & Button::A) {
        LED = {255, 0, 0};
        buzz(988);
        text = "A";
        return;
    }

    if(buttons.pressed & Button::B) {
        LED = {0, 255, 0};
        buzz(1047);
        text = "B";
        return;
    }

    if(buttons.pressed & Button::X) {
        LED = {0, 0, 255};
        buzz(1175);
        text = "X";
        return;
    }

    if(buttons.pressed & Button::Y) {
        LED = {255, 0, 0};
        buzz(1319);
        text = "Y";
        return;
    }

    if(buttons.pressed & Button::DPAD_UP) {
        LED = {0, 255, 0};
        buzz(988);
        text = "Up";
        return;
    }

    if(buttons.pressed & Button::DPAD_DOWN) {
        LED = {0, 0, 255};
        buzz(1319);
        text = "Down";
        return;
    }

    if(buttons.pressed & Button::DPAD_LEFT) {
        LED = {255, 0, 0};
        buzz(1175);
        text = "Left";
        return;
    }

    if(buttons.pressed & Button::DPAD_RIGHT) {
        LED = {0, 255, 0};
        buzz(1047);
        text = "Right";
        return;
    }
}

void update(uint32_t time) {
    switch(current_state) {
        case START:
#ifdef PICO_BOARD
            pwm_set_gpio_level(PICOSYSTEM_BACKLIGHT_PIN, 65535);
#endif
            text = "Pico System";
            LED = {0, 255, 0};

            goto_state(TEST_BATTERY, 500);
            break;
        case TEST_BATTERY:
            LED = {255, 0, 0};
            if(gpio_get(VBUS_DETECT_PIN)) {
                goto_state(REMOVE_USB, 100);
            } else {
                uint16_t result = adc_read();
                const float vconf = 3.3f / (1 << 12);
                float battery_voltage = result * vconf * 3.0f;
                if(battery_voltage > 2.5f && battery_voltage < 4.5f) {
                    text = "Battery: PASS!\n" + std::to_string(battery_voltage) + "v";
                    goto_state(TEST_BUTTONS, 500);
                } else {
                    text = "Battery: FAIL!\n" + std::to_string(battery_voltage) + "v";
                    goto_state(FAIL, 500);
                }
            }
            break;
        case REMOVE_USB:
            LED = {255, 0, 0};
            text = "Remove USB cable!";
            goto_state(TEST_BATTERY, 100);
            break;
        case TEST_BUTTONS:
            if(!pressed_buttons) {
                LED = {0, 0, 128};
                text = "Test D-pad\nand ABXY buttons...";
            }
            test_buttons();
            if(pressed_buttons == PICOSYSTEM_ALL_BUTTONS) {
                goto_state(PASS, 500);
            }
            break;
        case FAIL:
            LED = {255, 0, 0};
            text = "FAIL!";
#ifdef PICO_BOARD
            pwm_set_gpio_level(PICOSYSTEM_BACKLIGHT_PIN, 32768 + sin(time >> 8) * 32768);
#endif
            if(buttons.pressed) {
                goto_state(START, 100);
            }
            break;
        case PASS:
            pressed_buttons = 0;
            LED = {0, 128, 0};
            text = "Pass";
#ifdef PICO_BOARD
            pwm_set_gpio_level(PICOSYSTEM_BACKLIGHT_PIN, 32768 + sin(time >> 8) * 32768);
#endif
            if(buttons.pressed) {
                goto_state(START, 100);
            }
            break;
    }



}