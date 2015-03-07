
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include "common.h"
#include "ctrl_logic.h"
#include "digit_protocol.h"

#include "post_heating_counter.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"

#include "DS18B20.h"
#include "ctrl_variables.h"

#define PRE_HEATING_EXHAUST_TEMP_MAX 3.0f

uint32 g_call_cnt= 0;

typedef struct
{
    bool active;
    uint16 calc_power;
    uint16 calc_power_offset;
    time_t check_interval;
    time_t check_time;
    time_t start_time;
    real32 exhaust_temp_diff_prev;
} T_pre_heating;

typedef struct
{
    E_defrost_state state;
    time_t check_time;
    int32 pre_heating_power;
    real32 in_eff_prev;
} T_defrost;

T_pre_heating g_pre_heating;

T_defrost g_defrost_control;

E_defrost_state defrost_state()
{
    return g_defrost_control.state;
}

void *pvCtrl_logic_thread(void *ptr)
{
    ctrl_logic_init();
    while(1)
    {
        ctrl_logic_run();
        sleep(CTRL_LOGIC_TIMELEVEL);
    }
    return NULL;
}

static int32 pre_heating_calc_power(real32 exhaust_target_temp)
{
    real32 inside_temp = digit_get_inside_temp();
    
    real32 target_incoming_temp = inside_temp - ((inside_temp - exhaust_target_temp) / 0.8f);
    real32 temp_diff = target_incoming_temp - digit_get_outside_temp();
    
    real32 air_flow = 15 + 10 * digit_get_cur_fan_speed();
    real32 pre_heating_power = (air_flow * 1.225 * temp_diff);

    int32 ret = ceil(pre_heating_power / 100.0f) * 100;
    if (ret < 0)
        ret = 0;
    
    return ret;
}

static void pre_heating_control()
{
    if (ctrl_pre_heating_mode() == PRE_HEATING_MODE_ON)
    {
        if (!g_pre_heating.active)
        {
            g_pre_heating.active = true;
            g_pre_heating.check_time = time(NULL);
        }
    }
    else if (ctrl_pre_heating_mode() == PRE_HEATING_MODE_AUTO)
    {
        // get the smallest exhaust temp
        //real32 current_exhaust_temp = min(digit_get_exhaust_temp(), get_DS18B20_exhaust_temp());
        real32 current_exhaust_temp = digit_get_exhaust_temp();
        
        // get the biggest exhaust target temp
        real32 target_exhaust_temp = max(ctrl_dew_point(), ctrl_min_exhaust_temp());
        int32 i32Power = 0;
        
       
        if (target_exhaust_temp > PRE_HEATING_EXHAUST_TEMP_MAX)
        {
            target_exhaust_temp = PRE_HEATING_EXHAUST_TEMP_MAX;
        }

        real32 exhaust_temp_diff = target_exhaust_temp - current_exhaust_temp;
        
        real32 exhaust_temp_derivate = g_pre_heating.exhaust_temp_diff_prev - exhaust_temp_diff;
        
        int32 i32CalcPower = pre_heating_calc_power(target_exhaust_temp);
        
        g_pre_heating.exhaust_temp_diff_prev = exhaust_temp_diff;
        
   
        if (exhaust_temp_diff > 0 && exhaust_temp_derivate < 0)
        {
            i32Power = 1500;
        }
        else if (exhaust_temp_diff > 4.0f)
        {
            i32Power = 1500;
        }
        else if (exhaust_temp_diff > 3.0f)
        {
            i32Power = 1000;
        }
        else if (exhaust_temp_diff > 2.0f)
        {
            i32Power = i32CalcPower * 2;
        }          
        else if (exhaust_temp_diff > 1.0f)
        {
            i32Power = i32CalcPower * 1.5f;
        }       
        else if (exhaust_temp_diff > 0)
        {
            i32Power = i32CalcPower;
        }
        else if (exhaust_temp_diff > -1.0f)
        {
            i32Power = i32CalcPower / 2;
        }
        else if (exhaust_temp_diff > -1.5f)
        {
            i32Power  = i32CalcPower / 3;
        }
        else 
        {
            i32Power  = 0;
        }
        
        if (g_defrost_control.state == e_Defrost_Ongoing)
        {
            pre_heating_set_power(g_defrost_control.pre_heating_power);
        }
        else
        {
            pre_heating_set_power(i32Power);
        }
    }
    else
    {
        pre_heating_set_power(0);
    }
}

void defrost_control()
{    
    if (ctrl_defrost_mode() == DEFROST_MODE_ON)
    {
        defrost_resistor_start();
        g_defrost_control.state = e_Measuring;
    }        
    else if (ctrl_defrost_mode() == DEFROST_MODE_OFF)
    {
        defrost_resistor_stop();
        g_defrost_control.state = e_Measuring;
    }
    else
    {
        real32 in_eff = ctrl_filtered_in_efficiency();
        time_t current_time = time(NULL);
                
        if (g_defrost_control.state == e_Measuring ||
            g_defrost_control.state == e_Below_Limit)
        {  
            if (in_eff < ctrl_defrost_start_level())
            {
                if (g_defrost_control.state == e_Measuring)
                {
                    g_defrost_control.check_time = current_time;
                    g_defrost_control.state = e_Below_Limit;
                }
                /*
                else if (in_eff > g_defrost_control.in_eff_prev)
                {
                     g_defrost_control.state = e_Measuring;
                }
                */
                else if (current_time - g_defrost_control.check_time > 
                           ctrl_defrost_start_duration())
                {
                    g_defrost_control.check_time = current_time;
                    g_defrost_control.state = e_Defrost_Ongoing;
                }
            }
        }
        else if (g_defrost_control.state == e_Defrost_Ongoing)
        {
            if (get_DS18B20_incoming_temp() > ctrl_defrost_target_temp() ||
                current_time - g_defrost_control.check_time > ctrl_defrost_max_duration())
            {
                g_defrost_control.check_time = current_time;
                g_defrost_control.state = e_Defrost_Stopped;
            }
        }
        else if (g_defrost_control.state == e_Defrost_Stopped)
        {
            if (current_time - g_defrost_control.check_time > DEFROST_STOP_TIME)
            {
                g_defrost_control.state = e_Measuring;
            }
        }
        
        if (g_defrost_control.state == e_Defrost_Ongoing)
        {
            defrost_resistor_start();
        }
        else
        {
            defrost_resistor_stop();
        }
        g_defrost_control.in_eff_prev = in_eff;
    }
}


void ctrl_logic_init()
{
    ctrl_init();
    post_heating_counter_init();
    pre_heating_resistor_init();
    defrost_resistor_init();
    
    memset(&g_pre_heating, 0x0, sizeof(g_pre_heating));
    memset(&g_defrost_control, 0x0, sizeof(g_defrost_control));
    g_defrost_control.pre_heating_power = 1000;
}

void ctrl_logic_run()
{
    g_call_cnt++;

    post_heating_counter_update();
    pre_heating_resistor_counter_update();
    defrost_resistor_counter_update();
    
    if (digit_vars_ok() && DS18B20_vars_ok())
    {
        ctrl_update();
        
        if (g_call_cnt % 2)
        {
            pre_heating_control();
            defrost_control();
        }
    }
    else
    {
        ctrl_init();
        pre_heating_set_power(0);
        defrost_resistor_stop();
    }
}
