#include "leds.h"

void leds_init(void) {
    gpio_init(LED_VERMELHO); gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_init(LED_VERDE);    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_AZUL);     gpio_set_dir(LED_AZUL, GPIO_OUT);
    leds_off();
}

void leds_off(void) {
    gpio_put(LED_VERMELHO, 0);
    gpio_put(LED_VERDE, 0);
    gpio_put(LED_AZUL, 0);
}

void led_red(void)   { leds_off(); gpio_put(LED_VERMELHO, 1); }
void led_green(void) { leds_off(); gpio_put(LED_VERDE, 1); }
void led_blue(void)  { leds_off(); gpio_put(LED_AZUL, 1); }