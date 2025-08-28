#ifndef LEDS_H
#define LEDS_H

#include "pico/stdlib.h"

#define LED_VERMELHO 13
#define LED_VERDE    11
#define LED_AZUL     12

void leds_init(void);
void leds_off(void);
void led_red(void);
void led_green(void);
void led_blue(void);

#endif