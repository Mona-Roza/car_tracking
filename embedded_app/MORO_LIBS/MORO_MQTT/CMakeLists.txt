set(srcs "./src/MORO_MQTT.c" )

set(include_dir "./include")


idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dir}"
                       REQUIRES 
                        MORO_COMMON
                        mqtt
                    )