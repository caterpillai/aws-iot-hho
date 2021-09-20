set(COMPONENT_SRCS "main.c" 
                    "tasks/ui.c" 
                    "tasks/wifi.c" 
                    "tasks/read_hho_measures.c" 
                    "tasks/aws_iot_update.c")
set(COMPONENT_ADD_INCLUDEDIRS "." "tasks/include")

register_component()

target_add_binary_data(${COMPONENT_TARGET} "certs/aws-root-ca.pem" TEXT)