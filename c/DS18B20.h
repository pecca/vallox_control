#ifndef DS18B20_H
#define DS18B20_H

void *poll_DS18B20_sendors( void *ptr );

float get_DS18B20_outside_temp();

float get_DS18B20_exhaust_temp();

float get_DS18B20_incoming_temp();

time_t get_DS18B20_outside_temp_ts();

time_t get_DS18B20_exhaust_temp_ts();

time_t get_DS18B20_incoming_temp_ts();

void ds18b20_json_encode_vars(char *mesg);

#endif
