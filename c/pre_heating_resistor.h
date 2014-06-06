
#ifndef PRE_HEATING_RESISTOR_H
#define PRE_HEATING_RESISTOR_H

#include "types.h"

#define PRE_HEATING_RESISTOR_CHECK_INTERVAL 5

void pre_heating_resistor_init(void);

void pre_heating_set_power(uint16 u16Power);

void pre_heating_resistor_thread(void);

void pre_heating_resistor_check();

void pre_heating_resistor_get_status(bool *bPreHeatingOngoing,
                                     uint32 *u32StoppedTimeElapsed);

uint32 pre_heating_resistor_get_on_time_total();

uint16 pre_heating_get_power();

#endif
