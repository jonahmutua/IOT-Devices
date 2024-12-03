#include "SmartConfig.h"
#include <cstring>
#include <algorithm>

namespace SMARTCONFIG
{
    /*------------------------------------STATICS INITIALISATION------------------------------------------*/
      SmartConfig::smartconfig_e SmartConfig::smartconfig_state{SmartConfig::smartconfig_e::NOT_STARTED};        ///> Smartcongif state machine
      SemaphoreHandle_t SmartConfig::smartconfig_state_mutex{nullptr};          ///> mutex to guard smartconfig_state machine
      smartconfig_start_config_t SmartConfig::cfg{};                            ///> smartconfig configuration 
      SemaphoreHandle_t SmartConfig::smartconfig_init_done{nullptr};            ///> signal when done initilising
      TaskHandle_t      SmartConfig::_smartconfig_handle{nullptr};              ///> Handle to Smartconfig task
      QueueHandle_t     SmartConfig::_queue_handle{nullptr};                    ///> handle to queue : todo protect semaphore

    esp_err_t SmartConfig::_init()
    {
        esp_err_t status{ESP_OK};
  
        ///> Create event loop only if there isnt't one 
         status = esp_event_loop_create_default();
        if( ESP_ERR_INVALID_STATE == status )
        {
            ///> if we ever get here, means we tried to create a duplicate default event loop 
            ESP_LOGI(_log_tag, "Default event loop is created already. %d, %s", __LINE__, __func__);
            status = ESP_OK;
        }
    
        ///> Register SMARTCONFIG event handler 
        if( ESP_OK == status )
        {     
             ESP_LOGI(_log_tag, "Registering smtcfg event handler %d, %s ", __LINE__ , __func__);
             status = esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &smartconfig_event_handler, NULL);
             ESP_LOGI(_log_tag, "status: %s", esp_err_to_name( status) );
        }

        ///> set smart config type .
        if( ESP_OK == status )
        {
            ESP_LOGI(_log_tag, "smartconfig_set_type %d, %s ", __LINE__ , __func__);
            status = esp_smartconfig_set_type(SC_TYPE_ESPTOUCH);    ///>ToDo Support other types        
            ESP_LOGI(_log_tag, "status: %s", esp_err_to_name( status) );
        }

        ///> start smart config service 
        if( ESP_OK == status )
        {
           
            ESP_LOGI(_log_tag, "smartconfig_starting.. %d, %s ", __LINE__ , __func__);
            status = esp_smartconfig_start(&cfg);   ///> todo : lock & unlock cfg through mutex/sem    
            ESP_LOGI(_log_tag, "status: %s", esp_err_to_name( status) );
        }
  
