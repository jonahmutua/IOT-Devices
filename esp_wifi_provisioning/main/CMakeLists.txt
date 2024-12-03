
#Define all source files here
set(SOURCES ./main.cpp)

#Register idf components 
idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . 
                    REQUIRES freertos esp_common  Wifi nvs_flash )