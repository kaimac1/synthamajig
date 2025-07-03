#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/sync.h"
#include "hardware/structs/xip.h"
#include "hardware/regs/xip.h"
#include "quadrature_encoder.pio.h"
#include "hw.h"
#include "codec.h"
#include "oled.h"
#include "disk.h"
#include "pinmap.h"
#include "audio.h"
#include "psram_spi.h"
#include "pico/audio_i2s.h"
#include "gfx/ngl.h"
#include "../common.h"

const int PWM_SYSCLK_DIVISOR = 32;
const int LED_SCAN_RATE_HZ = 100;

#define NUM_COLUMNS 8
#define LED_TIMER_UPDATE_HZ (LED_SCAN_RATE_HZ * NUM_COLUMNS)

static bool led_timer_callback(repeating_timer_t *rt);
static void led_timer_start(void);
static void matrix_init(void);
static void psram_init(void);

static repeating_timer_t led_timer;
static int led_column;
static uint8_t led_value[NUM_LEDS];
static uint8_t btn_value[NUM_BUTTONS];

static struct audio_buffer_pool *audio_pool;
static struct audio_buffer *current_audio_buffer;

void hw_init(void) {

    stdio_init_all();
    INIT_PRINTF("\n\n~ Synthamajig ~\n");
    INIT_PRINTF("clk_sys=%.0f MHz\n", clock_get_hz(clk_sys) / 1000000.0f);

    // Debug LED
    //gpio_init(PICO_LED_PIN);
    //gpio_set_dir(PICO_LED_PIN, GPIO_OUT);

    // OLED & graphics library
    oled_init(ngl_framebuffer());
    ngl_init();

    // RAM
    psram_init();

    // Encoders
    pio_set_gpio_base(ENCODER_PIO, 16);
    pio_add_program(ENCODER_PIO, &quadrature_encoder_program);
    quadrature_encoder_program_init(ENCODER_PIO, 0, PIN_ENC0, 0);
    quadrature_encoder_program_init(ENCODER_PIO, 1, PIN_ENC1, 0);
    quadrature_encoder_program_init(ENCODER_PIO, 2, PIN_ENC2, 0);
    quadrature_encoder_program_init(ENCODER_PIO, 3, PIN_ENC3, 0);

    // Input & LED matrix
    matrix_init();

    // Codec
    codec_init();
    audio_pool = init_audio(SAMPLE_RATE, PIN_CODEC_DIN, PIN_CODEC_LRCK, 0, AUDIO_DMA_CHANNEL);

    // Disk
    disk_init();
}


void hw_audio_start(void) {
    audio_i2s_set_enabled(true);
}


int32_t read_knob(int encoder) {
    if ((encoder < 0) || (encoder >= 4)) return 0;
    return -quadrature_encoder_get_count(ENCODER_PIO, encoder);
}


void hw_set_led(int led, uint8_t value) {
    int scaled = (value*value) >> 8;
    led_value[led] = scaled;
}


void hw_debug_led(bool value) {
    gpio_put(PICO_LED_PIN, value);
}


bool hw_read_button(int button) {
    return btn_value[button];
}


static void psram_init(void) {
    int error = psram_spi_init();
    if (error) {
        INIT_PRINTF("  error %d\n", error);
        while (1);
    } else {
        INIT_PRINTF("  ok\n");
    }
}


static void set_sw_row(int row) {
    gpio_put(PIN_BTN0, 0);
    gpio_put(PIN_BTN1, 0);
    gpio_put(PIN_BTN2, 0);
    gpio_put(PIN_BTN3, 0);
    if (row == 0) gpio_put(PIN_BTN0, 1);
    if (row == 1) gpio_put(PIN_BTN1, 1);
    if (row == 2) gpio_put(PIN_BTN2, 1);
    if (row == 3) gpio_put(PIN_BTN3, 1);    

    // Work around E9 issue
    for (int i=0; i<NUM_COLUMNS; i++) gpio_set_input_enabled(PIN_COL0 + i, false);    
    for (int i=0; i<NUM_COLUMNS; i++) gpio_pull_down(PIN_COL0 + i);
    for (int i=0; i<NUM_COLUMNS; i++) gpio_set_input_enabled(PIN_COL0 + i, true);
}


