/**
 * @file main.c
 * @brief Collects measurements using the built-in sensors in the AWS IoT
 * EduKit and peripherals to measure Healthy Home Office [HHO] metrics and
 * publish them to AWS IoT services (via MQTT) to generate recommendations
 * that user's can use to improve their office space.
 *
 * @note based on the Smart Thermometer (v1.2.0) example from
 * https://edukit.workshop.aws/en/
 */

#include "esp_log.h"

#include "core2forAWS.h"
#include "read_hho_measures.h"
#include "aws_iot_update.h"
#include "ui.h"

void app_main()
{   
    Core2ForAWS_Init();
    Core2ForAWS_Display_SetBrightness(50);
    // Indicate that the device is on and recording.
    Core2ForAWS_LED_Enable(1);

    UI_Init(3);
    Read_HHO_Measures_Task_Init(2);
    AWS_IoT_Update_Task_Init(1);
}
