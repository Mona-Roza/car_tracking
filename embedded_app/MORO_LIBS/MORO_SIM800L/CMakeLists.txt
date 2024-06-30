set(srcs "./src/MORO_SIM800L.c" )

set(include_dir "./include")


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       REQUIRES 
                        espressif__esp_modem
                             
                        MORO_COMMON
                    )