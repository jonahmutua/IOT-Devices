#include "Wifi.h"

extern "C" void app_main(void)
{
    esp_err_t status{ESP_OK};
    ESP_LOGI( "main", "Initialsing NVS");
    ESP_ERROR_CHECK(  nvs_flash_init() );

    QueueHandle_t shared_queue = xQueueCreate(10, sizeof( IotData_t ));
    smartconfig_start_config_t  cfg{.enable_log = true, .esp_touch_v2_enable_crypt = false, .esp_touch_v2_key = nullptr};
    SMARTCONFIG::SmartConfig sc(cfg, shared_queue);
     //SMARTCONFIG::SmartConfig sc;
    
     WIFI::Wifi wifi(&sc, shared_queue);
   

    status = wifi.start();
   

   // Block app_main to prevent premature exit
    while (true) {
        vTaskDelay(portMAX_DELAY);
    }
}