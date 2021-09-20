#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

#include "core2forAWS.h"
#include "read_hho_measures.h"
#include "ui.h"

#define MAX_TEXTAREA_LENGTH 1024
#define FOOTER_Y_OFFSET -5

Page currentPage = DEVICE_LOGS;

// Components in the measurement screen
static lv_obj_t *measurementScreen;
static lv_obj_t *temperature;
static lv_obj_t *tempLabel;
static lv_obj_t *sound;
static lv_obj_t *soundLabel;
static lv_obj_t *light;
static lv_obj_t *lightLabel;
static lv_obj_t *gasSensor;
static lv_obj_t *gasSensorLabel;
static lv_style_t measureBoxStyle;

// Components in the status screen
static lv_obj_t *statusScreen;
static lv_obj_t *statusTxt;

// Components in the recommendations screen
static lv_obj_t *recommendationsScreen;
static lv_obj_t *recommendationsTxt;

// Labels
static lv_obj_t *wifiLabel;
static lv_obj_t *headerLabel;
static lv_obj_t *statusFooter;
static lv_obj_t *measuresFooter;
static lv_obj_t *recommendationsFooter;

static char *TAG = "UI";

static void ui_textarea_prune(size_t new_text_length)
{
    const char *current_text = lv_textarea_get_text(statusTxt);
    size_t current_text_len = strlen(current_text);
    if (current_text_len + new_text_length >= MAX_TEXTAREA_LENGTH)
    {
        for (int i = 0; i < new_text_length; i++)
        {
            lv_textarea_set_cursor_pos(statusTxt, 0);
            lv_textarea_del_char_forward(statusTxt);
        }
        lv_textarea_set_cursor_pos(statusTxt, LV_TEXTAREA_CURSOR_LAST);
    }
}

void UI_Status_Textarea_Add(char *baseTxt, char *param, size_t paramLen)
{
    if (baseTxt != NULL)
    {
        xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
        if (param != NULL && paramLen != 0)
        {
            size_t baseTxtLen = strlen(baseTxt);
            ui_textarea_prune(paramLen);
            size_t bufLen = baseTxtLen + paramLen;
            char buf[(int)bufLen];
            sprintf(buf, baseTxt, param);
            lv_textarea_add_text(statusTxt, buf);
        }
        else
        {
            lv_textarea_add_text(statusTxt, baseTxt);
        }
        xSemaphoreGive(xGuiSemaphore);
    }
    else
    {
        ESP_LOGE(TAG, "Textarea baseTxt is NULL!");
    }
}

void UI_Recommendations_Textarea_Update(char *newTxt) {
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_textarea_set_text(recommendationsTxt, newTxt);
    xSemaphoreGive(xGuiSemaphore);
}

void UI_Recommendations_Count_Update(uint8_t newCount) {
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    lv_label_set_text_fmt(recommendationsFooter, "%d!", newCount);
    xSemaphoreGive(xGuiSemaphore);
}

void UI_Wifi_Label_Update(bool state)
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (state == false)
    {
        lv_label_set_text(wifiLabel, LV_SYMBOL_WIFI);
    }
    else
    {
        char buffer[25];
        sprintf(buffer, "#0000ff %s #", LV_SYMBOL_WIFI);
        lv_label_set_text(wifiLabel, buffer);
    }
    xSemaphoreGive(xGuiSemaphore);
}

char labelText[40];

/** Updates the measurements screen with the given values. */
void UI_HHO_Measurements_Update(hho_measures_t measures) {
        
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    snprintf(labelText, 40, "Temperature\n-----\n%0.2f F", measures.temperature);
    lv_label_set_text(tempLabel, labelText);

    snprintf(labelText, 40, "Noise Level\n-----\n%d", measures.noiseLevel);
    lv_label_set_text(soundLabel, labelText);

    snprintf(labelText, 40, "Light Level\n-----\n%d lm", measures.lightIntensity);
    lv_label_set_text(lightLabel, labelText);

    snprintf(labelText, 40, "TVOC: %d ppm\n-----\neCO2: %d ppm", measures.tvoc, measures.eC02);
    lv_label_set_text(gasSensorLabel, labelText);

    xSemaphoreGive(xGuiSemaphore);
}

void show_status_page()
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (currentPage != DEVICE_LOGS)
    {
        lv_scr_load_anim(statusScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        lv_label_set_text(headerLabel, "Status");
        currentPage = DEVICE_LOGS;
    }
    xSemaphoreGive(xGuiSemaphore);
}

void show_measurements_page()
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (currentPage != MEASUREMENTS)
    {
        if (currentPage == DEVICE_LOGS)
        {
            lv_scr_load_anim(measurementScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        }
        else
        {
            lv_scr_load_anim(measurementScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
        }
        lv_label_set_text(headerLabel, "Measurements");
        currentPage = MEASUREMENTS;
    }
    xSemaphoreGive(xGuiSemaphore);
}

void show_recommendations_page()
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);
    if (currentPage != RECOMMENDATIONS)
    {
        lv_scr_load_anim(recommendationsScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
        lv_label_set_text(headerLabel, "Recommendations");
        currentPage = RECOMMENDATIONS;
    }
    xSemaphoreGive(xGuiSemaphore);
}

/** 
 * Background task that redraws the UI when a new page is selected via the
 * virtual buttons. 
 */
