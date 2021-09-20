#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "core2forAWS.h"
#include "rb_mst_30.h"
#include "u008.h"
#include "sound_sensor.h"
#include "read_hho_measures.h"
#include "ui.h"

static const char *TAG = "read_hho_measures_task";
static SemaphoreHandle_t thread_mutex;
static hho_measures_t recordedMeasurements;

float temperature;

float getTemperature() {
    // Sample temperature
    MPU6886_GetTempData(&temperature);
    // Convert to Fahrenheit (formula from the code sample)
    return (temperature * 1.8) + 32 - 50;
}

void read_task(void *param) {

    vTaskDelay(pdMS_TO_TICKS(1000));
    
    for (;;) {
        xSemaphoreTake(thread_mutex, portMAX_DELAY);
        
        m5s_u008_readout_t gasSensorResult = M5S_U008_GetLatestReadout();
        recordedMeasurements.lightIntensity = M5S_RBMST30_ReadMilliVolts();
        recordedMeasurements.noiseLevel = SoundSensor_GetVolume();
        recordedMeasurements.temperature = getTemperature();
        recordedMeasurements.tvoc = gasSensorResult.tvoc;
        recordedMeasurements.eC02 = gasSensorResult.eC02;

        ESP_LOGI(TAG, "Recorded HHO data: {light:%d temp:%f sound:%d tvoc:%d eC02:%d}", 
                recordedMeasurements.lightIntensity, 
                recordedMeasurements.temperature,
                recordedMeasurements.noiseLevel,
                recordedMeasurements.tvoc,
                recordedMeasurements.eC02);

        // Update UI to reflect the most recent recorded measures
        UI_HHO_Measurements_Update(recordedMeasurements);

        xSemaphoreGive(thread_mutex);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void Read_HHO_Measures_Task_Init(UBaseType_t priority) {
    // Initialize the expected sensor tasks. Assumes Core2ForAWS was already
    // initialized.
    #if CONFIG_SOFTWARE_M5S_RBMST30_SUPPORT && CONFIG_SOFTWARE_M5S_U008_SUPPORT
    M5S_RBMST30_Init();
    M5S_U008_Init();
    SoundSensor_Init();
    #else
    ESP_LOGE(TAG, "Couldn't initialize the peripherals for HHO measures.");
    return;
    #endif
    
    thread_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(&read_task, TAG, 4096*2, NULL, priority, NULL, 1);
}

// TODO(caterpillai): Consider using a queue (https://www.freertos.org/a00118.html). 
hho_measures_t Read_HHO_Measures() {
    xSemaphoreTake(thread_mutex, portMAX_DELAY);
    hho_measures_t result = recordedMeasurements;
    xSemaphoreGive(thread_mutex);
    return result;
}
