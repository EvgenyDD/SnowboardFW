#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LED_COUNT 160
// #define LED_COUNT 200

typedef struct
{
    uint8_t color_r;
    uint8_t color_g;
    uint8_t color_b;
} Color_t;

void ws2812_init(void);
void ws2812_push(void);
void ws2812_terminate(void);
void ws2812_set_angle(float angle, float w);

// API
void ws2812_clear(void);
void ws2812_set_led(uint16_t id, Color_t color);

extern const Color_t black;
extern const Color_t red;
extern const Color_t green;
extern const Color_t blue;
extern const Color_t white;

Color_t hsv2rgb(float h, float s, float v);

#endif // WS2812_H