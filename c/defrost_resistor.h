/**
 * @file   defrost_resistor.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of defrost resistor. 
 */

#ifndef DEFROST_RESISTOR_H
#define DEFROST_RESISTOR_H

/******************************************************************************
 *  Global function declaration
 ******************************************************************************/ 
 
void defrost_resistor_init(void);
void defrost_resistor_start(void);
void defrost_resistor_stop(void);
void defrost_resistor_counter_update(void);
bool defrost_resistor_get_status(void);
uint32 u32_defrost_resistor_get_on_time(void);
uint32 u32_defrost_resistor_get_on_time_total(void);

#endif
