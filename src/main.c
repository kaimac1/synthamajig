#include <stdio.h>
#include "pico/stdlib.h"

#define PICO_LED_PIN 25

int main() {
    stdio_init_all();
    gpio_init(PICO_LED_PIN);
    gpio_set_dir(PICO_LED_PIN, GPIO_OUT);    
    bool led = true;
    while (true) {
        gpio_put(PICO_LED_PIN, led);
        printf("Hello, world!\n");
        sleep_ms(1000);
        led = !led;
    }
}