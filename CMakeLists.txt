cmake_minimum_required(VERSION 3.13...3.27)

include(pico/pico-sdk/pico_sdk_init.cmake)
project(starling)
pico_sdk_init()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

#include_directories(${CMAKE_CURRENT_LIST_DIR})
#include_directories(gfx)

add_executable(synth
    src/main.c
)

target_link_libraries(synth
    pico_stdlib
)

# enable usb output, disable uart output
pico_enable_stdio_usb(synth 1)
pico_enable_stdio_uart(synth 0)

#pico_generate_pio_header(synth ${CMAKE_CURRENT_LIST_DIR}/hw/quadrature_encoder.pio)
#target_link_libraries(synth
#    pico_stdlib
#    pico_multicore
#    pico_audio_i2s
#    hardware_pio
#    hardware_i2c
#    hardware_pwm
#)
#target_compile_definitions(synth PRIVATE
#    # compile time configuration of I2S
#    PICO_AUDIO_I2S_MONO_INPUT=1
#    #define for our example code
#    USE_AUDIO_I2S=1
#)

pico_enable_stdio_usb(synth 1)

# create binaries
pico_add_extra_outputs(synth)
