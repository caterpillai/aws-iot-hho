/**
 * @file wifi.h
 * @brief Defines helper functions that initialize a wifi connection that
 * allows this device to connect to AWS IoT services.
 * 
 * @note: Based on the AWS IoT EduKit - Smart Thermostat v1.2.0 file by
 * the same name (`wifi.h`)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_event.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event */
#define CONNECTED_BIT BIT0
#define DISCONNECTED_BIT BIT1

/* Initializes the underlying esp_wifi connection */
void initialise_wifi(void);