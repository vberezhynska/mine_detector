idf_component_register(
    SRCS 
        "main.cpp"
        "controller.cpp"
        "touch_sensor.cpp"
        "gps_neo.cpp"
        "gps_decoder.cpp"
        "buzzer.cpp"
        "touch_sensor.cpp"
        "udp_socket.cpp"
        "http_client.cpp"
        "wifi_manager.cpp"
    INCLUDE_DIRS 
        "../include"
        "../include/interfaces"
    REQUIRES 
        driver
        esp_eth
        freertos
        nvs_flash
        esp_wifi
        esp_http_client
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++20)