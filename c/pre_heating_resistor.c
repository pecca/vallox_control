/**
 * @file   pre_heating_resistor.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of pre-heating resistor. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 

#include "common.h"
#include "relay_control.h"
#include "pre_heating_resistor.h"
#include "ctrl_logic.h"
 
/******************************************************************************
 *  Constants
 ******************************************************************************/ 
 
#define BACKUP_FILE_NAME                        "pre_heating_value.txt"
#define BACKUP_PRE_HEATING_TIME_INTERVAL_IN_SEC  (60*60) // once per hour  
#define PRE_HEATING_RESISTOR_CHECK_INTERVAL     CTRL_LOGIC_TIMELEVEL
#define PRE_HEATING_RELAY_PIN                   17
  
/******************************************************************************
 *  Local variables
 ******************************************************************************/
 
static uint32 g_u32StartTime = 0;
static uint32 g_u32CheckCallCnt = 0;
static uint32 g_u32StopTime = 0;
static uint32 g_u32OnTimeTotal = 0;

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/
 
static void read_on_time_from_file();
static void save_on_time_to_file();

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void pre_heating_resistor_init(void)
{
    relay_control_init(PRE_HEATING_RELAY_PIN);
    read_on_time_from_file();
}

void pre_heating_resistor_start(void)
{
    if (!g_u32StartTime)
    {
        relay_control_set_on(PRE_HEATING_RELAY_PIN);
        g_u32StartTime = time(NULL);
    }
}

void pre_heating_resistor_stop(void)
{
    if (g_u32StartTime)
    {
        relay_control_set_off(PRE_HEATING_RELAY_PIN);
        g_u32OnTimeTotal += u32_pre_heating_resistor_get_on_time();
        g_u32StartTime = 0;
    }
}

void pre_heating_resistor_counter_update(void)
{
    g_u32CheckCallCnt++;
    if ( (g_u32CheckCallCnt * PRE_HEATING_RESISTOR_CHECK_INTERVAL) % 
         BACKUP_PRE_HEATING_TIME_INTERVAL_IN_SEC == 0)
    {
        save_on_time_to_file();
    }
}

uint32 u32_pre_heating_resistor_get_on_time()
{
    if (g_u32StartTime)
    {
        return time(NULL) - g_u32StartTime;
    }
    else
    {
        return 0;
    }
}

bool pre_heating_resistor_get_status()
{
    if (g_u32StartTime)
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32 u32_pre_heating_resistor_get_on_time_total()
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

