set(srcs "./src/MORO_WEBSERVER.cpp" )

set(include_dir "./include")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       PRIV_REQUIRES 
                        MORO_COMMON
                        esp_http_server 
                        esp_https_server
                        json
                    )