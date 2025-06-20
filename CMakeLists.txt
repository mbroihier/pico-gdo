cmake_minimum_required(VERSION 3.12)
set(PICO_BOARD "pico_w")
include("$ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake")
set(PROJECT "gdo_client")
project(${PROJECT} C CXX ASM)
project(sdp_general_query C CXX ASM)
pico_sdk_init()
add_executable(sdp_general_query
    sdp_general_query.c)
add_executable(${PROJECT}
    ${PROJECT}.c client.c)
target_link_libraries(sdp_general_query
    pico_stdlib
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_none
    )
target_link_libraries(${PROJECT}
    pico_stdlib
    pico_btstack_classic
    pico_btstack_cyw43
    pico_cyw43_arch_none
    )
target_include_directories(sdp_general_query PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
target_include_directories(${PROJECT} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    )
pico_add_extra_outputs(${PROJECT})
pico_enable_stdio_usb(${PROJECT} 1)
pico_enable_stdio_uart(${PROJECT} 0)
pico_add_extra_outputs(sdp_general_query)
pico_enable_stdio_usb(sdp_general_query 1)
pico_enable_stdio_uart(sdp_general_query 0)

