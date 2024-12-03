#ifndef WIFI_H
#define WIFI_H


#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_smartconfig.h"
#include "nvs_flash.h"

#include  "esp_log.h"
#include  "esp_netif.h"
#include  "esp_wifi.h"

#include "TaskCommons.h"
#include "CommInterface.h"
#include "SmartConfig.h"

namespace WIFI
{
///> Wifi class 
class Wifi : public CommInterface
{
  public:
    ///> Enum for wifi state machine 
    enum  class  wifi_state_e 
    {
      WIFI_NOT_INITIALISED,
      WIFI_INITIALISED,
      WIFI_READY_TO_CONNECT,
      WIFI_WAITING_FOR_IP,
      WIFI_CONNECTED,
      WIFI_DISCONNECTED
    };
    /************************************* CONSTRUCTERS / DESTRUCTORS---------------------------------------- */
    Wifi(SMARTCONFIG::SmartConfig* user_sc = nullptr, QueueHandle_t queue_handle = nullptr) 
    {
      SMARTCONFIG::SmartConfig* my_sc = user_sc;
      sc = my_sc; 

      QueueHandle_t handle = queue_handle;
      _queue_handle = handle;
    }
    
    
    ~Wifi()
    {

    }
    /*------------------------------------PUBLIC METHODS------------------------------------------------------*/
    esp_err_t start() override;
    esp_err_t stop() override;

    ///> get wifi state machine 
    wifi_state_e getWifiState();

     private:
  /*---------------------------------------------PRIVATE MEMBER VARIABLES--------------------------------------------------------------*/
      ///> keep static vars here 
       static wifi_state_e _wifi_state;                        ///> tracks Wifi state machine
       constexpr static const char* _log_tag{"myWifi"};        ///> logging Tag
       static SemaphoreHandle_t _wifi_init_done;               ///> semaphore to signal wifi initiation completion.
       static SemaphoreHandle_t _wifi_state_mutex;             ///> Mutex to lock & unlock _wifi_state.
       static SMARTCONFIG::SmartConfig* sc ;                   ///> Smart config reference. ToDo: guard through semaphore
       static QueueHandle_t _queue_handle ;                    ///> Queue handle to wifi queue
 /*----------------------------------------------STATIC PRIVATE MEMBER METHODS-------------------------------------------*/
        ///> Wifi Task Function
        static void wifi_task(void *pv);

        ///> Creates mutex for protecting wifi state machine 
        static esp_err_t create_wifi_state_mutex();

        /// @brief    a mutex used to lock against wifi state machine
        /// @return   ESP_OK:success, ESP_ERR_INVALID_STATE
        static esp_err_t lock_wifi_state();
        
        ///> mutex Unlocks Wifi state machine 
        static esp_err_t unlock_wifi_state();

        ///> creates Initialization semaphore used  to synchronise threads during wifi initiation stage
        static esp_err_t create_wifi_initialisation_semaphore();

        ///> used to signal begining of wifi initiation process. 
        static esp_err_t take_wifi_initialisation_semaphore();

        ///> used to signal end of wifi initiation.
        static esp_err_t give_wifi_initialisation_semaphore();

        ///> Defines the entire logic for wifi initiation 
        static esp_err_t _init();

        ///> Automatically calls _init()  on condition of wether wifi is initialised or not.
        static esp_err_t _start();

        ///> Used to trigger wifi initiation process.
        esp_err_t _start_wifi_config_process();

 /*---------------------------------------------EVENT HANDLERS-------------------------------------------------------*/
        /// Wifi event handler 
        static void _wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

        ///> IP event handler
        static void _IP_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
 
}; 


}  // namespace WIFI

#endif  ///> WIFI_H