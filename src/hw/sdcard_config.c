#include "hw_config.h"
#include "pinmap.h"

#define DISK_BAUD_RATE (125 * 1000 * 1000 / 4) // 31.25 MHz

static spi_t spi = {
    .hw_inst = spi1,
    .sck_gpio = PIN_DISK_SCK,
    .mosi_gpio = PIN_DISK_MOSI,
    .miso_gpio = PIN_DISK_MISO,
    .baud_rate = DISK_BAUD_RATE
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = -1 // do not drive CS pin
};

static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if
};


size_t sd_get_num() {
    return 1;
}

sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        return &sd_card;
    } else {
        return NULL;
    }
}
