
#ifndef DEFROST_RESISTOR_H
#define DEFROST_RESISTOR_H

#include "types.h"

#define DEFROST_RESISTOR_CHECK_INTERNAL 5

void defrost_resistor_init(void);

void defrost_resistor_start(void);

void defrost_resistor_stop(void);

void defrost_resistor_check(void);

bool defrost_resistor_get_status();

uint32 defrost_resistor_get_on_time(void);

uint32 defrost_resistor_get_on_time_total();

#endif
