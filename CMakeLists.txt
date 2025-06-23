cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD "pico_w")
include("$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake")
set(PROJECT "gdo_client")
project(${PROJECT} C CXX ASM)
pico_sdk_init()
add_executable(${PROJECT}
    ${PROJECT}.cc client.cc lock.cc)
target_link_libraries(${PROJECT}
    pico_stdlib
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_none
    )
target_include_directories(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
pico_add_extra_outputs(${PROJECT})
pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)

