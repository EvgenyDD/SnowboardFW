#include "adc.h"
#include "debug.h"
#include "main.h"
#include <math.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim3;

static const float v_ref = 3.3f;
static const float adc_max_cnt = 4095.0f;

#define v_ref 3.3f
#define adc_max_cnt 4095.0f

enum
{
    ADC_VBAT,
    ADC_TEMP,

    ADC_CH_NUM
};
uint32_t adc_sel = 0; // offset, values: 0 or ADC_CH_NUM
static uint16_t adc_raw[ADC_CH_NUM * 2];

void adc_init(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_raw, ADC_CH_NUM * 2);
    __HAL_TIM_ENABLE(&htim3);
}

void adc_print(void)
{
    // for(uint32_t i = 0; i < ADC_CH_NUM; i++)
    // {
    //     debug("ADC: %d %d\n", i, adc_get_raw(i));
    // }
    debug("Vbat: %.2f\n", adc_get_v_bat());
    debug("Temp: %.2f\n", adc_get_temp());
}

uint16_t adc_get_raw(uint32_t channel) { return adc_raw[adc_sel + channel]; }

void adc_drv_conv_complete_half(void)
{
    adc_sel = 0;
    // filter_values();
}

void adc_drv_conv_complete_full(void)
{
    adc_sel = ADC_CH_NUM;
    // filter_values();
}

float adc_get_v_bat(void) { return adc_get_raw(ADC_VBAT) * 0.0049008f + 0.020111f; }
float adc_get_temp(void) { return (adc_get_raw(ADC_TEMP) / adc_max_cnt * v_ref - 0.76f) * 400.0f + 25.0f; }

float map(float x, float in_min, float in_max, float out_min, float out_max)
{
    if(x < in_min)
    {
        x = in_min;
    }
    if(x > in_max)
    {
        x = in_max;
    }
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float get_percent_battery(void) { return map(adc_get_v_bat(), 8.0f, 14.6f, 0, 1.0f); }
