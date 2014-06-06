
#ifndef POST_HEATING_COUNTER_H
#define POST_HEATING_COUNTER_H

#include "types.h"

#define POST_HEATING_COUNTER_UPDATE_INTERVAL 5

void post_heating_counter_init();

void post_heating_counter_update();

uint32 post_heating_counter_get_on_time_total();

#endif
