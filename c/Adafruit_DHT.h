#ifndef ADAFRUIT_DHT_H
#define ADAFRUIT_DHT_H

#include <stdbool.h>

void *poll_dht_sendor( void *ptr );

bool get_am2302_values(float *temp, float *rh);

time_t get_am2302_timestamp();


#endif
