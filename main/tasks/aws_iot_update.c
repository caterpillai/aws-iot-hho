#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#include "core2forAWS.h"
#include "read_hho_measures.h"
#include "aws_iot_update.h"
#include "wifi.h"
#include "ui.h"

#define MAX_LENGTH_OF_JSON_BUFFER 400
#define MAX_LENGTH_OF_NOTIFICATIONS 200
#define CLIENT_ID_LEN (ATCA_SERIAL_NUM_SIZE * 2)

static const char *TAG = "aws_iot_update_task";

/* CA Root certificate */
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");

/* Default MQTT HOST URL (from aws_iot_config.h) */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/* Default MQTT port (from aws_iot_config.h) */
uint32_t mqttPort = AWS_IOT_MQTT_PORT;

static hho_measures_t _hhoMeasures;
static char notificationBuffer[MAX_LENGTH_OF_NOTIFICATIONS] = "No current notifications";
static uint8_t notificationsCount = 0;
static bool _shadowUpdateInProgress;

// JSON Document Buffer and related fields to be initialized.
char JsonDocumentBuffer[MAX_LENGTH_OF_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
jsonStruct_t temperatureHandler;
jsonStruct_t soundHandler;
jsonStruct_t lightHandler;
jsonStruct_t tvocHandler;
jsonStruct_t eCO2Handler;
jsonStruct_t recommendationsHandler;
jsonStruct_t recommendationCountHandler;

void notification_message_callback(const char *pJsonString, uint32_t jsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(jsonStringDataLen);

    char * recommendations = (char *)(pContext->pData);
    ESP_LOGI(TAG, "Updating recommendations with: %s", recommendations);
    UI_Recommendations_Textarea_Update(recommendations);
}

void notification_count_callback(const char *pJsonString, uint32_t jsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(jsonStringDataLen);

    uint8_t newCount = *(uint8_t *) (pContext->pData);
    ESP_LOGI(TAG, "Update recommendations count to %d", newCount);    
    UI_Recommendations_Count_Update(newCount);
}

void initialize_JSON_buffer_fields() {
    // Initialize temperature field
    temperatureHandler.cb = NULL;
    temperatureHandler.pKey = "temperature";
    temperatureHandler.pData = &(_hhoMeasures.temperature);
    temperatureHandler.type = SHADOW_JSON_FLOAT;
    temperatureHandler.dataLength = sizeof(float);

    // Initialize sound field
    soundHandler.cb = NULL;
    soundHandler.pKey = "noiseLevel";
    soundHandler.pData = &(_hhoMeasures.noiseLevel);
    soundHandler.type = SHADOW_JSON_INT8;
    soundHandler.dataLength = sizeof(uint8_t);

    // Initialize the light field
    lightHandler.cb = NULL;
    lightHandler.pKey = "lightIntensity";
    lightHandler.pData = &(_hhoMeasures.lightIntensity);
    lightHandler.type = SHADOW_JSON_INT32;
    lightHandler.dataLength = sizeof(uint32_t);

    // Initialize the tvoc field
    tvocHandler.cb = NULL;
    tvocHandler.pKey = "tvoc";
    tvocHandler.pData = &(_hhoMeasures.tvoc);
    tvocHandler.type = SHADOW_JSON_INT8;
    tvocHandler.dataLength = sizeof(uint8_t);

    // Initialize the eCO2 field
    eCO2Handler.cb = NULL;
    eCO2Handler.pKey = "eCO2";
    eCO2Handler.pData = &(_hhoMeasures.eC02);
    eCO2Handler.type = SHADOW_JSON_INT8;
    eCO2Handler.dataLength = sizeof(uint8_t);

    // Initializes the notifications field
    recommendationsHandler.cb = notification_message_callback;
    recommendationsHandler.pKey = "notifications";
    recommendationsHandler.pData = &notificationBuffer;
    recommendationsHandler.type = SHADOW_JSON_STRING;
    recommendationsHandler.dataLength = MAX_LENGTH_OF_NOTIFICATIONS;

    // Initialize the notifications count field
    recommendationCountHandler.cb = notification_count_callback;
    recommendationCountHandler.pKey = "notificationCount";
    recommendationCountHandler.pData = &notificationsCount;
    recommendationCountHandler.type = SHADOW_JSON_INT8;
    recommendationCountHandler.dataLength = sizeof(uint8_t);
}

void mqtt_disconnect_callback_handler(AWS_IoT_Client *clientPtr, void *data) {
    ESP_LOGW(TAG, "Disconnected from AWS IoT Core");
    UI_Status_Textarea_Add("Disconnected from AWS IoT Core...", NULL, 0);
    
    if (clientPtr == NULL) {
        return;
    }

    if (aws_iot_is_autoreconnect_enabled(clientPtr)) {
        ESP_LOGI(TAG, "Attempting automatic reconnect.");
    } else {
        ESP_LOGW(TAG, "Manually attempting a reconnection.");
        IoT_Error_t rc = aws_iot_mqtt_attempt_reconnect(clientPtr);
        if (rc == NETWORK_RECONNECTED) {
            ESP_LOGI(TAG, "Manual reconnect was successful.");
        } else {
            ESP_LOGW(TAG, "Manual reconnect failed with code: %d", rc);
        }
    }
}

void shadow_update_status_callback(const char *namePtr, ShadowActions_t action,
    Shadow_Ack_Status_t status, const char *responseJSONDocumentPtr, void *contextDataPtr) {
    IOT_UNUSED(namePtr);
    IOT_UNUSED(action);
    IOT_UNUSED(contextDataPtr);

    _shadowUpdateInProgress = false;

    if (status == SHADOW_ACK_TIMEOUT) {
        ESP_LOGE(TAG, "Shadow update timeout.");
    } else if (status == SHADOW_ACK_REJECTED) {
        ESP_LOGE(TAG, "Shadow update rejected.");
    } else if (status == SHADOW_ACK_ACCEPTED) {
        ESP_LOGI(TAG, "Shadow update accepted with response: %s", responseJSONDocumentPtr);
    }
}

void update_task(void *param) {
    AWS_IoT_Client iotCoreClient;

    // Initialize the MQTT client
    ShadowInitParameters_t sp = ShadowInitParametersDefault;
    sp.pHost = HostAddress;
    sp.port = mqttPort;
    sp.enableAutoReconnect = false;
    sp.disconnectHandler = mqtt_disconnect_callback_handler;
    sp.pRootCA = (const char *)aws_root_ca_pem_start;
    sp.pClientCRT = "#";
    sp.pClientKey = "#0";

    // Retrieve the device serial number from the secure element
    char *clientId = malloc(CLIENT_ID_LEN + 1);
    ATCA_STATUS status = Atecc608_GetSerialString(clientId);
    if (status!= ATCA_SUCCESS) {
        ESP_LOGE(TAG, "Failed to retrieve device serial with atca status: %i", status);
        abort();
    }

    UI_Status_Textarea_Add("\nDevice client Id:\n>> %s <<\n", clientId, CLIENT_ID_LEN);

    xEventGroupWaitBits(
        wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
    
    ESP_LOGI(TAG, "Initializing shadow device connection.");
    IoT_Error_t rc = aws_iot_shadow_init(&iotCoreClient, &sp);
    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Shadow init method returned error: %d", rc);
        abort();
    }

    ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
    scp.pMyThingName = clientId;
    scp.pMqttClientId = clientId;
    scp.mqttClientIdLen = CLIENT_ID_LEN;

    rc = aws_iot_shadow_connect(&iotCoreClient, &scp);
    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Shadow connect method returned error: %d", rc);
        abort();
    }

    UI_Status_Textarea_Add("\nConnected to AWS IoT Core and pub/sub to the device shadow state\n", NULL, 0);

    rc = aws_iot_shadow_set_autoreconnect_status(&iotCoreClient, true);
    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Unable to set auto-reconnect to true with error: %d", rc);
        abort();
    }

    // Register callback for recommendations
    rc = aws_iot_shadow_register_delta(&iotCoreClient, &recommendationsHandler);
    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Unable to register callback for recomendations.");
    }

    // Register callback for recommendations count
    rc = aws_iot_shadow_register_delta(&iotCoreClient, &recommendationCountHandler);
    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Unable to register callback for recommendations count.");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));

    while(rc == NETWORK_ATTEMPTING_RECONNECT || 
        rc == NETWORK_RECONNECTED || 
        rc == SUCCESS) {
        rc = aws_iot_shadow_yield(&iotCoreClient, 1000);
        if (rc == NETWORK_ATTEMPTING_RECONNECT || _shadowUpdateInProgress) {
            // Skip the rest of the loop while waiting for a reconnect/pending update
            continue;
        }
        
        _hhoMeasures = Read_HHO_Measures();

        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
        if (rc == SUCCESS) {
            rc = aws_iot_shadow_add_reported(
                JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 7, 
                &temperatureHandler, &soundHandler, &lightHandler, 
                &tvocHandler, &eCO2Handler, &recommendationsHandler,
                &recommendationCountHandler);
            if (rc == SUCCESS) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
                if (rc == SUCCESS) {
                    ESP_LOGI(TAG, "Updating shadow device: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&iotCoreClient, clientId, 
                        JsonDocumentBuffer, shadow_update_status_callback, NULL, 5, true);
                        _shadowUpdateInProgress = true;
                } else {
                    ESP_LOGE(TAG, "Unable to finalize JSON document with error: %d", rc);
                }
            } else {
                ESP_LOGE(TAG, "Unable to add reported data with error: %d", rc);
            }

        } else {
            ESP_LOGE(TAG, "Unable to initialize the JSON message with error: %d", rc);
        }
        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }

    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "An error occured in the loop: %d", rc);
    }
    rc = aws_iot_shadow_disconnect(&iotCoreClient);

    if (rc != SUCCESS) {
        ESP_LOGE(TAG, "Disconnect error: %d", rc);
    } else {
        ESP_LOGI(TAG, "Successfully disconnected.");
    }

    vTaskDelete(NULL);
}

void AWS_IoT_Update_Task_Init(UBaseType_t priority) {
    ESP_LOGI(TAG, "AWS IoT SDK Version %d.%d.%d-%s", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
    
    initialize_JSON_buffer_fields();
    initialise_wifi();

    xTaskCreatePinnedToCore(
        &update_task, TAG, 4096*2, NULL, priority, NULL, 1);
}