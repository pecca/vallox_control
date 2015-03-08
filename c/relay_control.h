/**
 * @file   relay_control.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of relay control via GPIO registers. 
 */

#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/ 
 
void relay_control_init(uint16 u16Port);

void relay_control_set_on(uint16 u16Pin);

void relay_control_set_off(uint16 u16Pin);

#endif
