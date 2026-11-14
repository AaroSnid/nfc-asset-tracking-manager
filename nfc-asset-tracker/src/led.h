
#ifndef LED_H_
#define LED_H_
 
#include <stdbool.h>
 
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
 
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    LED_PATTERN_STATIC,     // Solid colour at full brightness
    LED_PATTERN_PULSING,    // Smooth fade handled in hardware 
    LED_PATTERN_FLASHING    // On/Off flashing
} led_pattern_t;
 
/* FreeRTOS task entry point 
 * Handles hardware configuration, then runs the pattern loop. 
 * Deletes itself on initialisation failure 
 */
void led_task(void *arg);
 
/*
 * Note functions below not ISR-safe
 */
 
/* Set the three color pins atomically with respect to other callers. */
int led_set_rgb(bool red, bool green, bool blue);
 
int led_set_red(void);
int led_set_green(void);
int led_set_blue(void);
int led_set_yellow(void);
int led_set_white(void);
 
/*
 * Notify LED task to switch the common-pin pattern 
 */
int led_set_pattern(led_pattern_t pattern);

#ifdef __cplusplus
}
#endif

#endif /* LED_H_ */