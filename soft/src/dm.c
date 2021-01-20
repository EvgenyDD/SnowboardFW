#include "dm.h"
#include "adc.h"
#include "debug.h"
#include "main.h"
#include "mpu6050.h"
#include "ws2812.h"
#include <math.h>

#include "phys_engine.h"

extern RNG_HandleTypeDef hrng;

DM_SHOW_T show = DM_IDLE, show_next = DM_IDLE;
uint32_t to = 0;

float param[DM_COUNT] = {0};

DM_SHOW_T dm_get_mode(void) { return show; }

#define PERIODIC_DECIMATION_CNT(value, diff, ...) \
    {                                             \
        static uint32_t cnt = 0;                  \
        cnt += diff;                              \
        if(cnt >= (value))                        \
        {                                         \
            cnt = 0;                              \
            __VA_ARGS__;                          \
        }                                         \
    }

void dm_switch(DM_SHOW_T mode)
{
    show = mode;
    debug("DM switch: %d\n", show);
}

void dm_switch_parameter(void)
{
    float *p = &param[show];
    switch(show)
    {
    default: break;

    case DM_RAINBOW_ROTATE:
        *p += 30;
        if(*p < 100 || *p > 200) *p = 100;
        break;

    case DM_PENDULUM:
        *p += 30;
        if(*p < 100 || *p > 220) *p = 100;
        break;

    case DM_RIDE0:
        *p += 20;
        if(*p < 80 || *p > 150) *p = 80;
        break;
    }

    debug("\tparam: %.3f\n", *p);
}

void dm_init(void)
{
    for(int i = 0; i < DM_COUNT; i++)
    {
        show = (DM_SHOW_T)i;
        dm_switch_parameter();
    }
    show = DM_IDLE;
}

void dm_switch_mode_vbat(void)
{
    to = 2000;
    show_next = show;
    dm_switch(DM_BAT);
}

void dm_switch_mode_sparkle(void)
{
    to = 1200;
    show_next = show;
    dm_switch(DM_SPARKLE);
    param[show] = HAL_RNG_GetRandomNumber(&hrng) % 6;
}

void dm_switch_mode_next(void)
{
    show++;
    if(show >= DM_COUNT || show < DM_IDLE) show = DM_IDLE;

    dm_switch(show);

    switch(show)
    {
    default: break;
    case DM_PENDULUM: phys_engine_reset(); break;
    }
    ws2812_clear();
}

