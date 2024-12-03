#ifndef SMARTCONFIG_H
#define SMARTCONFIG_H
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/idf_additions.h"
#include "esp_event.h"
#include "esp_smartconfig.h"
#include "esp_log.h"
#include "TaskCommons.h"
#include "CommInterface.h"
#include "IotData.h"

namespace SMARTCONFIG
{
    class SmartConfig : public CommInterface
    {
        public:
            ///> Enum Smartconfig State Machine
            enum class smartconfig_e
            {
                NOT_STARTED,
                STARTED
            };

    /*-------------------------------------PUBLIC METHODS --------------------------------------------*/
            ///> constructer 
            SmartConfig(smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT(), QueueHandle_t queue_handle = nullptr)
            {
                 setConfig(cfg);  ///> sets user provided config.
                 QueueHandle_t queue = queue_handle;
                 _queue_handle = queue;
            }

            ///> Destructor
            ~SmartConfig()
            {

            }

            ///> Starts Smartconfig services
            esp_err_t start() override;

            ///> stops smartconfig services
            esp_err_t stop() override;

            ///> gets and returns smarttconfig state machine 
            smartconfig_e getSmartConfigState();



        private:
    /*---------------------------------------PRIVATE MEMBERS---------------------------------------------------------*/
            static smartconfig_e smartconfig_state;                   ///> Smartcongif state machine
            static SemaphoreHandle_t smartconfig_state_mutex;         ///> mutex to guard smartconfig_state machine
            constexpr static const char*  _log_tag ="my_smartconfig"; ///> smartconfig log tag
            static smartconfig_start_config_t cfg;                    ///> smartconfig configuration 
            static SemaphoreHandle_t smartconfig_init_done;           ///> semaphore to signal when done initialising
            static TaskHandle_t     _smartconfig_handle;              ///> handle to created smartconfig task. Todo: guard thr semaphore
            static QueueHandle_t           _queue_handle;                    ///> handle to smartconfig queue

    /*--------------------------------------STATIC PRIVATE METHODS--------------------------------------------------*/

             ///> creates Initialization semaphore used  to synchronise threads during smartconfig initiation process
            static esp_err_t create_smartconfig_initialisation_semaphore();

            ///> used to signal begining of smart initiation process. 
            static esp_err_t take_smartconfig_initialisation_semaphore();

            ///> used to signal end of smartconfig initiation.
            static esp_err_t give_smartconfig_initialisation_semaphore();

            ///> Creates mutex for protecting smartconfig state machine 
             static esp_err_t create_smartconfig_state_mutex();

            /// @brief    a mutex used to lock against smartconfig state machine
            /// @return   ESP_OK:success, ESP_ERR_INVALID_STATE
            static esp_err_t lock_smartconfig_state();
        
            ///> mutex Unlocks smartconfig state machine 
            static esp_err_t unlock_smartconfig_state();

            ///> Begins smartconnfig process 
            esp_err_t start_smartconfig_initiation_process();

            ///>smartconfg task function
            static void smartconfig_task(void *pv);

            ///> Initialises smartconfig service 
            static esp_err_t _init();

            ///> sets samrtconfig configurations
            static void setConfig(smartconfig_start_config_t cfg);

            ///> sends smartconfig data into queue
            static esp_err_t sendToQueue( smartconfig_event_got_ssid_pswd_t credentials);

    /*----------------------------------- EVENT HANDLERS -----------------------------------------------------*/
            ///> Smartconfig event handler 
            static void smartconfig_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

    };



    /*---------------------------------- SMART CONFIG ---------------------------------- */
///> Smart config for smartconfig privisioning 
// class SmartConfig : public CommunicationInterface
// {
//   public:
//     
//   private:
//   ///> static member attributes here
//   static smartconfig_e _smartconfig_state ;
//   static smartconfig_start_config_t cfg;

//   static esp_err_t _init()
//   {
//     esp_err_t status{ESP_OK};
//     switch(_smartconfig_state) 
//     {
//       case smartconfig_e::NOT_STARTED:
//       {
//         status =  esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);
        
//             if( ESP_OK == status) 
//             {
//             cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
//             status = esp_smartconfig_start(&cfg);
//             }
            
//             if( ESP_OK == status)
//             {
            
//             _smartconfig_state = smartconfig_e::STARTED;
            
//             }
//         break;
//       }
      
//       default:
//       break;
//     }

//     return status;
//   }

// };

} //namespace SMARTCONFIG_H


#endif   