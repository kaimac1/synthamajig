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
#include "common.h"
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

psram_spi_inst_t psram;
pio_sm_config psram_sm_cfg;
const pio_program_t *current_program;
unsigned int current_program_offset;

static void use_program(const pio_program_t *program) {
    if (program == current_program) return;
    if (current_program) {
        pio_remove_program(PSRAM_PIO, current_program, current_program_offset);
        pio_sm_unclaim(PSRAM_PIO, PSRAM_SM0);
        pio_sm_unclaim(PSRAM_PIO, PSRAM_SM1);
        pio_clear_instruction_memory(PSRAM_PIO);
    }
    current_program = program;
    current_program_offset = pio_add_program(PSRAM_PIO, program);

    if (current_program == &spi_psram_program) {
        pio_spi_psram_cs_init(PSRAM_PIO, PSRAM_SM0, current_program_offset, PSRAM_CLKDIV, PSRAM_PIN_CS0, PSRAM_PIN_SCK, PSRAM_PIN_SD0_SI, PSRAM_PIN_SD1_SO);
    } else if (current_program == &qspi_psram_program) {
        pio_qspi_psram_cs_init(PSRAM_PIO, PSRAM_SM0, current_program_offset, PSRAM_CLKDIV, PSRAM_PIN_CS0, PSRAM_PIN_SCK, PSRAM_PIN_SD0_SI);
        pio_qspi_psram_cs_init(PSRAM_PIO, PSRAM_SM1, current_program_offset, PSRAM_CLKDIV, PSRAM_PIN_CS1, PSRAM_PIN_SCK, PSRAM_PIN_SD0_SI);
    }

}

// Change which chip select pin the PIO program drives.
// This is done by changing the SET pin base
static void select_device(int cs_pin) {
    int other_cs_pin = (cs_pin == PSRAM_PIN_CS0) ? PSRAM_PIN_CS1 : PSRAM_PIN_CS0;

    gpio_set_function(other_cs_pin, GPIO_FUNC_SIO);
    gpio_put(other_cs_pin, 1);
    sm_config_set_set_pin_base(&psram_sm_cfg, cs_pin);
    pio_sm_set_config(PSRAM_PIO, PSRAM_SM0, &psram_sm_cfg);
    pio_gpio_init(PSRAM_PIO, cs_pin);
    busy_wait_us(50);
}

static int initialise_device(int device) {
    use_program(&spi_psram_program);
    select_device(device ? PSRAM_PIN_CS1 : PSRAM_PIN_CS0);

    // Reset enable
    const uint8_t reset_en_cmd[] = {8, 0, 0x66};
    pio_spi_single_rw(reset_en_cmd, sizeof(reset_en_cmd), 0, 0);
    busy_wait_us(50);

    // Reset
    const uint8_t reset_cmd[] = {8, 0, 0x99};
    pio_spi_single_rw(reset_cmd, sizeof(reset_cmd), 0, 0);
    busy_wait_us(100);

    // Check ID registers
    const uint8_t id_cmd[] = {32, 16, 0x9F, 0, 0, 0};
    uint8_t id_data[2];
    pio_spi_single_rw(id_cmd, sizeof(id_cmd), id_data, sizeof(id_data));

    const uint8_t expected_mfid = 0x0D;
    const uint8_t expected_kgd = 0x5D;
    if ((id_data[0] != expected_mfid) || (id_data[1] != expected_kgd)) {
        INIT_PRINTF("  read ID error: MFID=%02x, KGD=%02x\n", id_data[0], id_data[1]);
        return 1;
    }
    return 0;
}

static void qspi_enter(int device) {
    use_program(&spi_psram_program);
    select_device(device ? PSRAM_PIN_CS1 : PSRAM_PIN_CS0);

    uint8_t qspi_en_cmd[] = {8, 0, 0x35};
    pio_spi_single_rw(qspi_en_cmd, sizeof(qspi_en_cmd), 0, 0);
    busy_wait_us(10);
}

