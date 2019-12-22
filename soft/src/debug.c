#include "debug.h"
#include "main.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern UART_HandleTypeDef huart1;

uint32_t rx_cnt = 0;

#define CON_OUT_BUF_SZ 512
#define CON_IN_BUF_SZ 512

char buffer_rx[CON_IN_BUF_SZ];
int buffer_rx_cnt = 0;

void debug_parse(char *s);

void debug(char *format, ...)
{
    static char buffer[CON_OUT_BUF_SZ + 1];
    va_list ap;

    va_start(ap, format);
    vsnprintf(buffer, CON_OUT_BUF_SZ, format, ap);
    va_end(ap);

    HAL_UART_Transmit(&huart1, buffer, strlen(buffer), 200);
}

void debug_rx(char x)
{
    if(x == '\n')
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx[buffer_rx_cnt + 1] = '\0';
        debug_parse(buffer_rx);
        buffer_rx_cnt = 0;
    }
    else
    {
        buffer_rx[buffer_rx_cnt] = x;
        buffer_rx_cnt++;
    }
}

void debug_poll(void)
{
}