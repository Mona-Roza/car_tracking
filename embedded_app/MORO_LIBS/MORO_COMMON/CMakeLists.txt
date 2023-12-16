set(srcs "./src/MORO_COMMON.cpp" )

set(include_dir "./include")

#if we are going to use any library that esp provides us, we add it to the REQUIRES section 
#by telling the location of the header file in the esp/components folder.

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       REQUIRES 
                        esp_netif
                        nvs_flash
                        app_update 
                        esp_http_client 
                        esp-tls 
                        esp_https_ota 
                        vfs 
                        fatfs 
                        spiffs 
                        json
                        efuse
                    )
