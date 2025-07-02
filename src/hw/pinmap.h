#pragma once
// pin & peripheral config

#define PICO_LED_PIN 15

// PSRAM SPI
#define PSRAM_PIN_CS0       0
#define PSRAM_PIN_CS1       1
#define PSRAM_PIN_SCK       2
#define PSRAM_PIN_SD0_SI    3
#define PSRAM_PIN_SD1_SO    4
#define PSRAM_PIN_SD2       5
#define PSRAM_PIN_SD3       6
#define PSRAM_PIO           pio2

// NAND flash
#define PIN_NAND_CS         9
#define PIN_NAND_SCK        10
#define PIN_NAND_MOSI       11
#define PIN_NAND_MISO       12
#define PIN_NAND_WP         13
#define PIN_NAND_HOLD       14
#define NAND_SPI            spi1
#define NAND_BAUD_RATE      (50*1000*1000)

// OLED
#define PIN_OLED_SDA        16
#define PIN_OLED_SCL        17
#define OLED_I2C            i2c0
#define OLED_CLK_KHZ        1000

// Button/LED matrix
#define LED_PWM_SLICE 1
#define PIN_LED0            18
#define PIN_LED1            19
#define PIN_LED2            20
#define PIN_LED3            21
#define PIN_BTN0            24
#define PIN_BTN1            25
#define PIN_BTN2            22
#define PIN_BTN3            23
#define PIN_COL0            26
#define PIN_COL1            27
#define PIN_COL2            28
#define PIN_COL3            29
#define PIN_COL4            30
#define PIN_COL5            31
#define PIN_COL6            32
#define PIN_COL7            33

// Encoders
#define PIN_ENC0        34  // encoder is on this pin and the next
#define PIN_ENC1        36
#define PIN_ENC2        38
#define PIN_ENC3        40
#define ENCODER_PIO     pio1

// Audio codec
#define PIN_CODEC_SDA   42
#define PIN_CODEC_SCL   43
#define PIN_CODEC_DOUT  44  // Output from codec
#define PIN_CODEC_LRCK  45
#define PIN_CODEC_BCK   46
#define PIN_CODEC_DIN   47  // Input to codec
#define CODEC_I2C       i2c1
#define PICO_AUDIO_PIO  0
#define AUDIO_DMA_CHANNEL 1