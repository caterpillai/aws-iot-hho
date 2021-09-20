/**
 * @file read_hho_measures.h
 * @brief Initialize a task that records the "Healthy Home Office" measures
 * every second and publishes the results to a queue for other tasks to
 * consume.
 */

#pragma once

#include <ctype.h>
#include "freertos/task.h"

/**
 * Represents the expected collection of measures captured by this task.
*/
typedef struct _hho_measures_t {
    float temperature;
    uint8_t noiseLevel;
    uint32_t lightIntensity;
    uint8_t tvoc;
    uint8_t eC02;
} hho_measures_t;

/**
 * @brief Initializes the task to aggregate the various HHO measures.
 * 
 * @param priority of the task (lower values indicate lower priorities).
 */
void Read_HHO_Measures_Task_Init(UBaseType_t priority);

/** @brief Returns the latest recorded HHO measures. */
hho_measures_t Read_HHO_Measures();