/**
 * @file   pre_heating_resistor.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of pre-heating resistor. 
 */

#ifndef PRE_HEATING_RESISTOR_H
#define PRE_HEATING_RESISTOR_H

/******************************************************************************
 *  Global function declarations
 ******************************************************************************/ 
 
void pre_heating_resistor_init(void);
void pre_heating_resistor_start(void);
void pre_heating_resistor_stop(void);

void pre_heating_resistor_counter_update(void);
bool pre_heating_resistor_get_status(void);
uint32 u32_pre_heating_resistor_get_on_time(void);
uint32 u32_pre_heating_resistor_get_on_time_total(void);

#endif
