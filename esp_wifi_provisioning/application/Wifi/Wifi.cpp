
#include "Wifi.h"
#include <cstring>
#include <algorithm>

namespace WIFI
{

    /*------------------------------------------------Statics ---------------------------------------------------------------------*/
    Wifi::wifi_state_e Wifi::_wifi_state{Wifi::wifi_state_e::WIFI_NOT_INITIALISED};
    SemaphoreHandle_t  Wifi::_wifi_state_mutex{ nullptr};
    SemaphoreHandle_t  Wifi::_wifi_init_done{ nullptr };
    SMARTCONFIG::SmartConfig* Wifi::sc{nullptr};
    QueueHandle_t Wifi::_queue_handle{nullptr};

     /*---------------------------------------------PUBLIC METHODS-----------------------------------------------------------*/
    
    esp_err_t Wifi::start()
    {
         esp_err_t status{ESP_OK};
        ///> ToDo: Create Wifi Task here! Also appropriate to create Smartconfig task 
            
        ///> Create semaphore for Wi-Fi initialization synchronization
        status = create_wifi_initialisation_semaphore();

        ///> create _wifi_state_mutex 
        if( ESP_OK == status ) status  = create_wifi_state_mutex();
        
        ///> Create wifi task 
        TaskHandle_t   wifi_task_handle = nullptr;
        xTaskCreatePinnedToCore(&wifi_task, "wifi_task", WIFI_TASK_STACK, this,  WIFI_TASK_PRIORITY, &wifi_task_handle, WIFI_TASK_CORE );
        if( nullptr == wifi_task_handle )
        {
            ESP_LOGI(_log_tag, "Fail to create wifi task. %d, %s", __LINE__, __func__);
            status = ESP_ERR_NO_MEM;
        }
        
                
        // Wait for the semaphore signal from the Wi-Fi task 
        status = take_wifi_initialisation_semaphore();

        return status;
    }

    esp_err_t Wifi::stop() 
    {
      ///> Todo : Implement the logic for stopping Wifi servive here!
      return ESP_OK;
    }

    Wifi::wifi_state_e Wifi::getWifiState()
    {
      return Wifi::_wifi_state;
    }

   /*---------------------------------------STATIC PRIVATE METHODS---------------------------------------------------------------------*/
   void Wifi::wifi_task(void *pv)
    {
      ///> Todo: perfrom wifi tasks here 
      ///> Todo: implement Queue to send the return status
        Wifi* instance = static_cast<Wifi* >(pv);
        instance->_start_wifi_config_process();
         if( nullptr != sc )
        {
            ESP_LOGI(_log_tag, "Starting smartconfig from wifi : %d, %s", __LINE__, __func__);
            sc->start();
        }

        give_wifi_initialisation_semaphore();

        ///> Credentials will be kept here 
        IotData_t msg{};
        
        while(1)
        {
         
            ESP_LOGI(_log_tag , "Wifi task running....");
            if( nullptr != _queue_handle )
            {
                ///> if queue is empty do not block!
                xQueueReceive(_queue_handle,(void *) &msg,  0);
            }  

            ///> check if password is empty 
            if( '\0'!= msg.password[0] )
            {
                ///> This is appropriate time to connect to access point !
                ESP_LOGI(_log_tag, "SSID: %s", msg.SSID);
                ESP_LOGI(_log_tag, "pass: %s", msg.password);
               
                wifi_config_t cfg{};

                esp_wifi_get_config(WIFI_IF_STA, &cfg);
                memcpy( cfg.sta.ssid, msg.SSID , std::min(sizeof(cfg.sta.ssid ), sizeof(msg.SSID ) ) );
                memcpy( cfg.sta.password, msg.password , std::min(sizeof(cfg.sta.password ), sizeof( msg.password ) ) );
                esp_wifi_set_config(WIFI_IF_STA, &cfg );
                esp_wifi_connect();

                // lock_wifi_state();
                // switch( _wifi_state )
                // {
                //     case WIFI_READY_TO_C
                //      esp_wifi_set_config()
                // }
            }

            vTaskDelay(pdMS_TO_TICKS(500) );
        }
    }

    esp_err_t Wifi::create_wifi_state_mutex()
    {
        esp_err_t status{ESP_OK};
        if(nullptr == _wifi_state_mutex )
        {
           _wifi_state_mutex =  xSemaphoreCreateMutex();
           if( nullptr == _wifi_state_mutex)
           {
                status = ESP_ERR_INVALID_STATE;
           }
        }
        return status;
    }

    
    esp_err_t Wifi::lock_wifi_state()
    {
        esp_err_t status{ESP_OK};
        if(pdTRUE != xSemaphoreTake(_wifi_state_mutex, portMAX_DELAY) ){
            ESP_LOGI(_log_tag, "Fail to lock _wifi_state_mutex");
            status = ESP_ERR_INVALID_STATE;
        }

        return status;
    }

