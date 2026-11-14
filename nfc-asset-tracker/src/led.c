#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "stdbool.h"
#include "driver/ledc.h"
#include "freertos/semphr.h"
 
#include "led.h"

#define LED_PIN_RED     GPIO_NUM_25
#define LED_PIN_GREEN   GPIO_NUM_26
#define LED_PIN_BLUE    GPIO_NUM_27
#define LED_PIN_CATHODE GPIO_NUM_32

#define LED_MUTEX_TIMEOUT_MS        10
#define LED_FLASH_HALF_PERIOD_MS    250
#define LED_PULSE_HALF_PERIOD_MS    800
#define LED_FADE_MARGIN_MS          20
#define LED_DUTY_MAX        ((1U << LEDC_TIMER_10_BIT) - 1U)

static SemaphoreHandle_t led_mutex = NULL;
static TaskHandle_t led_task_handle = NULL;

int led_set_rgb(bool red, bool green, bool blue) {
    if (led_mutex == NULL) return -1;

    // Mutex used to ensure color change atomicity
    if (xSemaphoreTake(led_mutex, portMAX_DELAY) == pdTRUE) {

        gpio_set_level(LED_PIN_RED, red);
        gpio_set_level(LED_PIN_GREEN, green);
        gpio_set_level(LED_PIN_BLUE, blue);

        xSemaphoreGive(led_mutex);
        return 0;
    } 
    return -2;
}

int led_set_red(void) {
    return led_set_rgb(1U, 0U, 0U);
}

int led_set_green(void) {
    return led_set_rgb(0U, 1U, 0U);
}

int led_set_blue(void) {
    return led_set_rgb(0U, 0U, 1U);
}

int led_set_yellow(void) {
    return led_set_rgb(1U, 1U, 0U);
}

int led_set_white(void) {
    return led_set_rgb(1U, 1U, 1U);
}

static TickType_t led_wait_ticks(led_pattern_t pattern) {
    // Half period chosen for pattern two state cycle
    switch (pattern) {
        case LED_PATTERN_FLASHING:
            return pdMS_TO_TICKS(LED_FLASH_HALF_PERIOD_MS);
        case LED_PATTERN_PULSING:
            return pdMS_TO_TICKS(LED_PULSE_HALF_PERIOD_MS + LED_FADE_MARGIN_MS);
        case LED_PATTERN_STATIC:
        default:
            // sleep until notification received
            return portMAX_DELAY;
    }
}

int led_set_pattern(led_pattern_t pattern) {
    if (led_task_handle == NULL) return -1;
    return (xTaskNotify(led_task_handle, (uint32_t)pattern, eSetValueWithOverwrite) == pdPASS) ? 0 : -1;
}

static int led_set_duty(uint32_t duty) {
    if (ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty) != ESP_OK) return -1;
    if (ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0)    != ESP_OK) return -1;
    return 0;
}

static int led_pattern_step(led_pattern_t pattern, bool *bright) {
    *bright = !*bright;
    uint32_t target = *bright ? LED_DUTY_MAX : 0U;
 
    switch (pattern) {
        case LED_PATTERN_FLASHING:
            return led_set_duty(target);
        case LED_PATTERN_PULSING:
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, target, LED_PULSE_HALF_PERIOD_MS);
            ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT); // LEDC_FADE_NO_WAIT since task must not block
            return 0;
        case LED_PATTERN_STATIC:
        default:
            return 0;
    }
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
    gpio_set_level(LED_PIN_RED, 0);
    gpio_set_level(LED_PIN_GREEN, 0);
    gpio_set_level(LED_PIN_BLUE, 0);

    const ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
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
        .duty           = LED_DUTY_MAX,
        .hpoint         = 0,
        .flags          = { .output_invert = 1 },
        .intr_type      = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    if (ledc_fade_func_install(0) != ESP_OK) return -1;

    led_mutex = xSemaphoreCreateMutex();
    if (led_mutex == NULL) return -1;

    return 0;
}

void led_task(void* arg) {
    int status = led_set_config();
    if (status == 0) {
        led_task_handle = xTaskGetCurrentTaskHandle();
    } 
    else {
        vTaskDelete(NULL);
        return;
    }
 
    led_pattern_t pattern = LED_PATTERN_STATIC;
    bool bright = true;
    for (;;) {
        uint32_t notification;
 
        if (xTaskNotifyWait(0x00, UINT32_MAX, &notification, led_wait_ticks(pattern)) == pdTRUE) {
            // Notification received, change pattern

            // Out of bounds pattern handled in default cases
            pattern = (led_pattern_t)notification;
 
            led_set_duty(LED_DUTY_MAX);
            bright = true;
 
            // Pulsing starts fading down immediately
            if (pattern == LED_PATTERN_PULSING) {
                led_pattern_step(pattern, &bright); // TODO: Add task failure signaling
            }
        
        } else {
            // Notification wait timed out, advance pattern
            led_pattern_step(pattern, &bright); // TODO: Add task failure signaling
        }
    }
}