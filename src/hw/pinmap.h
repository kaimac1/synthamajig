#pragma once
// pin & peripheral config

#define PICO_LED_PIN 25

// OLED
#define OLED_I2C        i2c_default
#define OLED_CLK_KHZ    1000
#define PIN_OLED_SDA    16
#define PIN_OLED_SCL    17

// Encoders
#define PIN_ENC0 0  // encoder is on this pin and the next
#define PIN_ENC1 2
#define PIN_ENC2 4
#define PIN_ENC3 6

// I2S
#define AUDIO_DMA_CHANNEL 1
#define PIN_I2S_DATA 9
#define PIN_I2S_BCLK 10
#define PIN_I2S_LRCLK_UNUSED 11
#define PIN_DAC_MUTE_UNUSED 22

// Button/LED matrix
#define LED_PWM_SLICE 1
#define PIN_COLSEL0 26
#define PIN_COLSEL1 27
#define PIN_COLSEL2 28
#define PIN_LED0 18
#define PIN_LED1 19
#define PIN_BTN0 20
#define PIN_BTN1 21
#define PIN_BTN2 22
#define PIN_BTN3 12

// SD card
#define PIN_DISK_MOSI   15
#define PIN_DISK_MISO   8
#define PIN_DISK_SCK    14
#define PIN_DISK_CS     13