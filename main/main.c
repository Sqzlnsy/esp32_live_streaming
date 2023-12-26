#define CONFIG_CAMERA_MODULE_ESP_S3_EYE 1

#include <esp_system.h>
#include <nvs_flash.h>
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

#include "esp_camera.h"
#include "camera_pins.h"
#include "connect_wifi.h"
#include "protocol_examples_common.h"
#include "file_serving_example_common.h"

static const char *TAG = "esp32-cam Webserver";

const char* base_path = "/data";

// Sensor Data Processing Task
void sensor_data_processing_task(void *pvParameter) {
    while (1) {
        ESP_LOGI(TAG, "Processing sensor data...");
        // Add sensor data processing logic here

        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for demonstration
    }
}

// HTTP Server Task
void http_server_task(void *pvParameter) {
    setup_server(base_path);
    ESP_LOGI(TAG, "ESP32 CAM Web Server is up and running\n");
    while (1) {
        ESP_LOGI(TAG, "Server running...");
        // Add video streaming logic here

        vTaskDelay(pdMS_TO_TICKS(10000)); // Delay for demonstration
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

    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize file storage */
    ESP_ERROR_CHECK(example_mount_storage(base_path));

    connect_wifi();

    if (wifi_connect_status)
    {
        err = init_camera();
        if (err != ESP_OK)
        {
            printf("err: %s\n", esp_err_to_name(err));
            return;
        }
    }
    else
        ESP_LOGI(TAG, "Failed to connected with Wi-Fi, check your network Credentials\n");

    // Sensor Data Processing Task
    if (xTaskCreate(sensor_data_processing_task, "sensor_data_processing", 4069, NULL, 10, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor data processing task");
    }

    // HTTP Server Task
    if (xTaskCreate(http_server_task, "http_server", 4096, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create HTTP server task");
    }

    if (start_stream() != ESP_OK){
        return;
    }
}