void dm_poll(uint32_t diff_ms)
{
    if(to)
    {
        if(to <= diff_ms)
        {
            to = 0;
            show = show_next;
            debug("DM TO switch: %d\n", show);
            ws2812_clear();
        }
        else
            to -= diff_ms;
    }

    switch(show)
    {
    default:
    case DM_IDLE: break;

    case DM_BAT:
    {
        uint32_t light = get_percent_battery() * LED_COUNT;
        Color_t color_light = {140, 0, 0};
        for(uint32_t i = 0; i < light; i++)
        {
            ws2812_set_led(i, &color_light);
        }
        for(uint32_t i = light; i < LED_COUNT; i++)
        {
            ws2812_set_led(i, &black);
        }
    }
    break;

    case DM_RAINBOW_ROTATE:
    {
        static uint32_t offset = 0;
        offset += diff_ms;
        uint32_t offset_scaled = offset / 30;
        if(offset_scaled == LED_COUNT) offset = offset_scaled = 0;

        for(uint32_t i = 0; i < LED_COUNT; i++)
        {
            int32_t hue = (int32_t)(offset_scaled + i) * 360 / LED_COUNT;
            hue %= 360;
            Color_t light_color = hsv2rgb(hue, 1.0f, param[show]);
            ws2812_set_led(i, &light_color);
        }
    }
    break;

    case DM_PENDULUM:
    {
        float ts = (float)diff_ms * 0.001f;

        // static float angle = 1; //3.14159256/2;
        float angle = -0.42f +
                      approx_atan2((float)accel_filt[1] * 0.00024420024 /*1/4095*/,
                                   (float)accel_filt[2] * 0.00024420024); //3.14159256/2;

        phys_engine_poll(ts, angle);

        ws2812_set_angle(phys_engine_get_angle(), phys_engine_get_w() * 20.0f, param[show], 25 /* led count */);

        // static float color_ring = 0;

        // static uint32_t ptr = 0;
        // static uint32_t cnt = 0;
        // if(cnt < HAL_GetTick())
        // {
        //     cnt = HAL_GetTick() + 5;

        //     color_ring += 0.2;
        //     if(color_ring >= 360) color_ring = 0;

        //     if(ptr >= LED_COUNT)
        //     {
        //         ptr = 0;
        //     }
        //     // ws2812_clear();

        //     // int rnd = rand() % 4;
        //     // ws2812_set_led(ptr, rnd == 0 ? red : rnd == 1 ? green : rnd == 2 ? blue : white);
        //     ws2812_set_led(ptr, hsv2rgb(color_ring, 1.0, 255.0));
        //     // ws2812_set_led(ptr, HSVtoRGB(color_ring));
        //     ws2812_set_led(ptr >= 10 ? ptr - 10 : 160 - 10 + ptr, black);

        //     ptr++;
        // }
    }
    break;

    case DM_POLICE:
    {
        enum
        {
            PHASE_RED_0,
            PHASE_RED_1,
            PHASE_RED_OFF,
            PHASE_BLU_0,
            PHASE_BLU_1,
            PHASE_BLU_OFF
        };

#define FLASH_CNT 30
#define FLASH_COLOR_LEN 35
#define FLASH_DELAY 200

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"

        static uint32_t phase = PHASE_BLU_OFF;
        if(phase == PHASE_BLU_OFF || phase == PHASE_RED_OFF)
        {
            ws2812_clear();
            PERIODIC_DECIMATION_CNT(FLASH_DELAY, diff_ms, { phase = phase == PHASE_BLU_OFF ? PHASE_RED_0 : PHASE_BLU_0; });
        }
        else if(phase == PHASE_RED_0 || phase == PHASE_RED_1)
        {
            PERIODIC_DECIMATION_CNT(FLASH_COLOR_LEN, diff_ms, {
                if(phase == PHASE_RED_0)
                {
                    phase = PHASE_RED_1;

                    Color_t c = {235, 0, 0};
                    for(uint32_t i = 0; i < LED_COUNT / 2; i++)
                    {
                        ws2812_set_led_recursive(i - 10, &c);
                    }
                }
                else
                {
                    phase = PHASE_RED_0;
                    ws2812_clear();
                }
                PERIODIC_DECIMATION_CNT(FLASH_CNT, diff_ms, { phase = PHASE_RED_OFF; });
            });
        }
        else if(phase == PHASE_BLU_0 || phase == PHASE_BLU_1)
        {
            PERIODIC_DECIMATION_CNT(FLASH_COLOR_LEN, diff_ms, {
                if(phase == PHASE_BLU_0)
                {
                    phase = PHASE_BLU_1;

                    Color_t c = {0, 0, 200};
                    for(uint32_t i = LED_COUNT / 2; i < LED_COUNT; i++)
                    {
                        ws2812_set_led_recursive(i - 10, &c);
                    }
                }
                else
                {
                    phase = PHASE_BLU_0;
                    ws2812_clear();
                }
                PERIODIC_DECIMATION_CNT(FLASH_CNT, diff_ms, { phase = PHASE_BLU_OFF; });
            });
        }

#pragma GCC diagnostic pop
    }
    break;

    case DM_SPARKLE:
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
        static bool is_lit = true;
        PERIODIC_DECIMATION_CNT(FLASH_COLOR_LEN, diff_ms, {
            if(is_lit)
            {
                is_lit = false;

                Color_t c[] = {
                    {230, 0, 0},
                    {230, 210, 0},
                    {0, 210, 0},
                    {0, 210, 210},
                    {0, 0, 210},
                    {230, 0, 210},
                };

                for(uint32_t i = 0; i < LED_COUNT; i += 5)
                {
                    ws2812_set_led(i++, &c[(int)param[show]]);
                    ws2812_set_led(i++, &c[(int)param[show]]);
                }
            }
            else
            {
                ws2812_clear();
                is_lit = true;
            }
        });
#pragma GCC diagnostic pop
    }
    break;

    case DM_RIDE0:
    {
        if(accel_filt[0] > 3000 && fabsf(accel_filt[1] < 2500) && fabsf(accel_filt[2]) < 2500)
            dm_switch_mode_sparkle();
        else
        {
            static float hue = 0;
            hue += (float)diff_ms / 30.0f;
            if(hue >= 360.0f) hue = 0;

            Color_t c = hsv2rgb(hue, 1.0, param[show]);

            if(accel_filt[1] < -500)
            {
                ws2812_clear();
                for(uint32_t i = 0; i < 15; i++)
                    ws2812_set_led(i, &c);
                for(uint32_t i = 125; i < LED_COUNT; i++)
                    ws2812_set_led(i, &c);

                int offsetp = map(accel_filt[1], -3000, -500, 15, 45);
                int offsetn = map(accel_filt[1], -3000, -500, 125, 95);

                for(int32_t i = 15; i < offsetp; i++)
                    ws2812_set_led(i, &c);

                for(int32_t i = 125; i > offsetn; i--)
                    ws2812_set_led(i, &c);
            }
            else if(accel_filt[1] > 500)
            {
                ws2812_clear();
                for(uint32_t i = 45; i < 95; i++)
                    ws2812_set_led(i, &c);

                int offsetp = map(accel_filt[1], 500, 3000, 15, 45);
                int offsetn = map(accel_filt[1], 500, 3000, 125, 95);

                for(int32_t i = 95; i < offsetn; i++)
                    ws2812_set_led(i, &c);

                for(int32_t i = 45; i > offsetp; i--)
                    ws2812_set_led(i, &c);
            }
            else
            {
                for(uint32_t i = 0; i < LED_COUNT; i++)
                    ws2812_set_led(i, &c);
            }
        }
    }
    break;

    case DM_RIDE_SPARKLE:
    {
        if(accel_filt[0] > 2000 && fabsf(accel_filt[1] < 2500) && fabsf(accel_filt[2]) < 2500)
            dm_switch_mode_sparkle();
        Color_t x = {100, 0, 0};
        ws2812_set_led(LED_COUNT / 2, &x);
    }
    break;
    }
}