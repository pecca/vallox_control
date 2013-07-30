
#ifndef DIGIT_PROTOCOL_H
#define DIGIT_PROTOCOL_H

#include "types.h"

#define SYSTEM_ID      0x01

#define DEVICE_ADDRESS 0x11
#define PANEL_ADDRESS  0x21
#define PI_ADDRESS     0x22

#define NUM_OF_DIGIT_VARS 2 // 16 // must be increased if new variables are used   

#define CUR_FAN_SPEED        0x29
#define OUTDOOR_TEMP         0x32
#define WASTE_AIR_TEMP       0x33
#define OUT_TEMP             0x34
#define INDOOR_TEMP          0x35
#define POST_HEATING_ON_CNT  0x55
#define POST_HEATING_OFF_CNT 0x56
#define POST_HEATING_TARGET  0xA4
#define PANEL_LEDS           0xA3
#define MAX_FAN_SPEED        0xA5
#define MIN_FAN_SPEED        0xA9
#define SUMMER_MODE_TEMP     0xAF
#define ANTIFREEZE_TEMP      0xA8 
#define ANTIFREEZE_HYSTERIS  0xB2
#define IN_FAN_VALUE         0xB0
#define OUT_FAN_VALUE        0xB1

typedef struct
{
	byte power_key        : 1;
	byte co2_key          : 1;
	byte rh_key           : 1;
	byte post_heating_key : 1;
	byte filter_led       : 1;
	byte post_heating_led : 1;
	byte fault_led        : 1;
	byte service_reminder : 1;
} T_panel_leds;

typedef struct
{
	byte id;
	byte value;
	byte default_value;
	time_t timestamp;
	time_t interval;
	bool req_ongoing;
} T_digit_var;

void digit_receive_msgs(int msg_cnt);

void digit_update_vars();

#endif
