/**
 * @file   DS18B20.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of DS18B20 sensors. 
 */

#ifndef DS18B20_H
#define DS18B20_H

/******************************************************************************
 *  Global function declaration
 ******************************************************************************/

void *DS18B20_thread(void *ptr);
real32 r32_DS18B20_outside_temp(void);
real32 r32_DS18B20_exhaust_temp(void);
real32 r32_DS18B20_incoming_temp(void);
time_t tDS18B20_outside_temp_ts(void);
time_t tDS18B20_exhaust_temp_ts(void);
time_t tDS18B20_incoming_temp_ts(void);
void DS18B20_json_encode_vars(char *mesg);
bool DS18B20_vars_ok();

#endif
