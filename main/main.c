#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi.h"
#include "ota.h"

static const char *TAG = "main";

void app_main(void) {
    esp_err_t status = WIFI_FAILURE;
    
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
	ret = nvs_flash_init();
    }
    
    ESP_ERROR_CHECK(ret);
  
    status = connect_wifi("ssid", "password");
    
    if (WIFI_SUCCESS != status) {
        ESP_LOGI(TAG, "Failed to associate to AP, dying...");
	return;
    }

    ota_service_start();

    ESP_LOGI(TAG, "OTA Service started, Current Firmware Image is running");

    while (1) vTaskDelay(10);
}
