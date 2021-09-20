#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "core2forAWS.h"

#define VIN 3300


static const char *TAG = "M5S-RB-MST-30"; 
static SemaphoreHandle_t thread_mutex; 
static uint32_t reportedIntensityMilliVolts;


void M5S_RBMST30_UpdateTask() {
    esp_err_t err = Core2ForAWS_Port_PinMode(PORT_B_ADC_PIN, ADC);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Could not connect to Port B with ESP_ERROR: %d", err);
    } else {
        ESP_LOGI(TAG, "Successfully connected Light Sensor to Port B.");
    }

    for(;;) {
        xSemaphoreTake(thread_mutex, portMAX_DELAY);
        // Need to adjust photoresistance raw value to a more useful human measure (lu)
        // https://www.aranacorp.com/en/luminosity-measurement-with-a-photoresistor/
        reportedIntensityMilliVolts = Core2ForAWS_Port_B_ADC_ReadMilliVolts();
        xSemaphoreGive(thread_mutex);

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    vTaskDelete(NULL);
}

void M5S_RBMST30_Init() {
    thread_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        M5S_RBMST30_UpdateTask, "M5S_RBMST30_TASK", 1024*4, NULL, 1, NULL, 1);
}

uint32_t M5S_RBMST30_ReadMilliVolts() {
    xSemaphoreTake(thread_mutex, portMAX_DELAY);
    uint32_t result = reportedIntensityMilliVolts;
    xSemaphoreGive(thread_mutex);
    return result;
}

