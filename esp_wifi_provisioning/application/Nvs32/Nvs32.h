#ifndef NVS32_H
#define NVS32_H

#include <cstring>
#include "esp_flash.h"
#include "nvs_flash.h"

namespace NVS
{
    class Nvs
    {
        public:
                Nvs()
                {

                }
                ~Nvs()
                {

                }

                /// @brief             initialises nvs 
                /// @return            ESP_OK - success otherwise ESP_FAIL.
                esp_err_t  nvs_init();

                /// @brief               saves credentials to nvs
                /// @param handle        handle to nvs 
                /// @param key           ssid
                /// @param value         password
                /// @param length        length of password
                /// @return              ESP_OK for success  otherwise ESP_FAIL
                esp_err_t nvs_save_creds(const char *key, const void *value, size_t length);

                /// @brief                loads credentials from nvs 
                /// @param handle         handle to nvs
                /// @param key            ssid
                /// @param out_value      password
                /// @param length         lenght of password
                /// @return               ESP_OK - ssid and password found othwise ESP_FAIL
                esp_err_t nvs_load_creds(const char *key, void *out_value, size_t *length);

        private:
                const char* _nvs_namespace {"nvs"};

    };

} //namespace NVS


#endif