     ///> ullocks _wifi_state
    esp_err_t Wifi::unlock_wifi_state()
    {
        esp_err_t status{ESP_OK};
        if(pdTRUE != xSemaphoreGive(_wifi_state_mutex) ){
            ESP_LOGI(_log_tag, "Fail to unlock _wifi_state_mutex");
            status = ESP_ERR_INVALID_STATE;
        }
        return status;
    }

    ///> creates Initialization semaphore used  to synchronise threads 
     esp_err_t Wifi::create_wifi_initialisation_semaphore()
    {
        esp_err_t status{ESP_OK};

        if (_wifi_init_done == nullptr )  
        {
            _wifi_init_done = xSemaphoreCreateBinary();
            if (_wifi_init_done == nullptr)
             {
                ESP_LOGE(_log_tag, "Failed to create semaphore for Wi-Fi initialization");
               status = ESP_ERR_NO_MEM;
            }
        }
        return status;
    }
  
    
    esp_err_t Wifi::take_wifi_initialisation_semaphore()
    {   esp_err_t status{ESP_OK};

         if (xSemaphoreTake(_wifi_init_done, pdMS_TO_TICKS(1000)) != pdTRUE) {
            ESP_LOGE(_log_tag, "Wi-Fi initialization timed out");
            status = ESP_ERR_TIMEOUT;
        }

        return status;
    }

    ///Give semaphore for initalisation
    esp_err_t Wifi::give_wifi_initialisation_semaphore()
    {   
        esp_err_t status{ESP_OK};
        if (_wifi_init_done != nullptr)
        {
            xSemaphoreGive(_wifi_init_done);
        }
        else
            status = ESP_ERR_INVALID_STATE;

        return status;
    }
    
    esp_err_t Wifi::_start_wifi_config_process()
    {
       esp_err_t status{ESP_OK};
       status = _start();
       return status;
    }
   
     esp_err_t Wifi::_init()
    {   
                esp_err_t status{ESP_OK};
                ///> Intitalise net interface 
                    ESP_LOGI(_log_tag, "Initialising net interface, line:%d , : %s ", __LINE__, __func__);
                    status = esp_netif_init();
                    ESP_LOGI(_log_tag, "Net interface initalisation: %s ", esp_err_to_name (status) );
            
                
                if( ESP_OK == status ) 
                {
                    ESP_LOGI(_log_tag, "Creating default event loop:, line:%d , : %s ", __LINE__, __func__);
                    status = esp_event_loop_create_default();
                    ESP_LOGI(_log_tag, "default event loop: %s ", esp_err_to_name (status) );
                }

                if( ESP_OK == status ) 
                {
                    ESP_LOGI(_log_tag, "Creating wifi STA:, line:%d , : %s ", __LINE__, __func__);
                    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
                    if(nullptr == sta_netif) 
                    status |= ESP_FAIL;
                    else 
                    status |= ESP_OK;
                    
                    ESP_LOGI(_log_tag, "default wifi_STA: %s ", esp_err_to_name (status) );
                }

                if( ESP_OK == status) 
                {
                    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
                    ESP_LOGI(_log_tag, "Configuring wifi interface:, line:%d , : %s ", __LINE__, __func__);
                    status = esp_wifi_init(&cfg);
                    ESP_LOGI(_log_tag, "Wifi interface configuration: %s ", esp_err_to_name (status) );
                }

                ///> register system events here 
                ///> Register wifi event handler 
                if( ESP_OK == status) 
                {
                    ESP_LOGI(_log_tag, "registering wifi event hanlder:, line:%d , : %s ", __LINE__, __func__);
                    status =  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL);
                    ESP_LOGI(_log_tag, "Wifi event handler registered: %s ", esp_err_to_name (status) );
                }
                
