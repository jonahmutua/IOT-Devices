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
                    ESP_LOGI(_log_tag, "STARTING");
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

       ///> signal tha calling task we are done with smartconfig initialisation process
        give_smartconfig_initialisation_semaphore();

        size_t size_required ;
        nvs_load_creds("creds", nullptr , &size_required);
        IotData_t* msg = new IotData_t[size_required] ;

        //nvs_delete_creds();

        ///> when we boot , load credentials from NVS  
         if(nullptr != msg && ESP_OK ==  nvs_load_creds("creds", msg , &size_required) )
         {
                
                char buff[32]{};
                // smartconfig_event_got_ssid_pswd_t data;
                // memcpy(data.ssid , msg.SSID, std::min( sizeof(data.ssid), sizeof(msg.SSID)));
                // memcpy(data.password , msg.password, std::min( sizeof(data.password), sizeof(msg.password)));
                to_string(smartconfig_message_type_e::SMARTCONFIG_CREDS_FROM_NVS, buff, sizeof(buff));
                memcpy(msg->msg_id, buff , sizeof(msg->msg_id));
                /// we got creds from nvs , sent the to queue
                sendToQueue(*msg);
          }
          if( nullptr != msg) delete[] msg;


        while(true)
        {
            ESP_LOGI(_log_tag, "Smartfconfig task is running..");
          
           ///> ToDo: keep smartconfig task work here 
           
            vTaskDelay( pdMS_TO_TICKS(4000));
        }

    }

    esp_err_t SmartConfig::start_smartconfig_initiation_process()
    {
        esp_err_t status{ESP_OK};
        
        ///> initialize smartconfig
        status = _init();

        return status;
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

    esp_err_t SmartConfig::sendToQueue(IotData_t data)
    {
        esp_err_t status{ESP_OK};
        // IotData_t msg;
        // memcpy(msg.password, credentials.password, std::min(sizeof(msg.password) , sizeof(credentials.password) ) );
        // memcpy(msg.SSID, credentials.ssid, std::min( sizeof(msg.SSID) , sizeof(credentials.ssid) ) );

        if( nullptr != _queue_handle )
        {
            status = xQueueSend(_queue_handle, &data , pdMS_TO_TICKS(1000));
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
            IotData_t msg{};
            char buff[32]{};
            smartconfig_event_got_ssid_pswd_t* data = static_cast<smartconfig_event_got_ssid_pswd_t* >(event_data);
            switch ( event_id )
            {
                case SC_EVENT_GOT_SSID_PSWD:
                {
                   ESP_LOGI(_log_tag, " Smartconfig got password! ");
                   memcpy( msg.SSID, data->ssid , std::min(sizeof(msg.SSID), sizeof(data->ssid)) );
                   memcpy( msg.password, data->password, std::min(sizeof(msg.password), sizeof(data->password)) );
                    ///> save the creds to nvs
                   nvs_save_creds("creds", &msg, sizeof(msg));

                   to_string(smartconfig_message_type_e::SMARTCONFIG_CREDS_FROM_ESPTOUCH, buff, sizeof(buff));
                   memcpy( msg.msg_id, buff , std::min(sizeof(msg.msg_id), sizeof(buff)) );
                   sendToQueue(msg);
                }
                   break;

                case  SC_EVENT_SEND_ACK_DONE:
                    ESP_LOGI(_log_tag, "ACK sent to cell phone");
                default:
                     break;
            }

        }
        

    }
    // Function to map enum values to string representations
   void SmartConfig::to_string(smartconfig_message_type_e messageType, char *out_buff, size_t len)
     {
        switch (messageType) {
            case smartconfig_message_type_e::SMARTCONFIG_RUNNING:
                memcpy(out_buff, "SMARTCONFIG_RUNNING", len );
                break;
               
            case smartconfig_message_type_e::SMARTCONFIG_STOPPED:
                memcpy(out_buff, "SMARTCONFIG_STOPPED", len );
                break;
                
            case smartconfig_message_type_e::SMARTCONFIG_CREDS_FROM_NVS:
                memcpy(out_buff, "SMARTCONFIG_CREDS_FROM_NVS", len );
                break;
            case smartconfig_message_type_e::SMARTCONFIG_CREDS_FROM_ESPTOUCH:
                memcpy(out_buff, "SMARTCONFIG_CREDS_FROM_ESPTOUCH", len );
                break;

            default:
                memcpy(out_buff, "UNKNOWN", len );
                break;
              
        }
    }

    esp_err_t SmartConfig::nvs_save_creds(const char *key, const void *value, size_t length)
    {
        nvs_handle handle {};
        esp_err_t  status{ESP_OK};
        status = nvs_open(_nvs_namespace, NVS_READWRITE, &handle);

        ESP_LOGI(_log_tag,"Saving credentials to nvs");
        if(ESP_OK == status) status = nvs_set_blob(handle,key , value , length);

        if(ESP_OK == status ) status = nvs_commit(handle);

        if( handle )  nvs_close(handle);

        return status;
    }

    esp_err_t SmartConfig::nvs_load_creds(const char *key, void *out_value, size_t *length)
    {
        nvs_handle handle {};
        esp_err_t  status{ESP_OK};
        status = nvs_open(_nvs_namespace, NVS_READWRITE, &handle);

        ESP_LOGI(_log_tag,"Retreiving credentials from nvs");
        if(ESP_OK == status) status = nvs_get_blob(handle,key , out_value , length);

        if( handle )  nvs_close(handle);
         ESP_LOGI(_log_tag,"%s", esp_err_to_name(status));
        return status;
    }

    esp_err_t SmartConfig::nvs_delete_creds()
    {
        nvs_handle handle {};
        esp_err_t  status{ESP_OK};
        status = nvs_open(_nvs_namespace, NVS_READWRITE, &handle);

        ESP_LOGI(_log_tag,"Deleting credentials from nvs");
        if(ESP_OK == status) status = nvs_erase_all(handle);

        if( handle )  nvs_close(handle);

        return status;
    }
} // namespace SMARTCONFIG