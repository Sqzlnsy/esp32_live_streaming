#define CONFIG_CAMERA_MODULE_ESP_S3_EYE 1
#include <stdio.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_camera.h"
#include "camera_pins.h"
#include "connect_wifi.h"
#include "protocol_examples_common.h"
#include "file_serving_example_common.h"
#include "spsc_rb.h"

// Constants for test
#define BUFFER_CAPACITY 1024*4

static const char *TAG = "esp32-cam Webserver";

const char* base_path = "/data";

// Sensor Data Processing Task
static void sensor_data_processing_task(void *pvParameter) {
    RingBuffer *ringBuffer = (RingBuffer *)pvParameter;
    FILE *f;
    char *data = malloc(128);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate memory for data");
        vTaskDelete(NULL); // Terminate the task if memory allocation fails
    }
    while (1) {
        ESP_LOGI(TAG, "Processing sensor data...");
        f = fopen("/data/hello.txt", "a"); // Open file for writing
        if (f == NULL) {
            // Handle file open error
            ESP_LOGE(TAG, "Failed to open file for writing");
        } else {
            if (!readFromBuffer(ringBuffer, data, 128)) {
                printf("Buffer read wrong !!!!!\n");
            } else {
                fprintf(f, "%s", data); // Write data to file
                // fprintf(f, "\n");
            }
            fclose(f); // Close the file
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Delay for demonstration
    }
}

void app_main()
{
    esp_err_t err;

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    RingBuffer *ringBuffer = createRingBuffer(BUFFER_CAPACITY);

    if (ringBuffer == NULL) {
        ESP_LOGE(TAG, "Failed to create data buffer");
        return;
    }

    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize file storage */
    ESP_ERROR_CHECK(example_mount_storage(base_path));

    connect_wifi();

    

    if (wifi_connect_status)
    {
        err = start_stream();
        if (err != ESP_OK)
        {
            printf("err: %s\n", esp_err_to_name(err));
            return;
        }
    }
    else
        ESP_LOGI(TAG, "Failed to connected with Wi-Fi, check your network Credentials\n");

    // Sensor Data Processing Task
    if (xTaskCreate(sensor_data_processing_task, "sensor_data_processing", 4096, (void *)ringBuffer, 15, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor data processing task");
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // HTTP Server Task
    if (xTaskCreate(https_server_task, "https_server", 4096, (void*) base_path, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTPS server task");
    }
     vTaskDelay(pdMS_TO_TICKS(100));

    // Data Link Task
    if (xTaskCreate(data_link_task, "data_link", 10240, (void *)ringBuffer, 12, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Async TCP task");
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (xTaskCreatePinnedToCore(&video_stream_task, "camera_tx", 10240, NULL, 15, NULL, tskNO_AFFINITY) != pdPASS){
        ESP_LOGE(TAG, "Failed to create video stream task");
    } 

    monitor_CPU();
}