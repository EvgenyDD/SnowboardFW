#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
void adc_print(void);

uint16_t adc_get_raw(uint32_t channel);

void adc_drv_conv_complete_half(void);
void adc_drv_conv_complete_full(void);

float adc_get_v_bat(void);
float adc_get_temp(void);

float get_percent_battery(void);

float map(float x, float in_min, float in_max, float out_min, float out_max);

#define UTILS_LP_FAST(value, sample, filter_constant) ((value) = (value) - (filter_constant) * ((value) - (sample)))

#endif // ADC_H