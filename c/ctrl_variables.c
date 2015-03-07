
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "digit_protocol.h"
#include "ctrl_variables.h"
#include "ctrl_logic.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"
#include "post_heating_counter.h"
#include "json_codecs.h"
#include "DS18B20.h"

T_ctrl_vars g_ctrl_vars;

void calc_in_out_effiency(float *in_eff, float *out_eff)
{
    float incoming_temp = get_DS18B20_incoming_temp();
    float outside_temp = get_DS18B20_outside_temp();
    float inside_temp = digit_get_inside_temp();
    float exhaust_temp = digit_get_exhaust_temp();

    float incoming_eff =  ((incoming_temp - outside_temp) /
                           (inside_temp - outside_temp)) * 100.0f;

    float outcoming_eff = ((inside_temp - exhaust_temp) /
                           (inside_temp - outside_temp)) * 100.0f;   

    if (incoming_eff > 100.0f)
    {
        incoming_eff = 100.0f;
    }
    else if (incoming_eff < 0.0f)
    {
        incoming_eff = 0.0f;
    }

    if (outcoming_eff > 100.0f)
    {
        outcoming_eff = 100.0f;
    }
    else if (outcoming_eff < 0.0f)
    {
        outcoming_eff = 0.0f;
    }
    *in_eff = incoming_eff;
    *out_eff = outcoming_eff;
}

float calc_dew_point(void)
{
    float inside_temp = digit_get_inside_temp();
    float rh = digit_get_rh1_sensor() / 100.0f;
    float tempA = 17.27f;
    float tempB = 237.7f;
    float tempZ =  (((tempA * inside_temp) / (tempB + inside_temp)) + log(rh));
    float dew_point = ((tempB * tempZ) / (tempA - tempZ));
    return dew_point;
}

void avf_filter_init(T_AvfFilter *filter, real32 value)
{
    filter->sum = 0;
    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++)
    {
        filter->table[i] = value;
        filter->sum += value;
    }
    filter->value = value;
}

void avf_filter_calc(T_AvfFilter *filter, real32 new_value, uint32 index)
{
    real32 old_value = filter->table[index % MOVING_AVERAGE_SIZE];
    filter->sum -= old_value;
    filter->table[index % MOVING_AVERAGE_SIZE] = new_value;
    filter->sum += new_value;
    filter->value = filter->sum / MOVING_AVERAGE_SIZE;
}

void ctrl_init()
{
    memset(&g_ctrl_vars, 0x0, sizeof(g_ctrl_vars));
    g_ctrl_vars.min_exhaust_temp = -3.0f;
    g_ctrl_vars.defrost_mode = DEFROST_MODE_OFF;
    g_ctrl_vars.pre_heating_mode = PRE_HEATING_MODE_OFF;
    pre_heating_get_power(0);
    
    g_ctrl_vars.defrost_max_duration = DEFROST_MAX_DURATION;
    g_ctrl_vars.defrost_start_duration = DEFROST_START_DURATION;
    g_ctrl_vars.defrost_start_level = DEFROST_TARGET_LEVEL;
    g_ctrl_vars.defrost_target_temp = DEFROST_TARGET_TEMP;
}

void ctrl_update()
{
    g_ctrl_vars.dew_point = calc_dew_point();
    calc_in_out_effiency(&g_ctrl_vars.in_efficiency, &g_ctrl_vars.out_efficiency);
    if (!g_ctrl_vars.call_cnt)
    {
        avf_filter_init(&g_ctrl_vars.in_eff, g_ctrl_vars.in_efficiency);
        avf_filter_init(&g_ctrl_vars.out_eff, g_ctrl_vars.out_efficiency);   
    }
    else
    {
        real32 in_eff = g_ctrl_vars.in_efficiency;
        real32 out_eff = g_ctrl_vars.out_efficiency;        
        
        if (defrost_state() == e_Defrost_Ongoing)
        {
            in_eff = g_ctrl_vars.in_eff.value;
            out_eff = g_ctrl_vars.out_eff.value;
        }
    
        avf_filter_calc(&g_ctrl_vars.in_eff, in_eff, g_ctrl_vars.call_cnt);
        avf_filter_calc(&g_ctrl_vars.out_eff, out_eff, g_ctrl_vars.call_cnt);
    }
    g_ctrl_vars.call_cnt++;

}

byte ctrl_defrost_mode()
{
    return g_ctrl_vars.defrost_mode;
}

byte ctrl_pre_heating_mode()
{
    return g_ctrl_vars.pre_heating_mode;
}

real32 ctrl_dew_point()
{
    return g_ctrl_vars.dew_point;
}

real32 ctrl_min_exhaust_temp()
{
    return g_ctrl_vars.min_exhaust_temp;
}

real32 ctrl_filtered_in_efficiency()
{
    return g_ctrl_vars.in_eff.value;
}

real32 ctrl_in_efficiency()
{
    return g_ctrl_vars.in_efficiency;
}

uint16 ctrl_defrost_max_duration()
{
    return g_ctrl_vars.defrost_max_duration * 60;
}

uint16 ctrl_defrost_start_duration()
{
    return g_ctrl_vars.defrost_start_duration * 60;
}

real32 ctrl_defrost_start_level()
{
    return g_ctrl_vars.defrost_start_level;
}

