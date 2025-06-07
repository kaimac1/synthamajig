/******************************************************************************

rp2040-psram

Copyright © 2023 Ian Scott

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

******************************************************************************/
#include "psram_spi.h"

#include <stdio.h>

#if defined(PSRAM_ASYNC) && defined(PSRAM_ASYNC_SYNCHRONIZE)
void __isr psram_dma_complete_handler() {
#if PSRAM_ASYNC_DMA_IRQ == 0
    dma_hw->ints0 = 1u << async_spi_inst->async_dma_chan;
#elif PSRAM_ASYNC_DMA_IRQ == 1
    dma_hw->ints1 = 1u << async_spi_inst->async_dma_chan;
#else
#error "PSRAM_ASYNC defined without PSRAM_ASYNC_DMA_IRQ set to 0 or 1"
#endif
    /* putchar('@'); */
#if defined(PSRAM_MUTEX)
    mutex_exit(&async_spi_inst->mtx);
#elif defined(PSRAM_SPINLOCK)
    spin_unlock(async_spi_inst->spinlock, async_spi_inst->spin_irq_state);
#endif
}
#endif // defined(PSRAM_ASYNC) && defined(PSRAM_ASYNC_SYNCHRONIZE)

pio_sm_config psram_sm_cfg;

static int initialise_device(psram_spi_inst_t *psram) {
    uint8_t reset_en_cmd[] = {8, 0, 0x66};
    pio_spi_single_rw(psram, reset_en_cmd, sizeof(reset_en_cmd), 0, 0);
    busy_wait_us(50);

    uint8_t reset_cmd[] = {8, 0, 0x99};
    pio_spi_single_rw(psram, reset_cmd, sizeof(reset_cmd), 0, 0);
    busy_wait_us(100);

    // Check ID registers
    uint8_t id_cmd[] = {32, 16, 0x9F, 0, 0, 0};
    uint8_t id_data[2];
    pio_spi_single_rw(psram, id_cmd, sizeof(id_cmd), id_data, sizeof(id_data));

    const uint8_t expected_mfid = 0x0D;
    const uint8_t expected_kgd = 0x5D;
    if ((id_data[0] != expected_mfid) || (id_data[1] != expected_kgd)) {
        printf("PSRAM read ID error: MFID=%02x, KGD=%02x\n", id_data[0], id_data[1]);
        return 1;
    }
    return 0;
}

static void enable_qspi(psram_spi_inst_t *psram) {
    uint8_t qspi_en_cmd[] = {8, 0, 0x35};
    pio_spi_single_rw(psram, qspi_en_cmd, sizeof(qspi_en_cmd), 0, 0);
    busy_wait_us(10);    
}

// Change which chip select pin the PIO program drives.
// This is done by changing the SET pin base
static void psram_select_device(psram_spi_inst_t *psram, int cs_pin) {
    static int old_cs_pin = -1;

    if (cs_pin != old_cs_pin) {
        if (old_cs_pin >= 0) gpio_set_function(old_cs_pin, GPIO_FUNC_SIO);
        sm_config_set_set_pin_base(&psram_sm_cfg, cs_pin);
        pio_sm_set_config(psram->pio, psram->sm, &psram_sm_cfg);
        pio_gpio_init(psram->pio, cs_pin);
        old_cs_pin = cs_pin;
    }
}

