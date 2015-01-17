
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include "types.h"
#include "ctrl_logic.h"
#include "digit_protocol.h"

#include "post_heating_counter.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"

#include "DS18B20.h"
#include "ctrl_variables.h"

#define PRE_HEATING_EXHAUST_TEMP_MAX 3.0f

typedef struct
{
    bool active;
    uint16 calc_power;
    uint16 calc_power_offset;
    time_t check_interval;
    time_t check_time;
    time_t start_time;
} T_pre_heating;

T_pre_heating g_pre_heating;


int32 pre_heating_calc_power(real32 exhaust_target_temp)
{
    real32 inside_temp = digit_get_inside_temp();
    
    real32 target_incoming_temp = inside_temp - ((inside_temp - exhaust_target_temp) / 0.8f);
    real32 temp_diff = target_incoming_temp - digit_get_outside_temp();
    
    float air_flow = 15 + 10 * digit_get_cur_fan_speed();
    float pre_heating_power = (air_flow * 1.225 * temp_diff);

    int32 ret = round(pre_heating_power / 100.0f) * 100;
    if (ret < 0)
        ret = 0;
    
    return ret;
}

void pre_heating_control()
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
        real32 current_exhaust_temp = min(digit_get_exhaust_temp(), get_DS18B20_exhaust_temp());
        // get the biggest exhaust target temp
        real32 target_exhaust_temp = max(ctrl_dew_point(), ctrl_min_exhaust_temp());
                
        if (target_exhaust_temp > PRE_HEATING_EXHAUST_TEMP_MAX)
        {
            target_exhaust_temp = PRE_HEATING_EXHAUST_TEMP_MAX;
        }

        real32 exhaust_temp_diff = target_exhaust_temp - current_exhaust_temp;
        
        int32 power = pre_heating_calc_power(target_exhaust_temp);
        
        //printf("curr_exhaust_temp %.1f, target_exhaust_temp %.1f, power_delta %d\n",  current_exhaust_temp, target_exhaust_temp, power);
   
   
   
        if (exhaust_temp_diff > 2)
        {
            pre_heating_set_power(power * 2);
        }
        else if (exhaust_temp_diff < -2)
        {
            pre_heating_set_power(power / 3);
        }
        else if (exhaust_temp_diff < -1)
        {
             pre_heating_set_power(power / 4);
        }
        else
        {
             pre_heating_set_power(power);
        }
        
        
  
        
        /*
        if (!g_pre_heating.active)
        {
            if (exhaust_temp_diff > 0)
            {
                // start pre-heating
                g_pre_heating.active = true;
                g_pre_heating.calc_power = 0;
                g_pre_heating.check_time = time(NULL) + 60;
                pre_heating_set_power(1500);
            }
        }
        else
        {
            if  (time(NULL) > g_pre_heating.check_time)
            {              
                if (g_pre_heating.calc_power + power_delta < 0)
                {
                    g_pre_heating.calc_power = 0;
                    g_pre_heating.active = false;
                }
                else if (g_pre_heating.calc_power + power_delta > 1500)
                {
                    g_pre_heating.calc_power = 1500;
                }
                else
                {
                    g_pre_heating.calc_power += power_delta;
                }
            
                pre_heating_set_power(g_pre_heating.calc_power);
                
                g_pre_heating.check_time = time(NULL) + 60;
            }
        }
        */
    }
   /* 
		if (g_call_cnt % 12 == 0)
		{
			float exhaust = min(exhaust_temp, exhaust_temp_ds);
			float target_exhaust_temp = exhaust;
			int fan_speed = digit_get_cur_fan_speed();
		
			if (target_exhaust_temp < g_dew_point ||
			    exhaust > g_dew_point)
			{
				target_exhaust_temp = g_dew_point;
			}			
			
			if (target_exhaust_temp < g_min_exhaust_temp)
			{
				target_exhaust_temp = g_min_exhaust_temp;
			}
			if (target_exhaust_temp > 6.0f)
			{
				target_exhaust_temp = 6.0f;
			}
		
			printf("target exhaust_temp = %f, current exhaust_temp = %f\n", target_exhaust_temp, exhaust);
		
			float air_flow = 15 + 10 * fan_speed;
			float pre_heating_power = (air_flow * 1.225 * (target_exhaust_temp - exhaust)) / 0.6f;
		
			int ipre_heating_power = ceil(pre_heating_power / 100.0f) * 100;
			
			printf("pre_heating_power delta = %d\n", ipre_heating_power);		
			pre_heating_set_power( pre_heating_get_power() + ipre_heating_power);
			printf("new pre heating power = %d\n", pre_heating_get_power());
			
		}
    }
    */
    else
    {
        pre_heating_set_power(0);
    }

}



void ctrl_logic_init()
{
    ctrl_init();
    post_heating_counter_init();
    pre_heating_resistor_init();
    defrost_resistor_init();
}

uint32 g_call_cnt= 0;

void ctrl_logic_run()
{
    g_call_cnt++;

    post_heating_counter_update();
    pre_heating_resistor_counter_update();
    defrost_resistor_counter_update();

    ctrl_update();
    
    if (digit_vars_ok())
    {
        pre_heating_control();

        if (ctrl_defrost_mode() == DEFROST_MODE_ON)
        {
            defrost_resistor_start();
        }
        else
        {
            defrost_resistor_stop();
        }
    }
    else
    {
        pre_heating_set_power(0);
        defrost_resistor_stop();
    }
}
