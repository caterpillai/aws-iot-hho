#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "core2forAWS.h"

#include "fft.h"

#define AUDIO_TIME_SLICES 60

static const char *TAG = "SoundSensor";
static SemaphoreHandle_t thread_mutex;
static uint8_t reportedSound;

// Helper function for working with audio data
long map(long x, long in_min, long in_max, long out_min, long out_max) {
    long divisor = (in_max - in_min);
    if(divisor == 0){
        return -1; //AVR returns -1, SAM returns 0
    }
    return (x - in_min) * (out_max - out_min) / divisor + out_min;
}

// Underlying task for this process.
void microphone_task() {
    static int8_t i2s_readraw_buff[1024];
    size_t bytesread;
    int16_t *buffptr;
    double data = 0;

    Microphone_Init();
    uint8_t maxSound = 0x00;
    uint8_t currentSound = 0x00;

    for (;;) {
        maxSound = 0x00;
        fft_config_t *real_fft_plan = 
            fft_init(512, FFT_REAL, FFT_FORWARD, NULL, NULL);
        i2s_read(I2S_NUM_0, (char *)i2s_readraw_buff, 1024, &bytesread, pdMS_TO_TICKS(100));
        buffptr = (int16_t *)i2s_readraw_buff;
        for (uint16_t count_n = 0; count_n < real_fft_plan->size; count_n++) {
            real_fft_plan->input[count_n] = (float)map(buffptr[count_n], INT16_MIN, INT16_MAX, -1000, 1000);
        }
        fft_execute(real_fft_plan);

        for (uint16_t count_n = 1; count_n < AUDIO_TIME_SLICES; count_n++) {
            data = sqrt(real_fft_plan->output[2 * count_n] * real_fft_plan->output[2 * count_n] + real_fft_plan->output[2 * count_n + 1] * real_fft_plan->output[2 * count_n + 1]);
            currentSound = map(data, 0, 2000, 0, 256);
            if(currentSound > maxSound) {
                maxSound = currentSound;
            }
        }
        fft_destroy(real_fft_plan);

        // Store max of sample in semaphore
        xSemaphoreTake(thread_mutex, portMAX_DELAY);
        reportedSound = maxSound;
        xSemaphoreGive(thread_mutex);
        
        // Minor delay to avoid bumping into other tasks
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}

void SoundSensor_Init() {
    thread_mutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        &microphone_task, "SoundSensor_Task", 4096, NULL, 1, NULL, 1);
}

uint8_t SoundSensor_GetVolume() {
    xSemaphoreTake(thread_mutex, portMAX_DELAY);
    uint8_t result = reportedSound;
    xSemaphoreGive(thread_mutex);
    return result;
}