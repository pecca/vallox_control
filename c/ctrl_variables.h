
#ifndef CTRL_VARIABLES_H
#define CTRL_VARIABLES_H

#include "common.h"
#include "ctrl_logic.h"

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

#define MOVING_AVERAGE_SIZE (60 * 60 / CTRL_LOGIC_TIMELEVEL)
#define FILTER_TIME (30 * 5)

#define DEFROST_MAX_DURATION     (15)
#define DEFROST_START_DURATION   (10)
#define DEFROST_TARGET_LEVEL     (72)
#define DEFROST_TARGET_TEMP      (17)

#define DEFROST_STOP_TIME        (10 * 60)

typedef struct
{
    real32 table[MOVING_AVERAGE_SIZE];
    real64 sum;
    real32 value;
} T_AvfFilter;

typedef struct
{
    uint32 call_cnt;
    real32 min_exhaust_temp;
    real32 dew_point;
    real32 in_efficiency;
    real32 out_efficiency;
    T_AvfFilter in_eff;
    T_AvfFilter out_eff;
    byte pre_heating_mode;    
    byte defrost_mode;
    uint16 defrost_max_duration;
    uint16 defrost_start_duration;
    real32 defrost_start_level;
    real32 defrost_target_temp;
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

real32 ctrl_filtered_in_efficiency();

real32 ctrl_in_efficiency();

uint16 ctrl_defrost_max_duration();

uint16 ctrl_defrost_start_duration();

real32 ctrl_defrost_start_level();

real32 ctrl_defrost_target_temp();


#endif
