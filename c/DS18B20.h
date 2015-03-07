#ifndef DS18B20_H
#define DS18B20_H

void *pvDS18B20_thread(void *ptr);

real32 get_DS18B20_outside_temp();

real32 get_DS18B20_exhaust_temp();

real32 get_DS18B20_incoming_temp();

time_t get_DS18B20_outside_temp_ts();

time_t get_DS18B20_exhaust_temp_ts();

time_t get_DS18B20_incoming_temp_ts();

void ds18b20_json_encode_vars(char *mesg);

bool DS18B20_vars_ok();

#endif
