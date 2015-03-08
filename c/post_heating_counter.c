
#include <stdio.h>
#include <math.h>
#include "post_heating_counter.h"
#include "digit_protocol.h"

#define BACKUP_FILE_NAME "post_heating_value.txt"

#define BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC (60*60) // once per hour

static uint32 g_u32OnTimeTotal;
static uint32 g_u32UpdateCallCnt = 0;
static bool g_bPostHeatingStarted = false;

static void read_on_time_from_file()
{
    FILE *file = fopen(BACKUP_FILE_NAME, "r");
    if (file)
    {
        fscanf(file, "%d", &g_u32OnTimeTotal);
        fclose(file);
    }
}

static void save_on_time_to_file()
{
    FILE *file = fopen(BACKUP_FILE_NAME, "w");
    if (file)
    {
        fprintf(file, "%d\n", g_u32OnTimeTotal);
        fclose(file);
    }
}

void post_heating_counter_init()
{
    read_on_time_from_file();
}

void post_heating_counter_update()
{
    byte on_cnt = (byte) r32_digit_post_heating_on_cnt();
    byte off_cnt = (byte) r32_digit_post_heating_off_cnt();
		
    g_u32UpdateCallCnt++;
    if ( (g_u32UpdateCallCnt * POST_HEATING_COUNTER_UPDATE_INTERVAL) % 
         BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC == 0)
    {
        save_on_time_to_file();
    }

    // calculate post heating seconds
    if ( g_bPostHeatingStarted == false && 
         on_cnt > 0 && 
         !(off_cnt % 10) )
    {

        g_bPostHeatingStarted = true;
        g_u32OnTimeTotal += (100 - off_cnt);
    }
    else if (on_cnt == 0)
    {
        g_bPostHeatingStarted = false;
    }
}

uint32 post_heating_counter_get_on_time_total()
{
    return g_u32OnTimeTotal;
}
