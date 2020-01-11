#include "adc.h"
#include "debounce.h"
#include "debug.h"
#include "dm.h"
#include "fatfs.h"
#include "main.h"
#include "phys_engine.h"
#include "ws2812.h"
#include <stdio.h>

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;

extern RNG_HandleTypeDef hrng;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

extern IWDG_HandleTypeDef hiwdg;

bool logging_enabled = false;

FIL file_log;

I2C_HandleTypeDef hi2c1;

button_ctrl_t btn[4];

float accel[3], gyro[3];
int16_t _accel[3], _gyro[3];

void MPU6050_Write(uint8_t Reg, uint8_t Data)
{
    uint8_t d[2] = {Reg, Data};
    HAL_StatusTypeDef sts = HAL_I2C_Master_Transmit(&hi2c1, 0x68 << 1, d, 2, 100);
    if(sts != 0)
    {
        // debug("FAIL: %d\n", sts);
    }
    // while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
    //     ;
    // I2C_GenerateSTART(I2C1, ENABLE);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    //     ;
    // I2C_Send7bitAddress(I2C1, (0x68 << 1), I2C_Direction_Transmitter);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    //     ;
    // I2C_SendData(I2C1, Reg); // Передаём адрес регистра
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    //     ;
    // I2C_SendData(I2C1, Data); // Передаём данные
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    //     ;
    // I2C_GenerateSTOP(I2C1, ENABLE);
}

uint8_t MPU6050_Read(uint8_t Reg)
{

    HAL_StatusTypeDef sts = HAL_I2C_Master_Transmit(&hi2c1, 0x68 << 1, &Reg, 1, 100);
    if(sts != 0)
    {
        // debug("FAIL: %d\n", sts);
    }
    static uint8_t Data;
    sts = HAL_I2C_Master_Receive(&hi2c1, 0x68 << 1, &Data, 1, 100);
    if(sts != 0)
    {
        // debug("FAIL: %d\n", sts);
    }
    return Data;
    // while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY))
    //     ;
    // I2C_GenerateSTART(I2C1, ENABLE);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    //     ;
    // I2C_Send7bitAddress(I2C1, (0x68 << 1), I2C_Direction_Transmitter);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
    //     ;
    // I2C_Cmd(I2C1, ENABLE);
    // I2C_SendData(I2C1, Reg); // Передаём адрес регистра
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED))
    //     ;
    // I2C_GenerateSTART(I2C1, ENABLE);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT))
    //     ;
    // I2C_Send7bitAddress(I2C1, (0x68 << 1), I2C_Direction_Receiver);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
    //     ;
    // I2C_AcknowledgeConfig(I2C1, DISABLE);
    // I2C_GenerateSTOP(I2C1, ENABLE);
    // while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
    //     ;
    // Data = I2C_ReceiveData(I2C1); // Принимаем данные
    // I2C_AcknowledgeConfig(I2C1, ENABLE);
    // return Data;
}

