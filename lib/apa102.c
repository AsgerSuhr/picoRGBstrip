/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "apa102.h"


void put_start_frame(PIO pio, uint sm) {
    pio_sm_put_blocking(pio, sm, 0u);
}

void put_end_frame(PIO pio, uint sm) {
    pio_sm_put_blocking(pio, sm, ~0u);
}

void put_rgb888(PIO pio, uint sm, uint8_t r, uint8_t g, uint8_t b, uint8_t brightness) {
    pio_sm_put_blocking(pio, sm,
                        0x7 << 29 |                   // magic
                        (brightness & 0x1f) << 24 |   // global brightness parameter
                        (uint32_t) b << 16 |
                        (uint32_t) g << 8 |
                        (uint32_t) r << 0
    );
}

/* 
void testRGB(bool *cnd) {
    // stdio_init_all();

    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &apa102_mini_program);
    apa102_mini_program_init(pio, sm, offset, SERIAL_FREQ, PIN_CLK, PIN_DIN);

    for (int i = 0; i < TABLE_SIZE; ++i)
        wave_table[i] = powf(sinf(i * M_PI / TABLE_SIZE), 5.f) * 255;

    uint t = 0;
    while (*cnd) {
        put_start_frame(pio, sm);
        for (int i = 0; i < N_LEDS; ++i) {
            put_rgb888(pio, sm,
                       wave_table[(i + t) % TABLE_SIZE],
                       wave_table[(2 * i + 3 * 2) % TABLE_SIZE],
                       wave_table[(3 * i + 4 * t) % TABLE_SIZE]
            );
        }
        put_end_frame(pio, sm);
        sleep_ms(10);
        ++t;
    }
}
 */