#include "adc.h"
#include "debounce.h"
#include "debug.h"
#include "dm.h"
#include "fatfs.h"
#include "file_operations.h"
#include "main.h"
#include "mpu6050.h"
#include "phys_engine.h"
#include "ws2812.h"
#include <stdio.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern CRC_HandleTypeDef hcrc;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;
extern IWDG_HandleTypeDef hiwdg;
extern I2C_HandleTypeDef hi2c1;

static bool logging_enabled = false;
static button_ctrl_t btn[4];

void init(void)
{
    LED0_GPIO_Port->ODR |= LED0_Pin;
    LED1_GPIO_Port->ODR |= LED1_Pin;
    LED2_GPIO_Port->ODR |= LED2_Pin;
    LED3_GPIO_Port->ODR |= LED3_Pin;

    __HAL_UART_ENABLE(&huart1);
    __HAL_UART_ENABLE(&huart3);
    __HAL_I2C_ENABLE(&hi2c1);

    adc_init();
    ws2812_init();
    ws2812_push();

    debounce_init(&btn[0], 100);
    debounce_init(&btn[1], 100);
    debounce_init(&btn[2], 100);
    debounce_init(&btn[3], 100);
    btn[0].pressed = true;

    PWR_EN_GPIO_Port->ODR |= PWR_EN_Pin;

    while(HAL_GetTick() < 800)
        asm("nop");

    mpu6050_init();
    file_op_init();
    dm_init();

    LED0_GPIO_Port->ODR &= (uint32_t)~LED0_Pin;
    LED1_GPIO_Port->ODR &= (uint32_t)~LED1_Pin;
    LED2_GPIO_Port->ODR &= (uint32_t)~LED2_Pin;
    LED3_GPIO_Port->ODR &= (uint32_t)~LED3_Pin;
}

void loop(void)
{
    // time diff
    static uint32_t time_ms_prev = 0;
    uint32_t time_ms_now = HAL_GetTick();
    uint32_t diff_ms = time_ms_now < time_ms_prev
                           ? 0xFFFFFFFF + time_ms_now - time_ms_prev
                           : time_ms_now - time_ms_prev;
    time_ms_prev = time_ms_now;

    dm_poll(diff_ms);

    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 1000;

        static float i = 0;
        i += 0.5f;

        debug("A: %.0f %.0f %.0f | G: %0.f %0.f %0.f | Vbat: %0.2f | Temp: %0.1f\n",
              accel_filt[0], accel_filt[1], accel_filt[2],
              gyro_filt[0], gyro_filt[1], gyro_filt[2],
              adc_get_v_bat(), adc_get_temp());
    }

    if(diff_ms > 0)
    {
        static uint32_t led0_cnt;
        led0_cnt += diff_ms;
        if(led0_cnt > 500) led0_cnt = 0;

        bool lit = logging_enabled ? led0_cnt > 80 : led0_cnt > 480;
        LED3_GPIO_Port->ODR ^= (-lit ^ LED3_GPIO_Port->ODR) & LED3_Pin;

        bool litb = get_percent_battery() < 0.3 ? led0_cnt > 400 : 0;
        LED0_GPIO_Port->ODR ^= (-litb ^ LED0_GPIO_Port->ODR) & LED0_Pin;
    }

    if(diff_ms > 0)
    {
        if(mpu6050_read_acc_gyro(diff_ms))
        {
            if(logging_enabled)
            {
                uint8_t buf[128] = "";

                snprintf(buf, sizeof(buf), "%d %d %d %d %d %d %d\r\n",
                         (int)HAL_GetTick(),
                         accel_raw[0],
                         accel_raw[1],
                         accel_raw[2],
                         gyro_raw[0],
                         gyro_raw[1],
                         gyro_raw[2]);

                size_t len = strlen(buf);

                if(file_op_log(buf, strlen(buf)) == 0) logging_enabled = false;
            }
        }
    }

    ws2812_push();

    static bool prev_btn[4] = {1, 1, 1, 1};

    debounce_cb(&btn[0], BTN0_GPIO_Port->IDR & BTN0_Pin, diff_ms);
    debounce_cb(&btn[1], !(BTN1_GPIO_Port->IDR & BTN1_Pin), diff_ms);
    debounce_cb(&btn[2], !(BTN2_GPIO_Port->IDR & BTN2_Pin), diff_ms);
    debounce_cb(&btn[3], !(BTN3_GPIO_Port->IDR & BTN3_Pin), diff_ms);

    static bool must_turn_off = false;
    // button processor
    {
        static bool shift_selection = false;
        if(shift_selection == false)
        {
            if(btn[0].pressed_shot)
            {
                if(dm_get_mode() == DM_BAT)
                {
                    file_op_log_disable();
                    must_turn_off = true;
                }

                dm_switch_mode_vbat();
            }
            if(btn[0].unpressed_shot && must_turn_off)
            {
                PWR_EN_GPIO_Port->ODR &= (uint32_t)(~PWR_EN_Pin);
            }

            if(btn[2].pressed_shot) dm_switch_parameter();
            if(btn[3].pressed_shot) dm_switch_mode_next();
        }
        else
        {
            if(btn[0].pressed_shot)
            {
                shift_selection = false;

                if(logging_enabled)
                {
                    file_op_log_disable();
                    logging_enabled = false;
                }
                else if(file_op_log_enable())
                {
                    logging_enabled = true;
                }
            }
        }

        if(btn[1].pressed_shot)
        {
            shift_selection = !shift_selection;
            if(shift_selection)
            {
                LED1_GPIO_Port->ODR |= LED1_Pin;
                LED2_GPIO_Port->ODR |= LED2_Pin;
            }
        }

        if(shift_selection == false)
        {
            LED1_GPIO_Port->ODR &= (uint32_t)~LED1_Pin;
            LED2_GPIO_Port->ODR &= (uint32_t)~LED2_Pin;
        }
    }

    for(uint8_t i = 0; i < 4; i++)
    {
        prev_btn[i] = btn[i].pressed;
    }

    HAL_IWDG_Refresh(&hiwdg);
}