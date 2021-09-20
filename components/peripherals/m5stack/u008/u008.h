/**
 * @file u008.h
 * @brief Initialize and read operations for the M5Stack u008 TVOC/eC02 sensor
 * unit connected via GPIO in Port A (I2C).
 * 
 * @note The M5Stack docs were not explicit as to how to do this properly, so I
 * used https://github.com/adafruit/Adafruit_SGP30/blob/master/Adafruit_SGP30.cpp
 * as a reference.
 */

#pragma once
#include <ctype.h>

/** Represents a labeled readout from this sensor. */
typedef struct {
    uint8_t tvoc;
    uint8_t eC02;
} m5s_u008_readout_t;

/**
 * @brief Initializes the sensor using I2C.
 * 
 * @note Creates a FreeRTOS task with the name `M5S_U008_Task`.
 * 
 * The created FreeRTOS task performs a period write command + subsequent read.
*/
void M5S_U008_Init();

/**
 * @brief Read the latest set of values returned from this sensor.
 * 
 * @returns the latest set of readouts from this sensor.
*/
m5s_u008_readout_t M5S_U008_GetLatestReadout();
