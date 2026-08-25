cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico2_w)
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(lora_driver C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_library(lora_driver STATIC lora.c)
target_include_directories(lora_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lora_driver pico_stdlib hardware_spi hardware_gpio)
