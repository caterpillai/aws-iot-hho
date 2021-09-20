/**
 * @file aws_iot_update.h
 * @brief Initialize a task that sends the most recent measurements to an AWS
 * IoT shadow device via MQTT.
 */

#pragma once

#include <ctype.h>

/**
 * @brief Initializes a task that periodically reads the latest HHO measures
 * and sends that data via MQTT to an AWS IoT shadow device.
 * 
 * @param priority of the task (lower values indicate a lower priority).
 */
void AWS_IoT_Update_Task_Init(UBaseType_t priority);