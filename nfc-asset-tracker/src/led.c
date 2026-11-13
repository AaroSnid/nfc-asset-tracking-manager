#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdbool.h"
#include "driver/ledc.h"

#define LED_PIN_RED     GPIO_NUM_25
#define LED_PIN_GREEN   GPIO_NUM_26
#define LED_PIN_BLUE    GPIO_NUM_27
#define LED_PIN_CATHODE GPIO_NUM_32

// Control the duty cycle on the common pin
typedef enum {
    LED_PATTERN_STATIC,
    LED_PATTERN_PULSING,
    LED_PATTERN_FLASHING
} led_pattern_t;

static SemaphoreHandle_t led_mutex;

int led_set_rgb(bool red, bool green, bool blue) {
    if (led_mutex == NULL) return -1;

    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {

        gpio_set_level(LED_PIN_RED, red);
        gpio_set_level(LED_PIN_GREEN, green);
        gpio_set_level(LED_PIN_BLUE, blue);

        xSemaphoreGive(led_mutex);
        return 0;
    } 
    return -2;
}

inline int led_set_red(void) {
    return led_set_rgb(1U, 0U, 0U);
}

inline int led_set_green(void) {
    return led_set_rgb(0U, 1U, 0U);
}

inline int led_set_blue(void) {
    return led_set_rgb(0U, 0U, 1U);
}

inline int led_set_yellow(void) {
    return led_set_rgb(1U, 1U, 0U);
}

inline int led_set_white(void) {
    return led_set_rgb(1U, 1U, 1U);
}

int led_set_pattern(led_pattern_t pattern) {
   
    // Signal led control task
    return -1;
}

int led_set_config(void) {
    const gpio_config_t led_color_config = {
        .pin_bit_mask = (1ULL << LED_PIN_RED) | (1ULL << LED_PIN_GREEN) | (1ULL << LED_PIN_BLUE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&led_color_config));

    const ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_13_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = (4000),  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    const ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .gpio_num       = LED_PIN_CATHODE,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) return -1;

    return 0;
}