
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

void ctrl_variables_set(byte id, char *str);

void ctrl_variables_get(byte id, char *str);

void ctrl_process_set_var(char *name, char *value);

void ctrl_json_encode_vars(char *str);

#endif