        return status;
    }
  
    esp_err_t SmartConfig::start()
    {
        esp_err_t status{ESP_OK};
        //> create smartconfig state mutex 
        status = create_smartconfig_state_mutex();

        ///> We only care to attempt to sart smartconfig service if we aren't started yet
        ESP_LOGI(_log_tag, "Locking smartconig state mc: %d, %s", __LINE__, __func__);
        lock_smartconfig_state();
        switch( smartconfig_state )
        {
            case  smartconfig_e::NOT_STARTED:
            {    ///> create initialisation mutex    
                create_smartconfig_initialisation_semaphore();
                
                ///> Wait for smart config task to complete init 
                ESP_LOGI(_log_tag, "Waiting to take init semaphore: %d, %s", __LINE__, __func__);

                ///> create smartconfig task and delegete smartconfig initialisation to the created task 
                // TaskHandle_t smartconfig_task_handle = nullptr;
                xTaskCreatePinnedToCore(&smartconfig_task, "smartcfg_task", SMARTCONFIG_TASK_STACK, this,  SMARTCONFIG_TASK_PRIORITY ,&_smartconfig_handle, SMARTCONFIG_TASK_CORE );
           
                 if( nullptr == _smartconfig_handle )
                {
                    ESP_LOGI(_log_tag, "Fail to create smart config task. %d, %s", __LINE__, __func__);
                    status = ESP_ERR_INVALID_STATE;
                }

                if( ESP_OK == status)
                {
                    ESP_LOGI(_log_tag, "NOT_STARTED->STARTED");
                    smartconfig_state = smartconfig_e::STARTED;
                }

                ESP_LOGI(_log_tag, "Taking to take init semaphore: %d, %s", __LINE__, __func__);
                take_smartconfig_initialisation_semaphore();

                break;

            }
            case smartconfig_e::STARTED:
                ESP_LOGI(_log_tag, "RUNNING.");
                break;

            default:
                ESP_LOGI(_log_tag , "INVALID STATE");
                break;

        }
        
         unlock_smartconfig_state();
         ESP_LOGI(_log_tag, "unLocked smartconig state mc: %d, %s", __LINE__, __func__);
        
      
        return status;
    }

     esp_err_t SmartConfig::stop()
     {
        //Todo: impliment stop smartconfig service here !
        return ESP_ERR_NOT_ALLOWED;
     }

    void SmartConfig::smartconfig_task(void *pv)
    {
        SmartConfig* instance = static_cast<SmartConfig* > (pv);
        ESP_LOGI(_log_tag, "Calling start_smartconfig_initiation_process() : %d, %s ", __LINE__, __func__);
        instance->start_smartconfig_initiation_process();  ///> todo: communicate initiation result

        ESP_LOGI(_log_tag, "Giving  init semaphore: %d, %s", __LINE__, __func__);
        give_smartconfig_initialisation_semaphore();

        while(true)
        {
            ESP_LOGI(_log_tag, "Smartfconfig task is running..");
            vTaskDelay( pdMS_TO_TICKS(2000));
        }

    }

    esp_err_t SmartConfig::start_smartconfig_initiation_process()
    {
        return _init();
    }

    esp_err_t SmartConfig::create_smartconfig_state_mutex()
    {
        esp_err_t status{ESP_OK};
        if(nullptr == smartconfig_state_mutex )
        {
           smartconfig_state_mutex =  xSemaphoreCreateMutex();
           if( nullptr == smartconfig_state_mutex)
           {
                status = ESP_ERR_INVALID_STATE;
           }
        }
        return status;
    }

    
    esp_err_t SmartConfig::lock_smartconfig_state()
    {
        esp_err_t status{ESP_OK};
        if(pdTRUE != xSemaphoreTake(smartconfig_state_mutex, portMAX_DELAY) ){
            ESP_LOGI(_log_tag, "Fail to lock _SmartConfig_state_mutex");
            status = ESP_ERR_INVALID_STATE;
        }

        return status;
    }

     ///> ullocks _SmartConfig_state
    esp_err_t SmartConfig::unlock_smartconfig_state()
    {
        esp_err_t status{ESP_OK};
        if(pdTRUE != xSemaphoreGive(smartconfig_state_mutex) ){
            ESP_LOGI(_log_tag, "Fail to unlock _SmartConfig_state_mutex");
            status = ESP_ERR_INVALID_STATE;
        }
        return status;
    }

    ///> creates Initialization semaphore used  to synchronise threads 
     esp_err_t SmartConfig::create_smartconfig_initialisation_semaphore()
    {
        esp_err_t status{ESP_OK};

        if (smartconfig_init_done == nullptr )  
        {
            smartconfig_init_done = xSemaphoreCreateBinary();
            if (smartconfig_init_done == nullptr)
             {
               ESP_LOGE(_log_tag, "Failed to create semaphore for Smartconfig initialization");
               status = ESP_ERR_NO_MEM;
            }
        }
        return status;
    }
  
    esp_err_t SmartConfig::take_smartconfig_initialisation_semaphore()
    {   esp_err_t status{ESP_OK};

         if (xSemaphoreTake(smartconfig_init_done, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE(_log_tag, "Smartconfig initialization timed out");
            status = ESP_ERR_TIMEOUT;
        }

        return status;
    }

    ///Give semaphore for initalisation
    esp_err_t SmartConfig::give_smartconfig_initialisation_semaphore()
    {   
        esp_err_t status{ESP_OK};
        if (smartconfig_init_done != nullptr)
        {
            xSemaphoreGive(smartconfig_init_done);
        }
        else
            status = ESP_ERR_INVALID_STATE;

        return status;
    }
    
    void SmartConfig::setConfig(smartconfig_start_config_t sm_cfg)
    {
        ///> todo: lock & unlock cfg through semaphore.
        cfg = sm_cfg;
    }

    esp_err_t SmartConfig::sendToQueue(smartconfig_event_got_ssid_pswd_t credentials)
    {
        esp_err_t status{ESP_OK};
        IotData_t msg;
        memcpy(msg.password, credentials.password, std::min(sizeof(msg.password) , sizeof(credentials.password) ) );
        memcpy(msg.SSID, credentials.ssid, std::min( sizeof(msg.SSID) , sizeof(credentials.ssid) ) );

        if( nullptr != _queue_handle )
        {
            status = xQueueSend(_queue_handle, &msg , pdMS_TO_TICKS(1000));
            if( pdTRUE != status)
            {
                ESP_LOGI(_log_tag, "send to queue timed out");
                status = ESP_ERR_TIMEOUT;
            }
            else
                status = ESP_OK;
        } 

        return status;
    }

    void SmartConfig::smartconfig_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
    {
        ///> Todo handle sc events here!
        
        if( SC_EVENT == event_base )
        {
            smartconfig_event_got_ssid_pswd_t* data = static_cast<smartconfig_event_got_ssid_pswd_t* >(event_data);
            switch ( event_id )
            {
                case SC_EVENT_GOT_SSID_PSWD:
                   ESP_LOGI(_log_tag, " Smartconfig got password! ");
                   sendToQueue(*data); 
                   break;

                case  SC_EVENT_SEND_ACK_DONE:
                    ESP_LOGI(_log_tag, "ACK sent to cell phone");
                default:
                     break;
            }

        }
        

    }

} // namespace SMARTCONFIG