static void qspi_exit(int device) {
    use_program(&qspi_psram_program);

    uint32_t setup = 0x00010000;
    uint32_t cmd = 0xF5000000;
    int sm = device ? PSRAM_SM1 : PSRAM_SM0;

    pio_sm_put(PSRAM_PIO, sm, setup);
    pio_sm_put(PSRAM_PIO, sm, cmd);
    while(!pio_sm_is_tx_fifo_empty(PSRAM_PIO, sm));
    busy_wait_us(100);
}

int psram_spi_init(void) {

#if defined(PSRAM_MUTEX)
    mutex_init(&psram.mtx);
#elif defined(PSRAM_SPINLOCK)
    int spin_id = spin_lock_claim_unused(true);
    psram.spinlock = spin_lock_init(spin_id);
#endif

    mutex_init(&psram.mtx);

    gpio_init(PSRAM_PIN_CS0);
    gpio_set_dir(PSRAM_PIN_CS0, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS0, 1);
    gpio_init(PSRAM_PIN_CS1);
    gpio_set_dir(PSRAM_PIN_CS1, GPIO_OUT);
    gpio_put(PSRAM_PIN_CS1, 1);

    gpio_set_drive_strength(PSRAM_PIN_CS0, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_CS1, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SCK, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD0_SI, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD1_SO, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD2, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_drive_strength(PSRAM_PIN_SD3, GPIO_DRIVE_STRENGTH_4MA);

    INIT_PRINTF("psram0\n");
    qspi_exit(0);
    int error = initialise_device(0);
    if (error) return 1;
    qspi_enter(0);

    INIT_PRINTF("psram1\n");
    qspi_exit(1);
    error = initialise_device(1);
    if (error) return 2;
    qspi_enter(1);

    use_program(&qspi_psram_program);
    return 0;
};

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

    pio_sm_unclaim(PSRAM_PIO, PSRAM_SM0);
    pio_sm_unclaim(PSRAM_PIO, PSRAM_SM1);
    pio_remove_program(PSRAM_PIO, &qspi_psram_program, current_program_offset);
}

void psram_test(psram_spi_inst_t *psram) {
    uint32_t psram_begin, psram_elapsed;
    float psram_speed;

    puts("Testing PSRAM...");

    // **************** 32 bits testing ****************
    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (16 * 1024 * 1024); addr += 4) {
        psram_write32(addr, addr + 10);
    }
    psram_elapsed = time_us_32() - psram_begin;
    psram_speed = 1000000.0 * 16 / psram_elapsed;
    printf("32 bit: PSRAM write 16MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);

    psram_begin = time_us_32();
    for (uint32_t addr = 0; addr < (16 * 1024 * 1024); addr += 4) {
        uint32_t result = psram_read32(addr);
        if (result != addr+10) {
            printf("PSRAM failure at address %x (%08x != %08x) ", addr, result, addr+10);
            return;
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 16 / psram_elapsed;
    printf("32 bit: PSRAM read 16MB in %d us, %.2f MB/s\n", psram_elapsed, psram_speed);    


    psram_begin = time_us_32();
    const int bufsiz = 4;
    const int nbytes = 4*bufsiz;
    for (uint32_t addr = 0; addr < 16*1024*1024; addr += nbytes) {
        uint32_t buffer[bufsiz];
        psram_readwords(addr, buffer, bufsiz);
        if (buffer[2] != addr+18) {
            printf("PSRAM failure at address %x (%08x %08x %08x %08x) ", addr, buffer[0], buffer[1], buffer[2], buffer[3]);
            return;
        }
    }
    psram_elapsed = (time_us_32() - psram_begin);
    psram_speed = 1000000.0 * 16 / psram_elapsed;
    printf("%d byte buffer: PSRAM read 16MB in %d us, %.2f MB/s\n", nbytes, psram_elapsed, psram_speed);    

}