psram_spi_inst_t psram_spi_init_clkdiv(PIO pio, int sm, float clkdiv) {
    psram_spi_inst_t spi;
    spi.pio = pio;
    spi.offset = pio_add_program(spi.pio, &spi_psram_program);
    spi.sm = sm;

#if defined(PSRAM_MUTEX)
    mutex_init(&spi.mtx);
#elif defined(PSRAM_SPINLOCK)
    int spin_id = spin_lock_claim_unused(true);
    spi.spinlock = spin_lock_init(spin_id);
#endif

    gpio_set_drive_strength(PSRAM_PIN_CS0, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_CS1, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SCK, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD0_SI, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD1_SO, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD2, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD3, GPIO_DRIVE_STRENGTH_4MA);
    /* gpio_set_slew_rate(PSRAM_PIN_CS, GPIO_SLEW_RATE_FAST); */
    /* gpio_set_slew_rate(PSRAM_PIN_SCK, GPIO_SLEW_RATE_FAST); */
    /* gpio_set_slew_rate(PSRAM_PIN_MOSI, GPIO_SLEW_RATE_FAST); */

    pio_spi_psram_cs_init(spi.pio, spi.sm, spi.offset, clkdiv, PSRAM_PIN_CS0, PSRAM_PIN_SCK, PSRAM_PIN_SD0_SI, PSRAM_PIN_SD1_SO);
    
    // Initialise & check
    psram_select_device(&spi, PSRAM_PIN_CS0);
    spi.error = initialise_device(&spi);
    if (spi.error) return spi;
    psram_select_device(&spi, PSRAM_PIN_CS1);
    spi.error = initialise_device(&spi);
    if (spi.error) return spi;    

    // Switch to QSPI mode
    psram_select_device(&spi, PSRAM_PIN_CS0);
    enable_qspi(&spi);
    psram_select_device(&spi, PSRAM_PIN_CS1);
    enable_qspi(&spi);

    // Use QSPI program
    pio_sm_unclaim(spi.pio, spi.sm);
    pio_remove_program(spi.pio, &spi_psram_program, spi.offset);    
    spi.offset = pio_add_program(spi.pio, &qspi_psram_program);
    pio_qspi_psram_cs_init(spi.pio, spi.sm, spi.offset, clkdiv, PSRAM_PIN_CS0, PSRAM_PIN_SCK, PSRAM_PIN_SD0_SI);
    
    // Write DMA channel setup
    spi.write_dma_chan = dma_claim_unused_channel(true);
    spi.write_dma_chan_config = dma_channel_get_default_config(spi.write_dma_chan);
    channel_config_set_transfer_data_size(&spi.write_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&spi.write_dma_chan_config, true);
    channel_config_set_write_increment(&spi.write_dma_chan_config, false);
    channel_config_set_dreq(&spi.write_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, true));
    dma_channel_set_write_addr(spi.write_dma_chan, &spi.pio->txf[spi.sm], false);
    dma_channel_set_config(spi.write_dma_chan, &spi.write_dma_chan_config, false);

    // Read DMA channel setup
    spi.read_dma_chan = dma_claim_unused_channel(true);
    spi.read_dma_chan_config = dma_channel_get_default_config(spi.read_dma_chan);
    channel_config_set_transfer_data_size(&spi.read_dma_chan_config, DMA_SIZE_32);
    channel_config_set_read_increment(&spi.read_dma_chan_config, false);
    channel_config_set_write_increment(&spi.read_dma_chan_config, true);
    channel_config_set_dreq(&spi.read_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, false));
    dma_channel_set_read_addr(spi.read_dma_chan, &spi.pio->rxf[spi.sm], false);
    dma_channel_set_config(spi.read_dma_chan, &spi.read_dma_chan_config, false);

#if defined(PSRAM_ASYNC)
    // Asynchronous DMA channel setup
    spi.async_dma_chan = dma_claim_unused_channel(true);
    spi.async_dma_chan_config = dma_channel_get_default_config(spi.async_dma_chan);
    channel_config_set_transfer_data_size(&spi.async_dma_chan_config, DMA_SIZE_8);
    channel_config_set_read_increment(&spi.async_dma_chan_config, true);
    channel_config_set_write_increment(&spi.async_dma_chan_config, false);
    channel_config_set_dreq(&spi.async_dma_chan_config, pio_get_dreq(spi.pio, spi.sm, true));
    dma_channel_set_write_addr(spi.async_dma_chan, &spi.pio->txf[spi.sm], false);
    dma_channel_set_config(spi.async_dma_chan, &spi.async_dma_chan_config, false);

#if defined(PSRAM_ASYNC_COMPLETE)
    irq_set_exclusive_handler(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, psram_dma_complete_handler);
    dma_irqn_set_channel_enabled(PSRAM_ASYNC_DMA_IRQ, spi.async_dma_chan, true);
    irq_set_enabled(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, true);
#endif // defined(PSRAM_ASYNC_COMPLETE)
#endif // defined(PSRAM_ASYNC)

    spi.error = 0;
    return spi;
};

psram_spi_inst_t psram_spi_init(PIO pio, int sm) {
    return psram_spi_init_clkdiv(pio, sm, 1.0);
}

void psram_spi_uninit(psram_spi_inst_t spi) {
#if defined(PSRAM_ASYNC)
    // Asynchronous DMA channel teardown
    dma_channel_unclaim(spi.async_dma_chan);
#if defined(PSRAM_ASYNC_COMPLETE)
    irq_set_enabled(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, false);
    dma_irqn_set_channel_enabled(PSRAM_ASYNC_DMA_IRQ, spi.async_dma_chan, false);
    irq_remove_handler(DMA_IRQ_0 + PSRAM_ASYNC_DMA_IRQ, psram_dma_complete_handler);
#endif // defined(PSRAM_ASYNC_COMPLETE)
#endif // defined(PSRAM_ASYNC)

    // Write DMA channel teardown
    dma_channel_unclaim(spi.write_dma_chan);

    // Read DMA channel teardown
    dma_channel_unclaim(spi.read_dma_chan);

#if defined(PSRAM_SPINLOCK)
    int spin_id = spin_lock_get_num(spi.spinlock);
    spin_lock_unclaim(spin_id);
#endif

    pio_sm_unclaim(spi.pio, spi.sm);
    pio_remove_program(spi.pio, &qspi_psram_program, spi.offset);
}

