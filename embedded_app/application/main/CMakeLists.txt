set(srcs "main.c")

set(include_dirs ".")

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS ${include_dirs}
                       
                       REQUIRES 
                       MORO_SIM800L
                       MORO_MQTT
                       MORO_COMMON
               
                       esp_timer
                       driver

                        EMBED_TXTFILES
                        "../embedded_certs/ca.pem"
                        "../embedded_certs/cert.crt"
                        "../embedded_certs/private.key"

                    )
