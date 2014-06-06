
#ifndef CTRL_LOGIC_H
#define CTRL_LOGIC_H

#define CTRL_LOGIC_TIMELEVEL   5 // sec

#define POST_HEATING_ON_SEC     1
#define PRE_HEATING_ON_SEC      2 
#define PRE_HEATING_ON_TIME     3
#define PRE_HEATING_OFF_TIME    4
#define PRE_HEATING_START_EFF   5
#define PRE_HEATING_STOP_TEMP   6
#define PRE_HEATING_ONGOING     7

void ctrl_logic_init();

void ctrl_logic_run();

void ctrl_set_var(byte id, char *value);

void pre_heating_mode_set(uint16 u16PreHeatingMode);

uint16 pre_heating_mode_get();

void pre_heating_power_set(uint16 u16PreHeatingPower);

uint16 pre_heating_power_get();

void defrost_mode_set(uint16 u16DefrostMode);

uint16 defrost_mode_get();

#endif
