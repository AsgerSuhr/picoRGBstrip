#ifndef APA120_H_   /* Include guard */
#define APA120_H_

#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "apa102.pio.h"

#define PIN_CLK 2
#define PIN_DIN 3

#define N_LEDS 30
#define SERIAL_FREQ (5 * 1000 * 1000)
#define TABLE_SIZE (1 << 8)

// Global brightness value 0->31
#define APA102_BRIGHTNESS 16

void put_start_frame(PIO pio, uint sm);
void put_end_frame(PIO pio, uint sm);
void put_rgb888(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
// void testRGB();

#endif 