#include "Nvs32.h"


namespace NVS
{
    esp_err_t Nvs::nvs_init()
    {
        return nvs_flash_init();
    }

esp_err_t Nvs::nvs_save_creds(const char *key, const void *value, size_t length)
{
    nvs_handle handle {};
    esp_err_t  status{ESP_OK};
    status = nvs_open(_nvs_namespace, NVS_READWRITE, &handle);

    if(ESP_OK == status) status = nvs_set_blob(handle,key , value , length);

    if(ESP_OK == status ) status = nvs_commit(handle);

    if( handle )  nvs_close(handle);

    return status;
}

esp_err_t Nvs::nvs_load_creds(const char *key, void *out_value, size_t *length)
{
    nvs_handle handle {};
    esp_err_t  status{ESP_OK};
    status = nvs_open(_nvs_namespace, NVS_READWRITE, &handle);

    if(ESP_OK == status) status = nvs_get_blob(handle,key , out_value , length);

    if( handle )  nvs_close(handle);

    return status;
}

}  // namespace NVS