void init(void)
{
    LED0_GPIO_Port->ODR |= LED0_Pin;
    LED1_GPIO_Port->ODR |= LED1_Pin;
    LED2_GPIO_Port->ODR |= LED2_Pin;
    LED3_GPIO_Port->ODR |= LED3_Pin;

    __HAL_UART_ENABLE(&huart1);
    __HAL_I2C_ENABLE(&hi2c1);
    // __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    adc_init();
    ws2812_init();
    ws2812_push();

    debounce_init(&btn[0], 100);
    debounce_init(&btn[1], 100);
    debounce_init(&btn[2], 100);
    debounce_init(&btn[3], 100);
    btn[0].pressed = true;

    PWR_EN_GPIO_Port->ODR |= PWR_EN_Pin;

    // LED1_GPIO_Port->ODR ^= LED1_Pin;
    // LED3_GPIO_Port->ODR ^= LED3_Pin;

    // MPU6050_I2C_init(); // Настраиваем I2C
    // for(uint32_t i32 = 0; i32 < 100000; i32++)
    // {
    // };

    while(HAL_GetTick() < 800)
        asm("nop");

    //Датчик тактируется от встроенного 8Мгц осциллятора
    //MPU6050_Write(0x6B, 0x41); // Register_PWR_M1 = 0, Disable sleep mode
    MPU6050_Write(0x6B, 0x00);

    //Выполнить очистку встроенных регистров датчика
    MPU6050_Write(0x1b, 0x08); // Register_UsCtrl = 1

    MPU6050_Write(0x1C, 0x10); // Register_UsCtrl = 1

    // ws2812_set_led(80, blue);

    FRESULT res = f_mount(&SDFatFS, "", 1);
    debug("Mount FS: %s\n", ff_result_to_string(res));

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
        prev_tick = HAL_GetTick() + 500;
        // LED0_GPIO_Port->ODR ^= LED0_Pin;
        // LED3_GPIO_Port->ODR ^= LED3_Pin;

        static float i = 0;
        i += 0.5f;

        debug("A=%.0f %.0f %.0f\n", accel[0], accel[1], accel[2]);
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
        // IMU
        {
            uint8_t dat = MPU6050_Read(0x3a);

            static uint32_t fail_read = 0;

            if(dat & 1)
            {
                fail_read = 0;

                LED2_GPIO_Port->ODR ^= LED2_Pin;
                _accel[0] = MPU6050_Read(0x3B) << 8;
                _accel[0] |= MPU6050_Read(0x3C);
                _accel[1] = MPU6050_Read(0x3D) << 8;
                _accel[1] |= MPU6050_Read(0x3E);
                _accel[2] = MPU6050_Read(0x3F) << 8;
                _accel[2] |= MPU6050_Read(0x40);
                _gyro[0] = MPU6050_Read(0x43) << 8;
                _gyro[0] |= MPU6050_Read(0x44);
                _gyro[1] = MPU6050_Read(0x45) << 8;
                _gyro[1] |= MPU6050_Read(0x46);
                _gyro[2] = MPU6050_Read(0x47) << 8;
                _gyro[2] |= MPU6050_Read(0x48);

                for(int i = 0; i < 3; i++)
                {
                    UTILS_LP_FAST(accel[i], (float)_accel[i], 0.2);
                    UTILS_LP_FAST(gyro[i], (float)_gyro[i], 0.2);
                }

                if(logging_enabled)
                {
                    uint8_t buf[128] = "";

                    snprintf(buf, sizeof(buf), "%d %d %d %d %d %d %d\r\n",
                             (int)HAL_GetTick(),
                             _accel[0],
                             _accel[1],
                             _accel[2],
                             _gyro[0],
                             _gyro[1],
                             _gyro[2]);

                    UINT testBytes;
                    size_t len = strlen(buf);
                    FRESULT res = f_write(&file_log, buf, strlen(buf), &testBytes);
                    if(res != FR_OK)
                    {
                        debug("Write file failed: %s\n", ff_result_to_string(res));
                        f_close(&file_log);
                    }
                    if(len != testBytes)
                    {
                        debug("Write file failed mismatch: %d writed: %d\n", len, testBytes);
                        f_close(&file_log);
                    }
                }
            }
            else
            {
                fail_read += diff_ms;
                if(fail_read > 200)
                {
                    for(int i = 0; i < 3; i++)
                    {
                        accel[i] = gyro[i] = 0;
                    }
                }
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
                    f_close(&file_log);
                    debug("Disable logging\n");
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

                if(logging_enabled)
                {
                    f_close(&file_log);
                    logging_enabled = false;
                    debug("Disable logging\n");
                }
                else
                {
                    char path[] = "l******.log";

                    for(uint32_t i = 1; i <= 6; i++)
                    {
                        path[i] = (char)((HAL_RNG_GetRandomNumber(&hrng) % 26) + 'a');
                    }

                    debug("Log file name: %s\n", path);

                    FRESULT res = f_open(&file_log, path, FA_WRITE | FA_CREATE_ALWAYS);
                    debug("Open log file: %s\n", ff_result_to_string(res));
                    if(res == FR_OK)
                    {
                        debug("Enable logging\n");
                        logging_enabled = true;
                    }
                }

                shift_selection = false;
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

    if(0)
        for(uint8_t i = 0; i < 4; i++)
        {
            if(btn[i].pressed != prev_btn[i])
            {
                if(i == 2)
                {
                    // debug("Vbat percent: %.3f\n", get_percent_battery());
                    adc_print();
                    // debug("A=%d\n", accel[0]);
                    // debug("A=%d\n", accel[1]);
                    // debug("A=%d\n", accel[2]);
                    // debug("G=%d\n", gyro[0]);
                    // debug("G=%d\n", gyro[1]);
                    // debug("G=%d\n", gyro[2]);
                }

                if(i == 3 && btn[i].pressed)
                {
                    do
                    {
                        uint32_t freeClust;
                        FATFS *fs_ptr = &SDFatFS;
                        FRESULT res = f_getfree("", &freeClust, &fs_ptr);
                        if(res != FR_OK)
                        {
                            debug("f_getfree() failed, res = %d\r\n", res);
                            break;
                        }

                        uint32_t totalBlocks = (SDFatFS.n_fatent - 2) * SDFatFS.csize;
                        uint32_t freeBlocks = freeClust * SDFatFS.csize;
                        debug("Total blocks: %lu (%lu Mb)\r\n", totalBlocks, totalBlocks / 2000);
                        debug("Free blocks: %lu (%lu Mb)\r\n", freeBlocks, freeBlocks / 2000);

                        DIR dir;
                        res = f_opendir(&dir, "/");
                        if(res != FR_OK)
                        {
                            debug("f_opendir() failed, res = %d\r\n", res);
                            break;
                        }

                        FILINFO fileInfo;
                        uint32_t totalFiles = 0;
                        uint32_t totalDirs = 0;
                        debug("--------\r\nRoot directory:\r\n");
                        for(;;)
                        {
                            res = f_readdir(&dir, &fileInfo);
                            if((res != FR_OK) || (fileInfo.fname[0] == '\0'))
                            {
                                break;
                            }

                            if(fileInfo.fattrib & AM_DIR)
                            {
                                debug("  DIR  %s\r\n", fileInfo.fname);
                                totalDirs++;
                            }
                            else
                            {
                                debug("  FILE %s\r\n", fileInfo.fname);
                                totalFiles++;
                            }
                        }

                        debug("(total: %lu dirs, %lu files)\r\n--------\r\n",
                              totalDirs, totalFiles);

                        FIL testFile;
                        uint8_t path[] = "fuckthe shit";

                        uint8_t testBuffer[16] = "SD write success";
                        res = f_open(&testFile, (char *)path, FA_READ | FA_WRITE | FA_OPEN_EXISTING);

                        debug("ONE: %s\n", ff_result_to_string(res));

                        UINT testBytes;
                        res = f_write(&testFile, testBuffer, strlen(testBuffer), &testBytes);
                        debug("TWO: %s\n", ff_result_to_string(res));

                        res = f_close(&testFile);
                        debug("THREE: %s\n", ff_result_to_string(res));
                    } while(0);
                }
            }
        }

    {
        for(uint8_t i = 0; i < 4; i++)
        {
            prev_btn[i] = btn[i].pressed;
        }
    }

    HAL_IWDG_Refresh(&hiwdg);
}