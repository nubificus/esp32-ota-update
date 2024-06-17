#include "ota.h"
#include "esp_ota_ops.h"
#include "tls.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "ota";

static esp_partition_t* update_partition = NULL;
static esp_ota_handle_t update_handle = 0;
static size_t partition_data_written  = 0;

static void ota_process_begin() {
    update_partition = esp_ota_get_next_update_partition(NULL);
    assert(update_partition != NULL);

    esp_err_t err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin() failed (%s)", esp_err_to_name(err));
	esp_ota_abort(update_handle);
	
	while(1) vTaskDelay(1000);
    }
	
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
}

static void ota_append_data_to_partition(unsigned char* data, size_t len) {

    if (esp_ota_write(update_handle, (const void*) data, len) != ESP_OK) {
        esp_ota_abort(update_handle);
	ESP_LOGE(TAG, "esp_ota_write() failed");
	
	while(1) vTaskDelay(1000);
    }

    partition_data_written += len;
}

static void ota_setup_partition_and_reboot() {
	
    ESP_LOGI(TAG, "Total bytes read: %d", partition_data_written);
    
    esp_err_t err = esp_ota_end(update_handle);
    
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) 
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
	else 
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        
	while (1) vTaskDelay(1000);
    }
	
    err = esp_ota_set_boot_partition(update_partition);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
	while (1) vTaskDelay(1000);
    }
	
    ESP_LOGI(TAG, "Prepare to restart system!");

    vTaskDelay(4000 / portTICK_PERIOD_MS);
	
    esp_restart();
}

static int ota_write_partition_from_tls_stream() {
    unsigned char rx_buffer[1024];

    tls_start();

    while (1) {
	memset(rx_buffer, 0, sizeof(rx_buffer));

    	int len = tls_next_chunk(rx_buffer);

	if (len <= 0) {
		break;	
	}
	else {
	    ota_append_data_to_partition(rx_buffer, len);
	}
    }

    tls_terminate();

    ESP_LOGI(TAG, "Now all the data has been received");
    
    return 1;
}

static void ota_service(void *pvParameters) {
    ota_process_begin();
    
    if (ota_write_partition_from_tls_stream() < 0) {
        ESP_LOGE(TAG, "Failed to update");
	while (1) vTaskDelay(1000);
    }
	
    ota_setup_partition_and_reboot();
}

void ota_service_start() {
    xTaskCreate(&ota_service, "OTAServiceTask", 8192, NULL, 5, NULL);
}
