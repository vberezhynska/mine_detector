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
        "../include/external"
    REQUIRES       
        esp_eth
        esp_wifi
        esp_event
        esp_http_client
        esp_timer
        driver
        freertos
        nvs_flash
        lwip
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=c++20)