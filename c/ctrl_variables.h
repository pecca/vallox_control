
#ifndef CTRL_VARIABLES_H
#define CTRL_VARIABLES_H

#include "types.h"

#define CTRL_VAR_ID_PRE_HEATING_POWER          1
#define CTRL_VAR_ID_PRE_HEATING_MODE           2
#define CTRL_VAR_ID_PRE_HEATING_ON_TIME_TOTAL  3
#define CTRL_VAR_ID_POST_HEATING_ON_TIME_TOTAL 4
#define CTRL_VAR_ID_DEFROST_MODE               5
#define CTRL_VAR_ID_DEFROST_ON_TIME            6
#define CTRL_VAR_ID_DEFROST_ON_TIME_TOTAL      7

#define DEFROST_MODE_OFF    0
#define DEFROST_MODE_ON     1
#define DEFROST_MODE_AUTO   2

#define PRE_HEATING_MODE_OFF   0
#define PRE_HEATING_MODE_ON    1
#define PRE_HEATING_MODE_AUTO  2

typedef struct
{
    real32 min_exhaust_temp;
    real32 dew_point;
    real32 in_efficiency;
    real32 out_efficiency;
    byte defrost_mode;
    byte pre_heating_mode;
} T_ctrl_vars;

extern T_ctrl_vars g_ctrl_vars;

void ctrl_init();

void ctrl_update();

void ctrl_set_var_by_name(char *name, char *value);

void ctrl_json_encode(char *str);

byte ctrl_defrost_mode();

byte ctrl_pre_heating_mode();

uint16 ctrl_pre_heating_power();

void ctrl_set_pre_heating_power(uint16 pre_heating_power);

real32 ctrl_dew_point();

real32 ctrl_min_exhaust_temp();



#endif
