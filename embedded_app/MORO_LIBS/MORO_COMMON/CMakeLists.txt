set(srcs "./src/MORO_COMMON.c" )

set(include_dir "./include")


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       REQUIRES 
                        json

                        esp_event
                        spiffs
                        efuse
                    )