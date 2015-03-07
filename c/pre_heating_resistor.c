#include "pre_heating_resistor.h"
#include "relay_control.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>

#define BACKUP_FILE_NAME "pre_heating_value.txt"

#define CHECK_CALL_CNT_INTERVAL_IN_SEC 5 //

#define BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC (60*60) // once per hour  

const uint16 c_u16PreHeatingRelayPin = 17;

static uint32 g_u32StoppedTime = 0;

static uint32 g_u32OnTimeTotal = 0;

static uint32 g_u32CheckCallCnt = 0;

static uint16 g_u16PreHeatingActivePowerIndex = 0;

static bool g_bInitDone = false;

typedef struct
{
    uint16 u16OnTime;
    uint16 u16OffTime;
} T_pre_heating_power;

T_pre_heating_power g_atpre_heating_power[] = 
{ 
  {0,  5},  // 0 W
  {5, 70},  // 100 W
  {6, 39},  // 200 W
  {9, 36},  // 300 W
  {12,33},  // 400 W
  {15,30},  // 500 W
  {12,18},  // 600 W
  {14,16},  // 700 W
  {16,14},  // 800 W
  {18,12},  // 900 W
  {30,15},  // 1000 W
  {33,12},  // 1100 W
  {36, 9},  // 1200 W
  {39, 6},  // 1300 W
  {70, 5},  // 1400 W
  {5,  0}   // 1500 W
};

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

void pre_heating_resistor_init(void)
{
    relay_control_init(c_u16PreHeatingRelayPin);

    read_on_time_from_file();

    g_bInitDone = true;
}

void pre_heating_set_power(uint16 u16Power)
{
    uint16 u16PowerIndex;

    if (u16Power > 1500)
    {
        u16Power = 1500;
    }
    u16PowerIndex = u16Power / 100;

    if (g_u16PreHeatingActivePowerIndex > 0 &&
        u16PowerIndex == 0)
    {
        g_u32StoppedTime = time(NULL);
    }

    g_u16PreHeatingActivePowerIndex = u16PowerIndex;
}

void *pvPre_heating_thread(void *ptr)
{
    pre_heating_resistor_thread();
    return NULL;
}

void pre_heating_resistor_thread(void)
{
    bool bResistorOn = false;

    while(1)
    {
        if (g_bInitDone == false) 
        {
            sleep(1);
            continue; 
        }

        T_pre_heating_power *power = &g_atpre_heating_power[g_u16PreHeatingActivePowerIndex];
        if (power->u16OnTime > 0)
        {
            if (bResistorOn == false)
            {
                relay_control_set_on(c_u16PreHeatingRelayPin);
                bResistorOn = true;
            }
            sleep(power->u16OnTime);
            g_u32OnTimeTotal += power->u16OnTime;
        }
        
        if (power->u16OffTime > 0)
        {
            if (bResistorOn == true)
            {
                relay_control_set_off(c_u16PreHeatingRelayPin);
                bResistorOn = false;
            }
            sleep(power->u16OffTime);
        }

    }
}

void pre_heating_resistor_counter_update()
{
    g_u32CheckCallCnt++;
    if ( (g_u32CheckCallCnt * PRE_HEATING_RESISTOR_CHECK_INTERVAL) % 
         BACKUP_ON_TIME_COUNTER_INTERVAL_IN_SEC == 0)
    {
        save_on_time_to_file();
    }
}

void pre_heating_resistor_get_status(bool *bPreHeatingOngoing,
                                     uint32 *u32StoppedTimeElapsed)
{
    if (g_u16PreHeatingActivePowerIndex > 0)
    {
        *bPreHeatingOngoing = true;
        *u32StoppedTimeElapsed = 0;
    }
    else
    {
        *bPreHeatingOngoing = false;
        *u32StoppedTimeElapsed = time(NULL) - g_u32StoppedTime;
    }
}


uint32 pre_heating_resistor_get_on_time_total()
{
    return g_u32OnTimeTotal;
}

uint16 pre_heating_get_power()
{
    return (g_u16PreHeatingActivePowerIndex * 100);
}
