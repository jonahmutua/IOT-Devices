#include "Wifi.h"

extern "C" void app_main(void)
{
    esp_err_t status{ESP_OK};
    ESP_LOGI( "main", "Initialsing NVS");
    status =  nvs_flash_init();

    QueueHandle_t shared_queue = xQueueCreate(10, sizeof( IotData_t ));
    smartconfig_start_config_t  cfg{.enable_log = true, .esp_touch_v2_enable_crypt = false, .esp_touch_v2_key = nullptr};
    SMARTCONFIG::SmartConfig sc(cfg, shared_queue);
     //SMARTCONFIG::SmartConfig sc;
    
    ///> provid an instance of smartconfig and shared_queue( wifi & smartconfig) to wifi instance
     WIFI::Wifi wifi(&sc, shared_queue);
   
    ///> Call wifi.start() to start wifi & smartconfig services
    if( ESP_OK == status )
    {
        ///> Starting wifi service
        status = wifi.start();
    }
    
    if( ESP_OK != status)
    {
        ///> in the process of setting up things, we have encountered an error .
        ESP_LOGI("main", "Set up failed: %s", esp_err_to_name ( status ));
        return;
    }
   

   // Block app_main to prevent premature exit
    while (true) {
        ///> You can do other interesting stuff here has wifi handles connectivity in its own task.
        vTaskDelay(portMAX_DELAY);
    }
}