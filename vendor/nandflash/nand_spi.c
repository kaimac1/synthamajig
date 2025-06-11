/**
 * @file        spi.c
 * @author      Andrew Loebs
 * @brief       Implementation file of the spi module
 *
 */
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "nand_spi.h"
#include "sys_time.h"
#include "hw/pinmap.h"
#include "common.h"

// public function definitions
void nand_spi_init(void) {
    INIT_PRINTF("Initialising disk:\n");

    // WP and HOLD pins held high for 4-wire SPI
    gpio_init(PIN_NAND_WP);
    gpio_set_dir(PIN_NAND_WP, GPIO_OUT);
    gpio_put(PIN_NAND_WP, 1);
    gpio_init(PIN_NAND_HOLD);
    gpio_set_dir(PIN_NAND_HOLD, GPIO_OUT);
    gpio_put(PIN_NAND_HOLD, 1);    

    uint64_t baudrate=spi_init(NAND_SPI, 25*1000*1000);
    INIT_PRINTF("  baud=%llu\n", baudrate);

    gpio_set_function(PIN_NAND_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NAND_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NAND_MISO, GPIO_FUNC_SPI);

    // CS set up elsewhere atm

}

int nand_spi_write(const uint8_t* write_buff, size_t write_len, uint32_t timeout_ms) {
    spi_write_blocking(NAND_SPI, write_buff, write_len);
    return SPI_RET_OK;
}

int nand_spi_read(uint8_t* read_buff, size_t read_len, uint32_t timeout_ms) {
    spi_read_blocking(NAND_SPI, 0, read_buff, read_len);
    return SPI_RET_OK;
}

int nand_spi_write_read(const uint8_t* write_buff, uint8_t* read_buff, size_t transfer_len, uint32_t timeout_ms) {
    spi_write_read_blocking(NAND_SPI, write_buff, read_buff, transfer_len);
    return SPI_RET_OK;
}
