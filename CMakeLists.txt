cmake_minimum_required(VERSION 3.13...3.27)

include(vendor/pico-sdk/pico_sdk_init.cmake)
include(pico_extras_import.cmake)

project(starling)
pico_sdk_init()
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_CURRENT_LIST_DIR}/src)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src/gfx)
include_directories(${CMAKE_CURRENT_LIST_DIR}/vendor)

add_subdirectory(vendor/fatfspico/src build)

add_executable(synth
    src/main.cpp

    src/hw/hw.c
    src/hw/oled.c
    src/hw/perf.c
    src/hw/disk.c
    src/hw/sdcard_config.c

    src/gfx/ngl.c
    src/gfx/gfx_ext.c
    src/gfx/widlib.c
    
    src/assets/assets.c

    src/input.c
    src/keyboard.c
    src/synth_common.cpp
    src/audio.cpp
    src/track.cpp
    src/sample.cpp
    src/instrument.cpp    
    src/userinterface.cpp

    vendor/libwave/libwave.c
    vendor/tlsf/tlsf.c
)

target_compile_options(synth PUBLIC -fno-exceptions)

pico_generate_pio_header(synth ${CMAKE_CURRENT_LIST_DIR}/src/hw/quadrature_encoder.pio)

target_link_libraries(synth
    pico_stdlib
    pico_audio_i2s
    hardware_i2c
    hardware_pwm
    hardware_pio
#    pico_multicore

    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico

    # linker flags:
    -specs=nano.specs
)

# enable usb output, disable uart output
pico_enable_stdio_usb(synth 1)
pico_enable_stdio_uart(synth 0)

target_compile_definitions(synth PRIVATE
    PICO_AUDIO_I2S_MONO_INPUT=1
    USE_AUDIO_I2S=1
)

# create binaries
pico_add_extra_outputs(synth)
