set(srcs "./src/MORO_SIM800L.cpp" )

set(include_dir "./include")


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       PRIV_REQUIRES 
                        MORO_COMMON
                        esp_timer
                    )