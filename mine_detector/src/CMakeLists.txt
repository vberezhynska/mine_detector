idf_component_register(
    SRCS 
        "main.cpp"
        "controller.cpp"
        "touch_sensor.cpp"
        "gps_neo.cpp"
        "gps_decoder.cpp"
        "buzzer.cpp"
    INCLUDE_DIRS 
        "../include"
        "../include/interfaces"
    REQUIRES 
        driver
        esp_eth
        freertos
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++20)