/**
 * @file   digit_protocol.h
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Interface of Vallox digit protocol
 */

#ifndef DIGIT_PROTOCOL_H
#define DIGIT_PROTOCOL_H

/******************************************************************************
 *  Global function declarations
 ******************************************************************************/ 
 
void *digit_receive_thread(void *ptr);
void *digit_update_thread(void *ptr);
bool digit_vars_ok(void);
void digit_json_encode_vars(char *str);
void digit_set_var_by_name(char *name, char *value);

real32 r32_digit_rh1_sensor();
real32 r32_digit_outside_temp();
real32 r32_digit_inside_temp();
real32 r32_digit_exhaust_temp();
real32 r32_digit_incoming_temp();
uint8 u8_digit_cur_fan_speed(void);
real32 r32_digit_incoming_target_temp();
real32 r32_digit_post_heating_on_cnt(void);
real32 r32_digit_post_heating_off_cnt(void);

void digit_set_incoming_target_temp(real32 r32Temp);

#endif
