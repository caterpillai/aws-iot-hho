set(COMPONENT_SRCDIRS .)
set(COMPONENT_ADD_INCLUDEDIRS .)

if(CONFIG_SOFTWARE_MIC_SUPPORT)
    list(APPEND COMPONENT_SRCDIRS sound-sensor)
    list(APPEND COMPONENT_ADD_INCLUDEDIRS sound-sensor/include)
endif()

set(COMPONENT_REQUIRES "mbedtls" "freertos" "core2forAWS")
register_component()