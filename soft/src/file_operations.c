#include "file_operations.h"
#include "debug.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern RNG_HandleTypeDef hrng;
extern RTC_HandleTypeDef hrtc;

enum
{
    __static_check_fatfs_lfn = 1 / ((_USE_LFN == 1) ? 1 : 0)
};

static FIL file_log;

static void file_op_upd_count_bin(uint32_t *log_file_serial_num)
{
    FIL file_serial;
    FRESULT sts;
    bool exist_error = true;

    (*log_file_serial_num)++; // increment count [0 => 1]

    // check file exists
    const TCHAR *f_name = "serial.bin";
    sts = f_open(&file_serial, f_name, FA_READ);
    if(sts == FR_OK)
    {
        if(f_size(&file_serial) != sizeof(uint32_t))
        {
            debug("[E] serial.bin wrong size: %d\n", f_size(&file_serial));
        }
        else
        {
            UINT bytesRead = 0;
            sts = f_read(&file_serial, log_file_serial_num, sizeof(uint32_t), &bytesRead);
            if(sts == FR_OK && bytesRead == sizeof(uint32_t))
            {
                (*log_file_serial_num)++; // increment count [N => N+1]
                exist_error = false;
            }
            else
            {
                debug("[E] Read serial.bin error! bytes: %d sts: %s\n", bytesRead, ff_result_to_string(sts));
            }
        }
        f_close(&file_serial);
    }
    else
    {
        exist_error = true;
        debug("[E] No serial.bin file!\n");
    }

    if((sts = f_open(&file_serial, f_name, FA_WRITE | FA_CREATE_ALWAYS)) == FR_OK)
    {
        UINT testBytes;
        if((sts = f_write(&file_serial, log_file_serial_num, sizeof(log_file_serial_num), &testBytes)) != FR_OK)
        {
            debug("[E] Write serial.bin file failed: %s\n", ff_result_to_string(sts));
            (*log_file_serial_num)--;
        }
        else if(sizeof(log_file_serial_num) != testBytes)
        {
            debug("[E] Write serial.bin failed mismatch: %d writed: %d\n", sizeof(log_file_serial_num), testBytes);
            (*log_file_serial_num)--;
        }
        f_close(&file_serial);
    }
    else
    {
        debug("[E] Create serial.bin file failed: %s\n", ff_result_to_string(sts));
        (*log_file_serial_num)--;
    }
}

int file_op_log_enable(void)
{
    uint32_t log_file_serial_num = 0;

    file_op_upd_count_bin(&log_file_serial_num);
    // debug("New log file count iterator: %d\n", log_file_serial_num);

    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    char trans_str[64] = {0};

    HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);

    char path[128] = {0};
    snprintf(path, sizeof(path), "LOG_%06lu_20%02d%02d%02d_%02d%02d%02d_%c%c%c.log",
             log_file_serial_num % 1000000,
             date.Year, date.Month, date.Date,
             time.Hours, time.Minutes, time.Seconds,
             (char)((HAL_RNG_GetRandomNumber(&hrng) % 26) + 'a'),
             (char)((HAL_RNG_GetRandomNumber(&hrng) % 26) + 'a'),
             (char)((HAL_RNG_GetRandomNumber(&hrng) % 26) + 'a'));

    debug("Log file name: %s\n", path);

    FRESULT res = f_open(&file_log, path, FA_WRITE | FA_CREATE_ALWAYS);
    if(res == FR_OK)
    {
        debug("Enable logging\n");
        return 1;
    }

    debug("[E] Create log file failed: %s\n", ff_result_to_string(res));
    return 0;
}

int file_op_log(const void *buf, UINT len)
{
    UINT testBytes;
    FRESULT res = f_write(&file_log, buf, len, &testBytes);
    if(res != FR_OK)
    {
        debug("Write file failed: %s\n", ff_result_to_string(res));
        file_op_log_disable();
        return 0;
    }
    if(len != testBytes)
    {
        debug("Write file failed mismatch: %d writed: %d\n", len, testBytes);
        file_op_log_disable();
        return 0;
    }
    return 1;
}

void file_op_log_disable(void)
{
    f_close(&file_log);
    debug("Disable logging\n");
}

void file_op_init(void)
{
    FRESULT res = f_mount(&SDFatFS, "", 1);
    debug("Mount FS: %s\n", ff_result_to_string(res));
}

void file_op_test(void)
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
        res = f_write(&testFile, testBuffer, (UINT)strlen(testBuffer), &testBytes);
        debug("TWO: %s\n", ff_result_to_string(res));

        res = f_close(&testFile);
        debug("THREE: %s\n", ff_result_to_string(res));
    } while(0);
}