                ///> Register IP event handler 
                    if( ESP_OK == status) 
                {
                    ESP_LOGI(_log_tag, "registering IP event hanlder:, line:%d , : %s ", __LINE__, __func__);
                    status =  esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &_IP_event_handler, NULL);
                    ESP_LOGI(_log_tag, "IP event handler registered: %s ", esp_err_to_name (status) );
                }

                ///> set wifi mode 
                if( ESP_OK == status)
                {
                    ESP_LOGI(_log_tag, "Setting wifi mode -> STA:, line:%d , : %s ", __LINE__, __func__);
                    status = esp_wifi_set_mode(WIFI_MODE_STA);
                    ESP_LOGI(_log_tag, "Wifi mode (sta) set: %s ", esp_err_to_name (status) );
                }
                
                ///> start wifi
                if( ESP_OK == status )
                {
                    ESP_LOGI(_log_tag, "Starting wifi..:%d, %s", __LINE__, __func__);
                    status = esp_wifi_start();
                    ESP_LOGI(_log_tag, "Wifi starteng result:%s", esp_err_to_name(status));
                }
                
     return status;
    }    


    esp_err_t Wifi::_start()
    {  
        ESP_LOGI(_log_tag, "Calling _start() %d, %s", __LINE__, __func__);
        esp_err_t status{ESP_OK};
        
        
        lock_wifi_state();
        ///> We care only if WIFI is  Not Initialised 
        if( _wifi_state == wifi_state_e::WIFI_NOT_INITIALISED )
        {       
              
                ESP_LOGI(_log_tag, "Calling _init() in : %d, %s", __LINE__, __func__);
                status = _init(); 
                ESP_LOGI(_log_tag, "_init() %s", esp_err_to_name(status));
                if( ESP_OK == status) 
                {             
                     _wifi_state = wifi_state_e::WIFI_INITIALISED;    
                }   
        }
        unlock_wifi_state();
        

      return status;
    }
    /*---------------------------------------------EVENT HANDLERS---------------------------------------------------------------------------*/
    ///> Event Handlers
    ///> wifi Event handler
    void Wifi::_wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
     {  
              if( WIFI_EVENT == event_base )
              {

                    ///> Lock wifi_state       
                    lock_wifi_state();
                    switch( event_id )
                    {
                        
                        case WIFI_EVENT_STA_START:
                        {
                            ESP_LOGI(_log_tag, "WIFI_EVENT_STA_START...");
                            _wifi_state = wifi_state_e::WIFI_READY_TO_CONNECT;
                            ///> TODO Start smart config here  ! 
                            ///> ToDO: smartconfig times out if started from here - find out why!
                            ///> Start is being done in Wifi start fun in the meantime .
                            // if( nullptr != sc )
                            // {
                            //     ESP_LOGI(_log_tag, "Starting smartconfig from wifi: %d, %s", __LINE__, __func__);
                            //     sc->start();
                            // }
                            ESP_LOGI(_log_tag, "WIFI_READY_TO_CONNECT: %d, %s", __LINE__, __func__);
                            esp_wifi_connect();

                            break;
                           
                        }

                        case WIFI_EVENT_STA_CONNECTED:
                            ESP_LOGI(_log_tag, "WIFI_WAITING_FOR_IP: %d, %s", __LINE__, __func__);
                            _wifi_state = wifi_state_e::WIFI_WAITING_FOR_IP;
                            break;
                        
                        case WIFI_EVENT_STA_DISCONNECTED: 
                            _wifi_state = wifi_state_e::WIFI_DISCONNECTED;
                            ESP_LOGI(_log_tag, "WIFI_DISCONNECTED!: %d, %s", __LINE__, __func__);
                            ///> ToDo: Handle Scenerios of Wifi disconnected as result of eg AP lost power ...
                            break;

                        default:
                        break;
                    }  
                     unlock_wifi_state();

                     
                } 
              
        }
         
       void Wifi::_IP_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
         { 
              //esp_event_base_t event = static_cast<>(event_base);
              if( IP_EVENT == event_base )
              {
               lock_wifi_state();
                switch( event_id )
                {
                    case IP_EVENT_STA_GOT_IP:
                        ESP_LOGI(_log_tag, "Got IP, Wifi Connection successfull! %d, %s", __LINE__, __func__);
                        _wifi_state = wifi_state_e::WIFI_CONNECTED;
                      
                        break;
                    case IP_EVENT_STA_LOST_IP:
                       ESP_LOGI(_log_tag, "Wifi lost IP .. waiting for IP! %d, %s", __LINE__, __func__);
                       _wifi_state = wifi_state_e::WIFI_WAITING_FOR_IP;
                      
                        break;
      
                    default:  ///> ToDo: Handle any other IP_EVENTS here , like IP_EVENT_STA_GOT_IPV6 ..etc
                    break;
                }  
                 unlock_wifi_state();
              }
              
          }

}  // namespace WIFI