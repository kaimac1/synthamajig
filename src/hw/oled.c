// SH1107-based OLED (128x128)

#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/dma.h"
#include "hw.h"

#define SH1107_I2C_ADDR            0x3C
#define SH1107_SET_MEM_MODE        0x20
#define SH1107_SET_SCROLL          0x2E
#define SH1107_SET_CONTRAST        0x81
#define SH1107_SET_CHARGE_PUMP     0x8D
#define SH1107_SET_SEG_REMAP       0xA0
#define SH1107_SET_ENTIRE_ON       0xA4
#define SH1107_SET_ALL_ON          0xA5
#define SH1107_SET_NORM_DISP       0xA6
#define SH1107_SET_INV_DISP        0xA7
#define SH1107_SET_MUX_RATIO       0xA8
#define SH1107_SET_DISP            0xAE
#define SH1107_SET_DISP_OFFSET     0xD3
#define SH1107_SET_DISP_CLK_DIV    0xD5
#define SH1107_SET_PRECHARGE       0xD9
#define SH1107_SET_COM_PIN_CFG     0xDA
#define SH1107_SET_VCOM_DESEL      0xDB

#define SH1107_HEIGHT              128
#define SH1107_WIDTH               128
#define SH1107_PAGE_HEIGHT         8
#define SH1107_NUM_PAGES           (SH1107_HEIGHT / SH1107_PAGE_HEIGHT)
#define SH1107_BUF_LEN             (SH1107_NUM_PAGES * SH1107_WIDTH)

#define DMA_PAGE_LEN               (SH1107_WIDTH+7) 
#define DMA_BUFFER_LEN             ((DMA_PAGE_LEN)*(SH1107_NUM_PAGES))

uint8_t *fbuf;                          // Framebuffer
uint16_t dma_buffer[DMA_BUFFER_LEN];    // Framebuffer data interleaved with I2C commands

volatile bool dma_active;
int dma_channel;
dma_channel_config dma_cfg;

const uint8_t default_brightness = 32;


void sh1107_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x80, cmd};
    i2c_write_blocking(OLED_I2C, SH1107_I2C_ADDR, buf, 2, false);
}


void sh1107_cmdlist(uint8_t *cmds, int num) {
    for (int i=0; i<num; i++) {
        sh1107_cmd(cmds[i]);
    }
}

void sh1107_init() {
    uint8_t cmds[] = {
        SH1107_SET_DISP,
        SH1107_SET_MEM_MODE, // horizontal
        0x10, 0xb0, 0xc8,               // set high col
        0x00, 0x10, 0x40,               // set low col
        SH1107_SET_SEG_REMAP | 0x00,
        0xC0, // normal scan direction
        SH1107_SET_NORM_DISP,
        SH1107_SET_MUX_RATIO,      0xFF,
        SH1107_SET_DISP_OFFSET,    0x00,
        SH1107_SET_DISP_CLK_DIV,   0xF0, 
        SH1107_SET_PRECHARGE,      0x22,
        SH1107_SET_COM_PIN_CFG,    0x12,
        SH1107_SET_VCOM_DESEL,     0x20,
        SH1107_SET_CHARGE_PUMP,    0x14,
        SH1107_SET_CONTRAST,       default_brightness,
        SH1107_SET_SCROLL | 0x00,
    };

    sh1107_cmdlist(cmds, count_of(cmds));
}


void start_dma_transfer(void) {
    dma_buffer[DMA_BUFFER_LEN-1] |= I2C_IC_DATA_CMD_STOP_BITS;    

    // Set address
    OLED_I2C->hw->enable = 0;
    OLED_I2C->hw->tar = SH1107_I2C_ADDR;
    OLED_I2C->hw->enable = 1;

    // Start DMA
    dma_channel_configure(
        dma_channel, 
        &dma_cfg,
        &i2c_get_hw(OLED_I2C)->data_cmd, // dest
        dma_buffer,
        DMA_BUFFER_LEN,
        true           // Start immediately.
    );
}


static void dma_handler(void) {
    // Clear irq flag
    dma_hw->ints1 = 1u << dma_channel;    
    // Wait for I2C stop condition
    while (!(OLED_I2C->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)) tight_loop_contents();
    
    dma_active = false;
}


// Start a DMA transfer of the buffer to the display.
// Returns false if a transfer is already in progress
bool oled_write(void) {

    if (dma_active) return false;
    dma_active = true;

    // Copy framebuffer into DMA buffer
    // DMA buffer elements are 16 bits wide, and each page is preceded by 7 words of command data
    // TODO optimise?
    for (int page=0; page<SH1107_NUM_PAGES; page++) {
        for (int i=0; i<SH1107_WIDTH; i++) {
            dma_buffer[page*DMA_PAGE_LEN+i+7] = fbuf[page*SH1107_WIDTH + i];
        }
    }

    start_dma_transfer();
    return true;
}


bool oled_busy(void) {
    return dma_active;
}


void oled_on(bool on) {
    while (oled_busy());
    sh1107_cmd(SH1107_SET_DISP | on);
}

void oled_set_brightness(int brightness) {
    if (brightness < 0) brightness = 0;
    if (brightness > 255) brightness = 255;
    while (oled_busy());
    sh1107_cmd(SH1107_SET_CONTRAST);
    sh1107_cmd(brightness);
}


void oled_init(uint8_t *framebuffer) {

    fbuf = framebuffer;

    i2c_init(OLED_I2C, OLED_CLK_KHZ * 1000);
    gpio_set_function(PIN_OLED_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_OLED_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_OLED_SDA);
    gpio_pull_up(PIN_OLED_SCL);

    dma_channel = dma_claim_unused_channel(true);
    dma_cfg = dma_channel_get_default_config(dma_channel);
    channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_cfg, true);
    channel_config_set_write_increment(&dma_cfg, false);
    channel_config_set_dreq(&dma_cfg, i2c_get_dreq(OLED_I2C, true));

    dma_channel_set_irq1_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_1, dma_handler);

    // Set IRQ priority
    // must be lower than the I2S DMA priority, which defautls to 0x80
    // only top 2 bits are significant
    irq_set_priority(DMA_IRQ_1, 0xf0);
    irq_set_enabled(DMA_IRQ_1, true);

    // Initialise buffer with I2C commands
    for (int page=0; page<SH1107_NUM_PAGES; page++) {
        dma_buffer[0 + page*DMA_PAGE_LEN] = 0x80 | I2C_IC_DATA_CMD_RESTART_BITS;
        dma_buffer[1 + page*DMA_PAGE_LEN] = 0x00;
        dma_buffer[2 + page*DMA_PAGE_LEN] = 0x80 | I2C_IC_DATA_CMD_RESTART_BITS;
        dma_buffer[3 + page*DMA_PAGE_LEN] = 0x10;
        dma_buffer[4 + page*DMA_PAGE_LEN] = 0x80 | I2C_IC_DATA_CMD_RESTART_BITS;
        dma_buffer[5 + page*DMA_PAGE_LEN] = 0xb0|page;
        dma_buffer[6 + page*DMA_PAGE_LEN] = 0x40 | I2C_IC_DATA_CMD_RESTART_BITS;
    }

    sh1107_init();
    oled_write();   // Write empty/initial framebuffer
    oled_on(1);     // Display on
}

