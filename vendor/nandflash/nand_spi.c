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

    uint64_t baudrate=spi_init(NAND_SPI, 1000*1000*1);
    INIT_PRINTF("  baud=%llu\n", baudrate);

    gpio_set_function(PIN_NAND_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NAND_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_NAND_MISO, GPIO_FUNC_SPI);

    // CS set up elsewhere atm

}


int nand_spi_write(const uint8_t* write_buff, size_t write_len, uint32_t timeout_ms) {
    // validate input
    if (!write_buff) {
        return SPI_RET_NULL_PTR;
    }

    // perform transfer
    uint32_t start_time = sys_time_get_ms();
    for (int i = 0; i < write_len; i++) {
        // block until tx empty or timeout
        while (!spi_is_writable(NAND_SPI)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // transmit data
        // LL_SPI_TransmitData8(SPI_INSTANCE, write_buff[i]);
        spi_get_hw(NAND_SPI)->dr = write_buff[i];

        /*// block until rx not empty
        while (!LL_SPI_IsActiveFlag_RXNE(SPI_INSTANCE)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // read data to clear buffer
        LL_SPI_ReceiveData8(SPI_INSTANCE);*/
        // Drain RX FIFO, then wait for shifting to finish (which may be *after*
        // TX FIFO drains), then drain RX FIFO again
        while (spi_is_readable(NAND_SPI)) {
            (void)spi_get_hw(NAND_SPI)->dr;
        }

        while (spi_get_hw(NAND_SPI)->sr & SPI_SSPSR_BSY_BITS) {
            tight_loop_contents();
        }

        while (spi_is_readable(NAND_SPI)) {
            (void)spi_get_hw(NAND_SPI)->dr;
        }

        // Don't leave overrun flag set
        spi_get_hw(NAND_SPI)->icr = SPI_SSPICR_RORIC_BITS;
    }

    return SPI_RET_OK;
}

int nand_spi_read(uint8_t* read_buff, size_t read_len, uint32_t timeout_ms) {
    // validate input
    if (!read_buff) {
        return SPI_RET_NULL_PTR;
    }

    // perform transfer
    uint32_t start_time = sys_time_get_ms();
    for (int i = 0; i < read_len; i++) {
        // block until tx empty or timeout
        while (!spi_is_writable(NAND_SPI)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // transmit data
        // LL_SPI_TransmitData8(SPI_INSTANCE, 0);
        spi_get_hw(NAND_SPI)->dr = 0;

        // block until rx not empty
        while (!spi_is_readable(NAND_SPI)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // read data from buffer
        read_buff[i] = (uint8_t)spi_get_hw(NAND_SPI)->dr;
    }

    return SPI_RET_OK;
}

int nand_spi_write_read(const uint8_t* write_buff, uint8_t* read_buff, size_t transfer_len, uint32_t timeout_ms) {
    // validate input
    if (!read_buff) {
        return SPI_RET_NULL_PTR;
    }

    // perform transfer
    uint32_t start_time = sys_time_get_ms();
    for (int i = 0; i < transfer_len; i++) {
        // block until tx empty or timeout
        while (!spi_is_writable(NAND_SPI)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // transmit data
        // LL_SPI_TransmitData8(SPI_INSTANCE, write_buff[i]);
        spi_get_hw(NAND_SPI)->dr = write_buff[i];

        // block until rx not empty
        while (!spi_is_readable(NAND_SPI)) {
            if (sys_time_is_elapsed(start_time, timeout_ms)) {
                return SPI_RET_TIMEOUT;
            }
        }
        // read data from buffer
        read_buff[i] = spi_get_hw(NAND_SPI)->dr;
    }

    return SPI_RET_OK;
}
