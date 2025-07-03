add_library(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE)

target_compile_definitions(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
    PICO_MAX_SHARED_IRQ_HANDLERS=8u
)
# target_compile_options(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE -ffile-prefix-map=${CMAKE_CURRENT_LIST_DIR}=)

target_sources(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ff.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffsystem.c
    ${CMAKE_CURRENT_LIST_DIR}/ff15/source/ffunicode.c
    ${CMAKE_CURRENT_LIST_DIR}/src/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/f_util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/ff_stdio.c
    ${CMAKE_CURRENT_LIST_DIR}/src/glue.c
)
target_include_directories(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
    ff15/source
    sd_driver
    include
)

target_link_libraries(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico INTERFACE
    hardware_dma
    hardware_spi
    hardware_sync
    pico_stdlib
)
