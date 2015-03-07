
#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include "common.h"

void relay_control_init(uint16 u16Port);

void relay_control_set_on(uint16 u16Pin);

void relay_control_set_off(uint16 u16Pin);

#endif
