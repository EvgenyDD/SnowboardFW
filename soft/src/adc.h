#ifndef ADC_H
#define ADC_H

#include <stdint.h>

void adc_init(void);
void adc_print(void);

uint16_t adc_get_raw(uint32_t channel);

void adc_drv_conv_complete_half(void);
void adc_drv_conv_complete_full(void);

#endif // ADC_H