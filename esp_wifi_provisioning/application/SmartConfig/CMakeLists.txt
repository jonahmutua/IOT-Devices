set(SOURCES ./SmartConfig.cpp )

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . ../TaskCommons ../CommInterface ../IotData
                        REQUIRES  esp_wifi freertos esp_netif nvs_flash   )
                        