void _ui_task(void *params)
{
    vTaskDelay(pdMS_TO_TICKS(5000));
    for (;;)
    {
        if (Button_WasPressed(button_left))
        {
            show_status_page();
        }
        else if (Button_WasPressed(button_middle))
        {
            show_measurements_page();        
        }
        else if (Button_WasPressed(button_right))
        {
            show_recommendations_page();
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelete(NULL);
}

/**
 * Initialize the layer containing the labels and icons that are always present
 * regardless of which screen is being displayed (e.g. the wi-fi label).
 */
void initialize_top_layer()
{
    // Create the footer labels
    statusFooter = lv_label_create(lv_layer_top(), NULL);
    lv_obj_align(statusFooter, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 45, FOOTER_Y_OFFSET);
    lv_label_set_text(statusFooter, LV_SYMBOL_SETTINGS);
    lv_label_set_recolor(statusFooter, true);

    measuresFooter = lv_label_create(lv_layer_top(), NULL);
    lv_obj_align(measuresFooter, NULL, LV_ALIGN_IN_BOTTOM_MID, 8, FOOTER_Y_OFFSET);
    lv_label_set_text(measuresFooter, LV_SYMBOL_HOME);
    lv_label_set_recolor(measuresFooter, true);

    recommendationsFooter = lv_label_create(lv_layer_top(), NULL);
    lv_obj_align(recommendationsFooter, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -22, FOOTER_Y_OFFSET);
    lv_label_set_text(recommendationsFooter, "!");
    lv_label_set_recolor(measuresFooter, true);

    // Create the header labels
    wifiLabel = lv_label_create(lv_layer_top(), NULL);
    lv_obj_align(wifiLabel, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 6);
    lv_label_set_text(wifiLabel, LV_SYMBOL_WIFI);
    lv_label_set_recolor(wifiLabel, true);

    headerLabel = lv_label_create(lv_layer_top(), NULL);
    lv_obj_align(headerLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
}

void initialize_measurement_page()
{
    // Create the measurements display screen
    measurementScreen = lv_obj_create(NULL, NULL);

    // Initialize shared style between measurement boxes
    lv_style_init(&measureBoxStyle);
    lv_style_set_radius(&measureBoxStyle, 0, 10);
    lv_style_set_bg_opa(&measureBoxStyle, 0, LV_OPA_COVER);
    lv_style_set_bg_color(&measureBoxStyle, 0, LV_COLOR_WHITE);
    lv_style_set_border_color(&measureBoxStyle, 0, LV_COLOR_BLUE);
    lv_style_set_border_width(&measureBoxStyle, 0, 2);
    lv_style_set_border_opa(&measureBoxStyle, 0, LV_OPA_30);
    lv_style_set_border_side(&measureBoxStyle, 0, LV_BORDER_SIDE_FULL);

    // Create measurement boxes
    temperature = lv_obj_create(measurementScreen, NULL);
    lv_obj_add_style(temperature, 0, &measureBoxStyle);
    lv_obj_set_size(temperature, 140, 70);
    lv_obj_align(temperature, NULL, LV_ALIGN_IN_TOP_LEFT, 15, 45);
    tempLabel = lv_label_create(temperature, NULL);
    lv_obj_align(tempLabel, NULL, LV_ALIGN_IN_TOP_LEFT, 10, 10);
    lv_label_set_recolor(tempLabel, true);
    lv_label_set_text(tempLabel, "Temperature\n-----");

    sound = lv_obj_create(measurementScreen, temperature);
    lv_obj_align(sound, NULL, LV_ALIGN_IN_TOP_RIGHT, -15, 45);
    soundLabel = lv_label_create(sound, tempLabel);
    lv_label_set_text(soundLabel, "Noise Level\n-----");

    light = lv_obj_create(measurementScreen, temperature);
    lv_obj_align(light, NULL, LV_ALIGN_IN_LEFT_MID, 15, 45);
    lightLabel = lv_label_create(light, tempLabel);
    lv_label_set_text(lightLabel, "Light Level\n-----");

    gasSensor = lv_obj_create(measurementScreen, temperature);
    lv_obj_align(gasSensor, NULL, LV_ALIGN_IN_RIGHT_MID, -15, 45);
    gasSensorLabel = lv_label_create(gasSensor, tempLabel);
    lv_label_set_text(gasSensorLabel, "TVOC: \n-----\neCO2: ");
}

void UI_Init(UBaseType_t priority)
{
    xSemaphoreTake(xGuiSemaphore, portMAX_DELAY);

    // Create the header and footer icons/labels
    initialize_top_layer();

    // Create the status screen
    statusScreen = lv_obj_create(NULL, NULL);
    statusTxt = lv_textarea_create(statusScreen, NULL);
    lv_obj_set_size(statusTxt, 300, 180);
    lv_obj_align(statusTxt, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, -27);
    lv_textarea_set_max_length(statusTxt, MAX_TEXTAREA_LENGTH);
    lv_textarea_set_text_sel(statusTxt, false);
    lv_textarea_set_cursor_hidden(statusTxt, true);
    lv_textarea_set_text(statusTxt, "[Healthy Home Office]\n");
    
    // Start on the newly created status screen
    lv_label_set_text(headerLabel, "Status");
    lv_scr_load(statusScreen);

    // Create the recommendations screen
    recommendationsScreen = lv_obj_create(NULL, NULL);
    recommendationsTxt = lv_textarea_create(recommendationsScreen, statusTxt);
    lv_textarea_set_text(recommendationsTxt, "No current recommendations\n");

    initialize_measurement_page();

    xSemaphoreGive(xGuiSemaphore);

    // Create task to listen to button presses and switch screens.
    xTaskCreatePinnedToCore(&_ui_task, "ui_task", 4096 * 2, NULL, priority, NULL, 1);
}