#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LED_COUNT (160 + 17)
// #define LED_COUNT 200

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} color_t;

void ws2812_init(void);
void ws2812_push(void);
void ws2812_terminate(void);

void ws2812_set_angle(float angle, float w, uint8_t brightness, uint8_t led_count);

// API
void ws2812_clear(void);
void ws2812_set_led(uint16_t id, const color_t *color);
void ws2812_set_led_all(const color_t *color);
void ws2812_set_led_recursive(int16_t id, const color_t *color);

extern const color_t black;
extern const color_t red;
extern const color_t green;
extern const color_t blue;
extern const color_t white;

color_t hsv2rgb(float h, float s, float v);
color_t color_dim(const color_t *c, float dim);

#endif // WS2812_H