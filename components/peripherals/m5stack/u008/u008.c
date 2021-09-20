#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#include "core2forAWS.h"
#include "u008.h"

#define WRITE_DELAY_MS 10

// The device address, buad rate and commands related to the underlying SGP30
// hardware were referenced from:
// https://github.com/adafruit/Adafruit_SGP30/blob/master/Adafruit_SGP30.cpp
#define DEVICE_ADDRESS 0x58
#define BAUD_RATE 115200

static const uint8_t init_commands[] = {0x20, 0x03};
static const uint8_t sensor_read_commands[] = {0x20, 0x08};

static const char *TAG = "M5S-U008";
static SemaphoreHandle_t thread_mutex;
static m5s_u008_readout_t reportedReadout;

void M5S_U008_UpdateReadoutTask() {
    // Initialize peripheral device with the expected buad_rate.
    I2CDevice_t port_A_peripheral = 
        Core2ForAWS_Port_A_I2C_Begin(DEVICE_ADDRESS, BAUD_RATE);
    
    // Initialize the sensor to get non-zero readings.
    esp_err_t init_err = Core2ForAWS_Port_A_I2C_Write(
        port_A_peripheral, I2C_NO_REG, &init_commands, 2);

    if (init_err) {
        ESP_LOGW(
            TAG, 
            "Initialization of %s peripheral failed with error: %s",
            TAG,
            esp_err_to_name(init_err));
    } else {
        ESP_LOGI(TAG, "Successfully connected Gas Sensor to Port A.");
    }

    // TODO: May need to follow the Adafruit example and issue the baselining
    // command + perform those operations before looping over the read/write.

    for(;;){
        // Issue command for TVOC and eCO2 readings
        esp_err_t write_err = 
            Core2ForAWS_Port_A_I2C_Write(
                port_A_peripheral, I2C_NO_REG, &sensor_read_commands, 2);
        if(!write_err) {
            // A minute delay to give the sensor time to respond to the command
            // before reading the result
            vTaskDelay(pdMS_TO_TICKS(WRITE_DELAY_MS));
            
            uint8_t reply[2];
            esp_err_t read_err = 
                Core2ForAWS_Port_A_I2C_Read(port_A_peripheral, I2C_NO_REG, &reply, 2);
            
            if(!read_err) {
                xSemaphoreTake(thread_mutex, portMAX_DELAY);
                reportedReadout.tvoc = reply[0];
                reportedReadout.eC02 = reply[1];
                xSemaphoreGive(thread_mutex);
            } else {
                ESP_LOGW(TAG, "CO2 Read Error: %s", esp_err_to_name(read_err));
            }
        } else {
            ESP_LOGW(TAG, "CO2 Write Error: %s", esp_err_to_name(write_err));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    Core2ForAWS_Port_A_I2C_Close(port_A_peripheral);
    vTaskDelete(NULL);
}

void M5S_U008_Init() {
    thread_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        M5S_U008_UpdateReadoutTask, "M5S_U008_Task", 1024*2, NULL, 1, NULL, 1);
}

m5s_u008_readout_t M5S_U008_GetLatestReadout() {
    xSemaphoreTake(thread_mutex, portMAX_DELAY);
    m5s_u008_readout_t result; 
    result.tvoc = reportedReadout.tvoc;
    result.eC02 = reportedReadout.eC02;
    xSemaphoreGive(thread_mutex);
    return result;
}