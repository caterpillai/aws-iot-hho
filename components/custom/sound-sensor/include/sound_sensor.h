/**
 * @file sound-sensor.h
 * @brief A custom wrapper around the core2forAWS `Microphone` component that
 * performs simple transformations on read values.
 * 
 * This is the exact same functionality provided in the `Smart-Thermostat`
 * `microphone-task` example (see: 
 * https://github.com/m5stack/Core2-for-AWS-IoT-EduKit/blob/master/Smart-Thermostat/main/main.c).
 * The separation was performed to make main and the sensor reading task
 * a little cleaner and remove the direct dependency of other files on
 * fft-related code.
 */

#pragma once
#include <ctype.h>

/**
 * @brief Initializes the sound sensor for reading the maximum volume of the
 * surrounding area.
 * 
 * Creates a FreeRTOS task with the name `SoundSensor_Task`, which reads input
 * from the `Microphone` component provided by core2forAWS, performs a
 * FFT and stores the max of the sample for later reads.
 */
void SoundSensor_Init();

/**
 * @brief Returns the last recorded max volume from the latest microphone sample. 
 * 
 * @return last max recorded volume.
*/
uint8_t SoundSensor_GetVolume();