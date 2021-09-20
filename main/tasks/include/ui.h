/**
 * @file ui.h
 * @brief Initializes the screens that a user can see on this device and a
 * background task that reacts to button presses.
 */

#pragma once

#include "read_hho_measures.h"

/** Available pages on this device */
typedef enum
{
    DEVICE_LOGS, // 'Status' page displaying user-facing device updates (i.e. connectivity)
    MEASUREMENTS, // 'Measurements' page displaying latest environmental readings
    RECOMMENDATIONS // 'Recommendations' page displaying recommended actions for the user
} Page;

/** Adds a line to the status page. */
void UI_Status_Textarea_Add(char *txt, char *param, size_t paramLen);

/** Updates the current recommendations list. */
void UI_Recommendations_Textarea_Update(char *newTxt);

/** Udpdates the current notifications count. */
void UI_Recommendations_Count_Update(uint8_t newCount);

/** Updates the wifi label based on current connectivity. */
void UI_Wifi_Label_Update(bool state);

/** Updates the measurements page with the latest readings. */
void UI_HHO_Measurements_Update(hho_measures_t measures);

/** Creates the initial UI screens and monitors for button presses. */
void UI_Init();