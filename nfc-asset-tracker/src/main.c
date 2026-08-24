#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_NUM_2

void app_main() {

    const gpio_config_t gpio2_config = {
        .pin_bit_mask = (1UL << LED_GPIO),   // Bitmask for GPIO 2
        .mode = GPIO_MODE_OUTPUT,              // Output mode
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&gpio2_config);

    printf("Test Serial Output Message\n");

    while(1) {


        printf("Test Message from blink\n");

        // Turn on Onboard LED
       gpio_set_level(LED_GPIO, 1);

        vTaskDelay(pdMS_TO_TICKS(1000));

        // Turn off Onboard LED
        gpio_set_level(LED_GPIO, 0);

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
}