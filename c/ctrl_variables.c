
#include <stdio.h>
#include "ctrl_variables.h"
#include "ctrl_logic.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"
#include "post_heating_counter.h"
#include "json_codecs.h"
#include <string.h>

uint16 g_u16Ctrl_pre_heating_power;

uint16 g_u16Ctrl_pre_heating_mode;

uint16 g_u16Ctrl_defrost_mode;

uint16 g_u16Ctrl_defrost_duration;

void ctrl_process_set_var(char *name, char *value)
{
	int u16Temp;
	
    if (!strcmp(name, "pre_heating_power"))
    {
        sscanf(value, "%d", &u16Temp);
        pre_heating_power_set(u16Temp);
    }
	else if (!strcmp(name, "pre_heating_mode"))
    {
        sscanf(value, "%d", &u16Temp);
        pre_heating_mode_set(u16Temp);
    }
    else if (!strcmp(name, "defrost_mode"))
    {
        sscanf(value, "%d", &u16Temp);
        defrost_mode_set(u16Temp);
    }
}

void ctrl_variables_set(byte id, char *str)
{
    int u16Temp;

    if (id == CTRL_VAR_ID_PRE_HEATING_POWER)
    {
        sscanf(str, "%d", &u16Temp);
        pre_heating_power_set(u16Temp);
    }
    else if (id == CTRL_VAR_ID_PRE_HEATING_MODE)
    {
        sscanf(str, "%d", &u16Temp);
        pre_heating_mode_set(u16Temp);
    }
    else if (id == CTRL_VAR_ID_DEFROST_MODE)
    {
        sscanf(str, "%d", &u16Temp);
        defrost_mode_set(u16Temp);
    }
}

void ctrl_variables_get(byte id, char *str)
{
    if (id == CTRL_VAR_ID_PRE_HEATING_ON_TIME_TOTAL)
    {
        sprintf(str, "%d", pre_heating_resistor_get_on_time_total());
    }
    else if (id == CTRL_VAR_ID_POST_HEATING_ON_TIME_TOTAL)
    {
        sprintf(str, "%d", post_heating_counter_get_on_time_total());
    }
    else if (id == CTRL_VAR_ID_DEFROST_ON_TIME_TOTAL)
    {
        sprintf(str, "%d", defrost_resistor_get_on_time_total());
    }
    else if (id == CTRL_VAR_ID_DEFROST_MODE)
    {
        sprintf(str, "%d", defrost_mode_get());
    } 
    else if (id == CTRL_VAR_ID_PRE_HEATING_MODE)
    {
        sprintf(str, "%d", pre_heating_mode_get());
    }
    else if (id == CTRL_VAR_ID_PRE_HEATING_POWER)
    {
        sprintf(str, "%d", pre_heating_power_get());
    }
    else if (id == CTRL_VAR_ID_DEFROST_ON_TIME)
    {
        sprintf(str, "%d", defrost_resistor_get_on_time());
    }
}

void ctrl_json_encode_vars(char *str)
{
	char sub_str[1000];
	
	strcpy(sub_str, "");
	strcpy(str, "{");

	json_encode_integer(sub_str,
						"pre_heating_on_time",
						pre_heating_resistor_get_on_time_total());
	strncat(sub_str, ",", 1);
	
	
	json_encode_integer(sub_str,
						"post_heating_on_time",
						post_heating_counter_get_on_time_total());
	strncat(sub_str, ",", 1);
	
	json_encode_integer(sub_str,
						"defrost_on_time",
						defrost_resistor_get_on_time_total());
	strncat(sub_str, ",", 1);
	
	json_encode_integer(sub_str,
						"defrost_mode",
						defrost_mode_get());
	strncat(sub_str, ",", 1);
	
	json_encode_integer(sub_str,
						"pre_heating_mode",
						pre_heating_mode_get());
	strncat(sub_str, ",", 1);
	
	json_encode_integer(sub_str,
						"pre_heating_power",
						pre_heating_power_get());
	
	json_encode_object(str,
					   CONTROL_VARS,
					   sub_str);
	strncat(str, "}", 1);
}
