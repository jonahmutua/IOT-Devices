set(SOURCES ./Wifi.cpp )

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . ../TaskCommons ../CommInterface
                        REQUIRES  esp_wifi freertos esp_netif nvs_flash SmartConfig  )
                        