real32 ctrl_defrost_target_temp()
{
    return g_ctrl_vars.defrost_target_temp;
}

void ctrl_set_var_by_name(char *name, char *value)
{
    if (!strcmp(name, "pre_heating_power"))
    {
        int u16Temp;
        sscanf(value, "%d", &u16Temp);
        pre_heating_set_power(u16Temp);
    }
    else if (!strcmp(name, "pre_heating_mode"))
    {
        int u16Temp;
        sscanf(value, "%d", &u16Temp);
        g_ctrl_vars.pre_heating_mode = u16Temp;
    }
    else if (!strcmp(name, "defrost_mode"))
    {
        int u16Temp;
        sscanf(value, "%d", &u16Temp);
        g_ctrl_vars.defrost_mode = u16Temp;
    }
    else if (!strcmp(name, "min_exhaust_temp"))
    {
        float fTemp;
        sscanf(value, "%f", &fTemp);
        g_ctrl_vars.min_exhaust_temp = fTemp;
    }
    else if (!strcmp(name, "defrost_start_level"))
    {
        float fTemp;
        sscanf(value, "%f", &fTemp);
        g_ctrl_vars.defrost_start_level = fTemp;
    }
    else if (!strcmp(name, "defrost_target_temp"))
    {
        float fTemp;
        sscanf(value, "%f", &fTemp);
        g_ctrl_vars.defrost_target_temp = fTemp;
    }  
    else if (!strcmp(name, "defrost_max_duration"))
    {
        int u16Temp;
        sscanf(value, "%d", &u16Temp);
        g_ctrl_vars.defrost_max_duration = u16Temp;
    }
    else if (!strcmp(name, "defrost_start_duration"))
    {
        int u16Temp;
        sscanf(value, "%d", &u16Temp);
        g_ctrl_vars.defrost_start_duration = u16Temp;
    }     
}


void ctrl_json_encode(char *mesg)
{
    char sub_str1[1000];
    char sub_str2[1000];
    
    strcpy(mesg, "{");
    strcpy(sub_str1, "");
    strcpy(sub_str2, "");
    
    // pre_heating_time
    json_encode_float(sub_str2,
                      "value",
                      pre_heating_resistor_get_on_time_total());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "pre_heating_time",
                       sub_str2);
    strncat(sub_str1, ",", 1); 

    // post_heating_time
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      post_heating_counter_get_on_time_total());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "post_heating_time",
                       sub_str2);
    strncat(sub_str1, ",", 1);  
    
    // defrost_time total
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      defrost_resistor_get_on_time_total());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_time",
                       sub_str2);
    strncat(sub_str1, ",", 1);      

    // defrost_time
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      defrost_resistor_get_on_time());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_on_time",
                       sub_str2);
    strncat(sub_str1, ",", 1);  
    
    // defrost_mode
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.defrost_mode);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_mode",
                       sub_str2);
    strncat(sub_str1, ",", 1); 
   
    // pre_heating_mode
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.pre_heating_mode);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "pre_heating_mode",
                       sub_str2);
    strncat(sub_str1, ",", 1);   
  
     // pre_heating_power
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      pre_heating_get_power());
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "pre_heating_power",
                       sub_str2);
    strncat(sub_str1, ",", 1);   
  
     // dew_point
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.dew_point);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "dew_point",
                       sub_str2);
    strncat(sub_str1, ",", 1);   
 
     // dew_point
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.min_exhaust_temp);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "min_exhaust_temp",
                       sub_str2);
    strncat(sub_str1, ",", 1); 
 
     // in_efficiency
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.in_efficiency);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "in_efficiency",
                       sub_str2);
    strncat(sub_str1, ",", 1);   
  
     // out_efficiency
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.out_efficiency);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "out_efficiency",
                       sub_str2);
    strncat(sub_str1, ",", 1);                   

     // in_efficiency filtered
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.in_eff.value);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "in_efficiency_filtered",
                       sub_str2);
    strncat(sub_str1, ",", 1);   
  
     // out_efficiency filtered
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.out_eff.value);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "out_efficiency_filtered",
                       sub_str2);                       
    strncat(sub_str1, ",", 1);                   

     // defrost_start_level
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.defrost_start_level);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_start_level",
                       sub_str2);
    strncat(sub_str1, ",", 1);  

     // defrost_target_temp
    strcpy(sub_str2, "");
    json_encode_float(sub_str2,
                      "value",
                      g_ctrl_vars.defrost_target_temp);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_target_temp",
                       sub_str2);
    strncat(sub_str1, ",", 1); 
    
    // defrost_max_duration
    strcpy(sub_str2, "");
    json_encode_integer(sub_str2,
                      "value",
                      g_ctrl_vars.defrost_max_duration);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_max_duration",
                       sub_str2);
    strncat(sub_str1, ",", 1); 
  
    // defrost_start_duration
    strcpy(sub_str2, "");
    json_encode_integer(sub_str2,
                      "value",
                      g_ctrl_vars.defrost_start_duration);
    strncat(sub_str2, ",", 1);
    json_encode_integer(sub_str2,
                        "ts",
                        time(NULL));
    json_encode_object(sub_str1,
                       "defrost_start_duration",
                       sub_str2);
     
    json_encode_object(mesg,
                       CONTROL_VARS,
                       sub_str1);
    strncat(mesg, "}", 1);
}