static void matrix_init(void) {
    // Columns
    for (int i=0; i<NUM_COLUMNS; i++) {
        gpio_init(PIN_COL0 + i);
        gpio_set_dir(PIN_COL0 + i, GPIO_IN);
        gpio_pull_down(PIN_COL0 + i);
    }

    // Button row drive
    gpio_init(PIN_BTN0);  gpio_set_dir(PIN_BTN0, GPIO_OUT);
    gpio_init(PIN_BTN1);  gpio_set_dir(PIN_BTN1, GPIO_OUT);
    gpio_init(PIN_BTN2);  gpio_set_dir(PIN_BTN2, GPIO_OUT);
    gpio_init(PIN_BTN3);  gpio_set_dir(PIN_BTN3, GPIO_OUT);

    // pwm_config config = pwm_get_default_config();
    // pwm_config_set_clkdiv(&config, PWM_SYSCLK_DIVISOR);
    // pwm_init(LED_PWM_SLICE, &config, true);
    // pwm_set_wrap(1, 255); // 8 bit resolution
    // led_timer_start();
}


static void led_timer_start(void) {
    gpio_set_function(PIN_LED0, GPIO_FUNC_PWM);
    gpio_set_function(PIN_LED1, GPIO_FUNC_PWM);
    add_repeating_timer_us(-1000000 / LED_TIMER_UPDATE_HZ, led_timer_callback, NULL, &led_timer);
}


static bool led_timer_callback(repeating_timer_t *rt) {
    //perf_start(PERF_MATRIX);

    // // Disable LEDs while changing row
    // gpio_init(PIN_LED0);
    // gpio_init(PIN_LED1);
    // pwm_set_enabled(LED_PWM_SLICE, 0);

    // led_column = (led_column + 1) % NUM_COLUMNS;
    // set_column(led_column);
    // pwm_set_gpio_level(PIN_LED0, led_value[led_column]);
    // pwm_set_gpio_level(PIN_LED1, led_value[led_column+8]);

    // // Reenable with reset counter to avoid ghosting
    // pwm_set_counter(LED_PWM_SLICE, 0);
    // pwm_set_enabled(LED_PWM_SLICE, 1);
    // gpio_set_function(PIN_LED0, GPIO_FUNC_PWM);
    // gpio_set_function(PIN_LED1, GPIO_FUNC_PWM);

    //perf_end(PERF_MATRIX);
    return true; // keep repeating
}


void delay_us_in_isr(uint32_t us) {
    int64_t now = get_absolute_time();
    while (get_absolute_time() - now < us);
}


void hw_scan_buttons(void) {

    // // Stop scanning and blank LEDs while reading the buttons
    // cancel_repeating_timer(&led_timer);
    // gpio_init(PIN_LED0);
    // gpio_init(PIN_LED1);

    for (int row=0; row<4; row++) {
        set_sw_row(row);
        sleep_us(2);
        //delay_us_in_isr(10); // we should need this but don't for some reason
        btn_value[8*row+0] = gpio_get(PIN_COL0);
        btn_value[8*row+1] = gpio_get(PIN_COL1);
        btn_value[8*row+2] = gpio_get(PIN_COL2);
        btn_value[8*row+3] = gpio_get(PIN_COL3);
        btn_value[8*row+4] = gpio_get(PIN_COL4);
        btn_value[8*row+5] = gpio_get(PIN_COL5);
        btn_value[8*row+6] = gpio_get(PIN_COL6);
        btn_value[8*row+7] = gpio_get(PIN_COL7);
    }
    set_sw_row(-1);

    // set_column(led_column);
    // led_timer_start();
}


AudioBuffer get_audio_buffer(void) {
    // The Pico code uses audio_buffers but we don't want to expose that to
    // the main application code
    struct audio_buffer *abuf = take_audio_buffer(audio_pool, true);
    current_audio_buffer = abuf;

    AudioBuffer buffer;
    buffer.samples = (uint16_t*)abuf->buffer->bytes;
    buffer.sample_count = abuf->max_sample_count;
    return buffer;
}


void put_audio_buffer(AudioBuffer buffer) {
    current_audio_buffer->buffer->bytes = (uint8_t*)buffer.samples;
    current_audio_buffer->sample_count = current_audio_buffer->max_sample_count;

    give_audio_buffer(audio_pool, current_audio_buffer);
    current_audio_buffer = NULL;
}
