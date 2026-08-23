idf_component_register(
    SRCS 
        "main.cpp"
        "controller.cpp"
        "touch_sensor.cpp"
        "buzzer.cpp"
    INCLUDE_DIRS 
        "../include"
        "../include/external"
    REQUIRES 
        driver
        esp_eth
)

# Enforce C++20 standard for ESP-IDF
target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)

# Optional: Disable extensions if required
target_compile_options(${COMPONENT_LIB} PRIVATE -fno-ext-numeric-literals)