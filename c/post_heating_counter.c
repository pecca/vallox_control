/**
 * @file   post_heating_counter.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of post heating counter. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 

#include "common.h"
#include "digit_protocol.h"
#include "ctrl_logic.h"

/******************************************************************************
 *  Constants
 ******************************************************************************/
 
#define BACKUP_FILE_NAME                        "post_heating_value.txt"
#define BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC  (60*60) // once per hour
#define POST_HEATING_COUNTER_UPDATE_INTERVAL    CTRL_LOGIC_TIMELEVEL

/******************************************************************************
 *  Local variables
 ******************************************************************************/
 
static uint32 g_u32OnTimeTotal;
static uint32 g_u32UpdateCallCnt = 0;
static bool g_bPostHeatingStarted = false;

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/

static void read_on_time_from_file();
static void save_on_time_to_file();
 
/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void post_heating_counter_init()
{
    read_on_time_from_file();
}

void post_heating_counter_update()
{
    uint8 u8OnCnt = (uint8) r32_digit_post_heating_on_cnt();
    uint8 u8OffCnt = (uint8) r32_digit_post_heating_off_cnt();
        
    g_u32UpdateCallCnt++;
    if ( (g_u32UpdateCallCnt * POST_HEATING_COUNTER_UPDATE_INTERVAL) % 
         BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC == 0)
    {
        save_on_time_to_file();
    }

    // calculate post heating seconds
    if ( g_bPostHeatingStarted == false && 
         u8OnCnt > 0 && 
         !(u8OffCnt % 10) )
    {

        g_bPostHeatingStarted = true;
        g_u32OnTimeTotal += (100 - u8OffCnt);
    }
    else if (u8OnCnt == 0)
    {
        g_bPostHeatingStarted = false;
    }
}

uint32 u32_post_heating_counter_get_on_time_total()
{
    return g_u32OnTimeTotal;
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

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
