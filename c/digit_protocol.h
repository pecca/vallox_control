
#ifndef DIGIT_PROTOCOL_H
#define DIGIT_PROTOCOL_H

#include "types.h"

#define SYSTEM_ID      0x01

#define DEVICE_ADDRESS 0x11
#define PANEL_ADDRESS  0x21
#define PI_ADDRESS     0x22

#define NUM_OF_DIGIT_VARS 24 // must be increased if a new variabe is taken into use   

#define CUR_FAN_SPEED               0x29
#define OUTSIDE_TEMP                0x32
#define EXHAUST_TEMP                0x33
#define INSIDE_TEMP                 0x34
#define INCOMING_TEMP               0x35
#define POST_HEATING_ON_CNT         0x55
#define POST_HEATING_OFF_CNT        0x56
#define INCOMING_TARGET_TEMP        0xA4
#define PANEL_LEDS                  0xA3
#define MAX_FAN_SPEED               0xA5
#define MIN_FAN_SPEED               0xA9
#define HRC_BYPASS_TEMP             0xAF
#define INPUT_FAN_STOP_TEMP         0xA8 
#define CELL_DEFROSTING_HYSTERESIS  0xB2
#define DC_FAN_INPUT                0xB0
#define DC_FAN_OUTPUT               0xB1
#define FLAGS_2                     0x6D
#define FLAGS_4                     0x6F
#define FLAGS_5                     0x70
#define FLAGS_6                     0x71
#define RH_MAX                      0x2A
#define RH1_SENSOR                  0x2F
#define BASIC_RH_LEVEL              0xAE

#define INVALID_VALUE        0xFF

#define DS18B20_SENSOR1      0xD1
#define DS18B20_SENSOR2      0xD2
#define AM2302_TEMP_SENSOR   0xD3
#define AM2302_HR_SENSOR     0xD4

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
    byte (*StrToValue) (char*);
    void (*ValueToStr) (byte, char*);

} T_digit_var;

void digit_receive_msgs(void);

void digit_update_vars();

void convert_digit_var_value_to_str(byte id, char *str);

void digit_send_set_request(byte id, byte new_value);

#endif
