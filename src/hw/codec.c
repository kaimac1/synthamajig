#include "codec.h"
#include "pinmap.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "../common.h"

#define CODEC_I2C_CLK_KHZ 100
#define CODEC_I2C_ADDR 0x18

// Clock config
// Gives fs=48 khz from BCLK=1.536 MHz
#define NDAC    2
#define MDAC    7
#define DOSR    128
#define PLL_R   4
#define PLL_J   14
#define PLL_P   1

static int set_reg(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    int cnt = i2c_write_blocking(CODEC_I2C, CODEC_I2C_ADDR, data, sizeof(data), false);
    if (cnt == sizeof(data)) {
        return 0;
    }
    return cnt; // error value
}

void codec_init(void) {

    INIT_PRINTF("codec\n");

    // I2C
    i2c_init(CODEC_I2C, CODEC_I2C_CLK_KHZ * 1000);
    gpio_set_function(PIN_CODEC_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_CODEC_SCL, GPIO_FUNC_I2C);

    int r = set_reg(0x00, 0x00);    // set page 0
    if (r) {
        INIT_PRINTF("  error\n");
        return;
    }
    set_reg(0x01, 0x01); // reset
    set_reg(0x0b, 0x80 | NDAC);
    set_reg(0x0c, 0x80 | MDAC);
    set_reg(0x0d, 0x00); // dosr = 128
    set_reg(0x0e, 0x80); // dosr = 128
    set_reg(0x1b, 0x00); // I2S, 16 bit
    // set_reg(0x3c, 0x01); // PRB_P1 (default)
    set_reg(0x04, 0x07); // BCLK -> PLL, PLL -> CODEC_CLKIN
    set_reg(0x06, 14);  // PLL_J = 14
    set_reg(0x07, 0);   // PLL_D = 0
    set_reg(0x08, 0);   // PLL_D = 0
    set_reg(0x05, 0b10010100); // PLL_P=1, PLL_R=4, PLL on

    // PLL debug output:
    // set_reg(0x1a, 0x81); // clkout divider on, div 1
    // set_reg(0x19, 0x03); // cdiv_clkin = pll
    // set_reg(0x34, 0x10); // clkout on gpio

    set_reg(0x00, 0x01); // page 1
    set_reg(0x01, 0x08); // disable internal AVdd
    set_reg(0x02, 0x01); // enable analogue blocks, enable AVdd LDO
    set_reg(0x7b, 0x01); // REF charging time = 40 ms
    set_reg(0x14, 0x25); // soft stepping
    set_reg(0x0a, 0x33); // common mode 0.9 V, HP common mode 1.65 V
    set_reg(0x0c, 0x08); // left DAC to HPL
    set_reg(0x0d, 0x08); // right DAC to HPR
    set_reg(0x03, 0x00); // DAC PTM = PTM_P3/4
    set_reg(0x04, 0x00); //
    set_reg(0x10, 0x00); // HPL gain = 0 dB
    set_reg(0x11, 0x00); // HPR gain = 0 dB
    set_reg(0x09, 0x30); // power up HPL/HPR drivers
    // can wait here if soft stepping

    set_reg(0x00, 0x00); // page 0
    set_reg(0x3f, 0xd6); // power up DAC channels
    set_reg(0x40, 0x00); // unmute

}

