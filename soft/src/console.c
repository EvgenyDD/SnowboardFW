#include "adc.h"
#include "debug.h"
#include "main.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int strcmp_(char *s1, char *s2)
{
    for(;;)
    {
        if(*s1 == '\n' || *s1 == '\0') return 1;
        if(*s2 == '\n' || *s2 == '\0') return 1;
        if(*s1 != *s2) return 0;
        s1++;
        s2++;
    }
}

static int find_space(char *s)
{
    char *ss = s;
    for(;; ss++)
    {
        if(*ss == '\n' || *ss == '\0') return 0;
        if(*ss == ' ') return ss - s + 1;
    }
}

void debug_parse(char *s)
{
    if(strcmp_(s, "info"))
    {
        // debug("Popadalovo\n");

        // unsigned int state = 0;

        // int c = sscanf(&s[find_space(s)], "%u", &state);
        // if(c > 0)
        // {
        //     debug("YEAH %d\n", state);
        // }
        // servo_print();
        adc_print();
    }
    else
    {
        debug("Not Found!\n");
    }
}