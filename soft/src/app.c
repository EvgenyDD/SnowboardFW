#include "adc.h"
#include "debounce.h"
#include "debug.h"
#include "fatfs.h"
#include "main.h"
#include "phys_engine.h"
#include "ws2812.h"

extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;

extern CRC_HandleTypeDef hcrc;

extern RNG_HandleTypeDef hrng;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

I2C_HandleTypeDef hi2c1;

extern uint32_t rx_cnt;

button_ctrl_t btn[4];

void MPU6050_Write(uint8_t Reg, uint8_t Data)
{
    uint8_t d[2] = {Reg, Data};
    HAL_I2C_Master_Transmit(&hi2c1, 0x68 << 1, d, 2, 100);
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

    HAL_I2C_Master_Transmit(&hi2c1, 0x68 << 1, &Reg, 1, 100);
    static uint8_t Data;
    HAL_I2C_Master_Receive(&hi2c1, 0x68 << 1, &Data, 1, 100);
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

    PWR_EN_GPIO_Port->ODR |= PWR_EN_Pin;
    LED1_GPIO_Port->ODR ^= LED1_Pin;
    LED3_GPIO_Port->ODR ^= LED3_Pin;

    // MPU6050_I2C_init(); // Настраиваем I2C
    // for(uint32_t i32 = 0; i32 < 100000; i32++)
    // {
    // };

    //Датчик тактируется от встроенного 8Мгц осциллятора
    //MPU6050_Write(0x6B, 0x41); // Register_PWR_M1 = 0, Disable sleep mode
    MPU6050_Write(0x6B, 0x00);

    //Выполнить очистку встроенных регистров датчика
    MPU6050_Write(0x1b, 0x08); // Register_UsCtrl = 1

    MPU6050_Write(0x1C, 0x10); // Register_UsCtrl = 1

    // ws2812_set_led(80, blue);

    FRESULT res = f_mount(&SDFatFS, "0", 1);
    debug("Mount FS: %s\n", ff_result_to_string(res));
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

    static uint32_t prev_tick = 0;
    if(prev_tick < HAL_GetTick())
    {
        prev_tick = HAL_GetTick() + 500;
        LED0_GPIO_Port->ODR ^= LED0_Pin;
        LED3_GPIO_Port->ODR ^= LED3_Pin;

        static float i = 0;
        // debug("Fuckyou %.2f %d\n", i, rx_cnt);
        i += 0.5f;
    }

    static int16_t accel[3], gyro[3];

    if(diff_ms > 0)
    {
        // IMU
        {
            if(MPU6050_Read(0x3a) & 1)
            {
                accel[0] = MPU6050_Read(0x3B) << 8;
                accel[0] |= MPU6050_Read(0x3C);
                accel[1] = MPU6050_Read(0x3D) << 8;
                accel[1] |= MPU6050_Read(0x3E);
                accel[2] = MPU6050_Read(0x3F) << 8;
                accel[2] |= MPU6050_Read(0x40);
                // x = MPU6050_Read(0x43) << 8;
                // x |= MPU6050_Read(0x44);
                // y = MPU6050_Read(0x45) << 8;
                // y |= MPU6050_Read(0x46);
                // z = MPU6050_Read(0x47) << 8;
                // z |= MPU6050_Read(0x48);
            }
        }

        // phys
        {

            float ts = (float)diff_ms * 0.001f;

            // static float angle = 1; //3.14159256/2;
            float angle = -0.42f +
                          approx_atan2((float)accel[1] * 0.00024420024 /*1/4095*/,
                                       (float)accel[2] * 0.00024420024); //3.14159256/2;

            phys_engine_poll(ts, angle);

            // ws2812_set_angle(phys_engine_get_angle(),
            //                  phys_engine_get_w() * 20.0f);
        }
    }

    ws2812_push();

    static float color_ring = 0;

    static uint32_t ptr = 0;
    static uint32_t cnt = 0;
    // if(cnt < HAL_GetTick())
    if(0)
    {
        cnt = HAL_GetTick() + 5;

        color_ring += 0.2;
        if(color_ring >= 360) color_ring = 0;

        if(ptr >= LED_COUNT)
        {
            ptr = 0;
        }
        // ws2812_clear();

        // int rnd = rand() % 4;
        // ws2812_set_led(ptr, rnd == 0 ? red : rnd == 1 ? green : rnd == 2 ? blue : white);
        ws2812_set_led(ptr, hsv2rgb(color_ring, 1.0, 255.0));
        // ws2812_set_led(ptr, HSVtoRGB(color_ring));
        ws2812_set_led(ptr >= 10 ? ptr - 10 : 160 - 10 + ptr, black);

        ptr++;
    }

    static bool prev_btn[4] = {0};

    debounce_cb(&btn[0], BTN0_GPIO_Port->IDR & BTN0_Pin, diff_ms);
    debounce_cb(&btn[1], !(BTN1_GPIO_Port->IDR & BTN1_Pin), diff_ms);
    debounce_cb(&btn[2], !(BTN2_GPIO_Port->IDR & BTN2_Pin), diff_ms);
    debounce_cb(&btn[3], !(BTN3_GPIO_Port->IDR & BTN3_Pin), diff_ms);

    static bool first_press = false;

    for(uint8_t i = 0; i < 4; i++)
    {
        if(btn[i].pressed != prev_btn[i])
        {
            if(i == 1)
                phys_engine_reset();
            if(i == 0)
            {
                // PWR Button
                if(btn[i].pressed == false)
                {
                    if(first_press == false)
                        first_press = true;
                    else
                        PWR_EN_GPIO_Port->ODR &= (uint32_t)(~PWR_EN_Pin);
                }
            }

            debug("Btn %d %s\n", i, btn[i].pressed ? "pressed" : "released");

            if(i == 2)
            {
                adc_print();
                debug("A=%d\n", accel[0]);
                debug("A=%d\n", accel[1]);
                debug("A=%d\n", accel[2]);
                debug("G=%d\n", gyro[0]);
                debug("G=%d\n", gyro[1]);
                debug("G=%d\n", gyro[2]);
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
                    res = f_open(&testFile, (char *)path, FA_WRITE | FA_CREATE_NEW);

                    debug("ONE: %s\n", ff_result_to_string(res));

                    UINT testBytes;
                    res = f_write(&testFile, testBuffer, strlen(testBuffer), &testBytes);
                    debug("TWO: %s\n", ff_result_to_string(res));

                    res = f_close(&testFile);
                    debug("THREE: %s\n", ff_result_to_string(res));
                } while(0);
            }
        }
        prev_btn[i] = btn[i].pressed;
    }
}