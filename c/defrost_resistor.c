/**
 * @file   defrost_resistor.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of defrost resistor. 
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 

#include "common.h"
#include "relay_control.h"
#include "defrost_resistor.h"
#include "ctrl_logic.h"
 
/******************************************************************************
 *  Constants
 ******************************************************************************/
 
#define BACKUP_FILE_NAME                    "defrost_heating_value.txt"
#define BACKUP_DEFROST_TIME_INTERVAL_IN_SEC (60*60) // once per hour
#define DEFROST_RESISTOR_CHECK_INTERNAL     CTRL_LOGIC_TIMELEVEL
#define DEFROST_RELAY_PIN                   22

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

void defrost_resistor_init(void)
{
    relay_control_init(DEFROST_RELAY_PIN);
    read_on_time_from_file();
}

void defrost_resistor_start(void)
{
    if (!g_u32StartTime)
    {
        relay_control_set_on(DEFROST_RELAY_PIN);
        g_u32StartTime = time(NULL);
    }
}

void defrost_resistor_stop(void)
{
    if (g_u32StartTime)
    {
        relay_control_set_off(DEFROST_RELAY_PIN);
        g_u32OnTimeTotal += u32_defrost_resistor_get_on_time();
        g_u32StartTime = 0;
    }
}

void defrost_resistor_counter_update(void)
{
    g_u32CheckCallCnt++;
    if ( (g_u32CheckCallCnt * DEFROST_RESISTOR_CHECK_INTERNAL) % 
         BACKUP_DEFROST_TIME_INTERVAL_IN_SEC == 0)
    {
        save_on_time_to_file();
    }
}

uint32 u32_defrost_resistor_get_on_time()
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

bool defrost_resistor_get_status()
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

uint32 u32_defrost_resistor_get_on_time_total()
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
