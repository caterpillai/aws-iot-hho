/**
 * @file rb_mst_30.h 
 * @brief Initialize and read operations for the M5Stack rb-mst-30 Light Sensor
 *  unit connected via GPIO in Port B (ADC).
 */

#pragma once

#include <ctype.h>

/**
 * @brief Initializes the RBMST30 using ADC read rotocol.
 * 
 * @note Creates a FreeRTOS task with the name `M5S_RBMST30_Task`.
 * 
 * The FreeRTOS task performs a passive read of the light sensor at a set 
 * sampling rate, storing the latest value.
 */
void M5S_RBMST30_Init();

/**
 * @brief Reads the latest value returned by the light sensor (if any).
 * 
 * @return The latest value returned by the light sensor.
*/
uint32_t M5S_RBMST30_ReadMilliVolts();