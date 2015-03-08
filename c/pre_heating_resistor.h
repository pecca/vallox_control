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
void pre_heating_set_power(uint16 u16Power);
void pre_heating_resistor_thread(void);
void pre_heating_resistor_counter_update(void);
void pre_heating_resistor_get_status(bool *bPreHeatingOngoing,
                                     uint32 *u32StoppedTimeElapsed);
uint32 u32_pre_heating_resistor_get_on_time_total(void);
uint16 u16_pre_heating_get_power(void);
void *pre_heating_thread(void *ptr);

#endif
