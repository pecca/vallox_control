
#ifndef DIGIT_PROTOCOL_H
#define DIGIT_PROTOCOL_H

#include <time.h>
#include "types.h"

#define SYSTEM_ID      0x01

#define DEVICE_ADDRESS 0x11
#define PANEL_ADDRESS  0x21
#define PI_ADDRESS     0x22


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
#define PRE_HEATING_TEMP            0xA7
#define IO_GATE_1                   0x06
#define IO_GATE_2                   0x07
#define IO_GATE_3                   0x08

#define CUR_FAN_SPEED_INDEX               0
#define OUTSIDE_TEMP_INDEX                1
#define EXHAUST_TEMP_INDEX                2
#define INSIDE_TEMP_INDEX                 3
#define INCOMING_TEMP_INDEX               4
#define POST_HEATING_ON_CNT_INDEX         5
#define POST_HEATING_OFF_CNT_INDEX        6
#define INCOMING_TARGET_TEMP_INDEX        7
#define PANEL_LEDS_INDEX                  8
#define MAX_FAN_SPEED_INDEX               9
#define MIN_FAN_SPEED_INDEX               10
#define HRC_BYPASS_TEMP_INDEX             11
#define INPUT_FAN_STOP_TEMP_INDEX         12 
#define CELL_DEFROSTING_HYSTERESIS_INDEX  13
#define DC_FAN_INPUT_INDEX                14
#define DC_FAN_OUTPUT_INDEX               15
#define FLAGS_2_INDEX                     16
#define FLAGS_4_INDEX                     17
#define FLAGS_5_INDEX                     18
#define FLAGS_6_INDEX                     19
#define RH_MAX_INDEX                      20
#define RH1_SENSOR_INDEX                  21
#define BASIC_RH_LEVEL_INDEX              22
#define PRE_HEATING_TEMP_INDEX            23
#define IO_GATE_1_INDEX                   24
#define IO_GATE_2_INDEX                   25
#define IO_GATE_3_INDEX                   26

#define NUM_OF_DIGIT_VARS                 27   // must be updated if a new variable is taken into use   

#define INVALID_VALUE        0xFF

#define DIGIT_RETRANS_CNT       5

#define BIT0   0
#define BIT1   1
#define BIT2   2
#define BIT3   3
#define BIT4   4
#define BIT5   5
#define BIT6   6
#define BIT7   7

#define GET_BIT(value, bit) (bool)(value & (1 << bit))

typedef struct
{
    byte id;
    char *name_str;
    byte value;
    byte expected_value;
    time_t timestamp;
    time_t interval;
    bool get_ongoing;
    bool set_ongoing;
    byte (*StrToValue) (char*);
    void (*ValueToStr) (byte, char*);

} T_digit_var;


void digit_receive_msgs(void);

void digit_update_vars();

void digit_set_var(T_digit_var *var, byte value);

float digit_get_outside_temp();

float digit_get_inside_temp();

float digit_get_exhaust_temp();

float digit_get_incoming_temp();

float digit_get_incoming_target_temp();

void digit_set_incoming_target_temp(float temp);

float digit_get_post_heating_on_cnt(void);

float digit_get_post_heating_off_cnt(void);

void digit_json_encode_vars(char *str);

void digit_set_var_by_name(char *name, char *value);

#endif
