
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "digit_protocol.h"
#include "rs485.h"
#include "temperature_conversion.h"
#include "json_codecs.h"

uint32 g_digit_set_var_failed_cnt;
uint32 g_digit_retrans_cnt;

byte StrToValue_Temperature(char *str)
{
    float temp;
    sscanf(str, "%f", &temp); 
    return celsius_to_NTC(temp);
}

void ValueToStr_Temperature(byte value, char *str)
{
    sprintf(str, "%.1f", NTC_to_celsius(value));
}

byte StrToValue_FanSpeed(char *str)
{
    byte ret = 0;
    int fan_speed;
    sscanf(str, "%d", &fan_speed);
    for (int i = 0; i < fan_speed; i++)
    {
       ret |= (0x1 << i); 
    }
    return ret;
}

void ValueToStr_FanSpeed(byte value, char *str)
{
    int fan_speed = 0;

    for (int i = 8; i > 1; i--)
    {
        if (value & (0x1 << (i-1)))
        {
            fan_speed = i;
            break;
        }
    } 
    sprintf(str, "%d", fan_speed);
}

void ValueToStr_BitMap(byte value, char *str)
{
    sprintf(str, "\"%X\"", value);
}

void ValueToStr_IO_gate_1(byte value, char *str)
{
    int fan_speed;
    for (fan_speed = 8; fan_speed > 1; fan_speed--)
    {
        if (value & (1 << (fan_speed -1)))
        {
            break;
        }
    }
    strcpy(str, "{");
    json_encode_integer(str,
                        "fan_speed",
                        fan_speed);    
    strncat(str, "}", 1);
}

void ValueToStr_IO_gate_2(byte value, char *str)
{
    strcpy(str, "{");
    json_encode_integer(str,
                        "post-heating",
                        GET_BIT(value, BIT5));
    strncat(str, "}", 1);                         
}

void ValueToStr_IO_gate_3(byte value, char *str)
{
    strcpy(str, "{");
    json_encode_integer(str,
                        "HRC-position",
                        GET_BIT(value, BIT1));
    strncat(str, ",", 1);                    
    json_encode_integer(str,
                        "fault-relay",
                        GET_BIT(value, BIT2));        
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "fan-input",
                        GET_BIT(value, BIT3));       
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "pre-heating",
                        GET_BIT(value, BIT4));       
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "fan-output",
                        GET_BIT(value, BIT5));
    strncat(str, ",", 1);                         
    json_encode_integer(str,
                        "booster-switch",
                        GET_BIT(value, BIT6));                     
    strncat(str, "}", 1);                        
}

void ValueToStr_Leds(byte value, char *str)
{
    strcpy(str, "{");
    json_encode_integer(str,
                        "power-key",
                        GET_BIT(value, BIT0));
    strncat(str, ",", 1);                    
    json_encode_integer(str,
                        "CO2-key",
                        GET_BIT(value, BIT1));        
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "%RH-key",
                        GET_BIT(value, BIT2));       
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "post-heating-key",
                        GET_BIT(value, BIT3));       
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "filter-check-symbol",
                        GET_BIT(value, BIT4));
    strncat(str, ",", 1);                         
    json_encode_integer(str,
                        "post-heating-symbol",
                        GET_BIT(value, BIT5));
    strncat(str, ",", 1);                         
    json_encode_integer(str,
                        "fault-symbol",
                        GET_BIT(value, BIT6)); 
    strncat(str, ",", 1); 
    json_encode_integer(str,
                        "service-symbol",
                        GET_BIT(value, BIT7));                      
    strncat(str, "}", 1); 
}

void ValueToStr_RH(byte value, char *str)
{
    int rh = (value - 51)/2.04;
    sprintf(str, "%d", rh);
}

void ValueToStr_Counter(byte value, char *str)
{
    int secs = value/2.5;
    sprintf(str, "%d", secs);
}

byte StrToValue_CellDeFroHyst(char *str)
{
    int hyst;
    sscanf(str, "%d", &hyst);
    return hyst + 2;
}

void ValueToStr_CellDeFroHyst(byte value, char *str)
{
    int hysteresis = value - 2;
    sprintf(str, "%d", hysteresis);
}

byte StrToValue_FanPower(char *str)
{
    int fan_power;
    sscanf(str, "%d", &fan_power);
    return fan_power;
}

void ValueToStr_FanPower(byte value, char *str)
{
    sprintf(str, "%d", value);
}

T_digit_var g_digit_vars[] = 
{
    { CUR_FAN_SPEED, "cur_fan_speed", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { OUTSIDE_TEMP, "outside_temp", INVALID_VALUE, 0, 0, 15, false, false, NULL, &ValueToStr_Temperature },
    { EXHAUST_TEMP, "exhaust_temp", INVALID_VALUE, 0, 0, 15, false, false, NULL, &ValueToStr_Temperature },
    { INSIDE_TEMP, "inside_temp", INVALID_VALUE, 0, 0, 15, false, false, NULL, &ValueToStr_Temperature },
    { INCOMING_TEMP, "incoming_temp", INVALID_VALUE, 0, 0, 15, false, false, NULL, &ValueToStr_Temperature },
    { POST_HEATING_ON_CNT, "post_heating_on_cnt", INVALID_VALUE, 0, 0, 5, false, false, NULL, &ValueToStr_Counter },
    { POST_HEATING_OFF_CNT, "post_heating_off_cnt", INVALID_VALUE, 0, 0, 5, false, false, NULL, &ValueToStr_Counter },
    { INCOMING_TARGET_TEMP, "incoming_target_temp", INVALID_VALUE, 0, 0, 200, false, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { PANEL_LEDS, "panel_leds", INVALID_VALUE, 0, 0, 20, false, false, NULL, &ValueToStr_Leds },
    { MAX_FAN_SPEED, "max_fan_speed", INVALID_VALUE, 0, 0, 200, false, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { MIN_FAN_SPEED, "min_fan_speed", INVALID_VALUE, 0, 0, 20, false, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { HRC_BYPASS_TEMP, "hrc_bypass_temp", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { INPUT_FAN_STOP_TEMP, "input_fan_stop_temp", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { CELL_DEFROSTING_HYSTERESIS, "cell_defrosting_hysteresis", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_CellDeFroHyst, &ValueToStr_CellDeFroHyst },
    { DC_FAN_INPUT, "dc_fan_input", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { DC_FAN_OUTPUT, "dc_fan_output", INVALID_VALUE, 0, 0, 120, false, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { FLAGS_2, "flags_2", INVALID_VALUE, 0, 0, 20, false, false, NULL, &ValueToStr_BitMap },
    { FLAGS_4, "flags_4", INVALID_VALUE, 0, 0, 20, false, false, NULL, &ValueToStr_BitMap },
    { FLAGS_5, "flags_5", INVALID_VALUE, 0, 0, 20, false, false, NULL, &ValueToStr_BitMap },
    { FLAGS_6, "flags_6", INVALID_VALUE, 0, 0, 20, false, false, NULL, &ValueToStr_BitMap },
    { RH_MAX, "rh_max", INVALID_VALUE, 0, 0, 200, false, false, NULL, &ValueToStr_RH },
    { RH1_SENSOR, "rh1_sensor", INVALID_VALUE, 0, 0, 20, false, false, NULL, ValueToStr_RH},
    { BASIC_RH_LEVEL, "basic_rh_level", INVALID_VALUE, 0, 0, 120, false, false, NULL, ValueToStr_RH},
    { PRE_HEATING_TEMP, "pre_heating_temp", INVALID_VALUE, 0, 0, 200, false, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { IO_GATE_1, "IO_gate_1", INVALID_VALUE, 0, 0, 200, false, false, NULL, &ValueToStr_IO_gate_1 },
    { IO_GATE_2, "IO_gate_2", INVALID_VALUE, 0, 0, 200, false, false, NULL, &ValueToStr_IO_gate_2 },
    { IO_GATE_3, "IO_gate_3", INVALID_VALUE, 0, 0, 200, false, false, NULL, &ValueToStr_IO_gate_3 },
};


T_digit_var *digit_get_var_by_id(byte id)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (id == g_digit_vars[i].id)
        {
            return &g_digit_vars[i];
        }
    }
    return NULL;
}

T_digit_var *digit_get_var_by_name(char *name)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (!strcmp(name, g_digit_vars[i].name_str))
        {
            return &g_digit_vars[i];
        }
    }
    return NULL;
}


void digit_recv_msg(byte id, byte value)
{
    T_digit_var *var = digit_get_var_by_id(id);
    if (var)
    {
        if (var->set_ongoing)
        {
            if (value == var->expected_value)
            {
                // set request accomplished
                var->set_ongoing = false;
            }            
        }
        var->value = value;
        var->timestamp = time(NULL);
        var->get_ongoing = false;
    }
}

uint16 digit_calc_crc(byte msg[6])
{
    uint16 checksum = 0;
    for (int i = 0; i < 5; i++)
    {
        checksum += msg[i];
    }
    checksum %= 256;
    
    return checksum;
}


bool digit_is_valid_msg(byte msg[6])
{
    uint16 checksum ;
 
    if (msg[0] != SYSTEM_ID ||
        msg[1] != DEVICE_ADDRESS)
    {
        return false;
    }

    checksum = digit_calc_crc(msg);
    
    if (checksum == msg[5])
        return true;
    else
    {
        return false;
    }
}

void digit_send_msg(byte msg[6])
{
    msg[5] = digit_calc_crc(msg);
    rs485_send_msg(6, msg);
    usleep(100000); // sleep 100 ms
}

void digit_send_get_var(byte id)
{
    // encode get request
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, id, 0 };
    digit_send_msg(msg);
}

void digit_send_set_var(byte id, byte value)
{
    // encode set request
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, id, value, 0 };
    digit_send_msg(msg);
}


void digit_set_var(T_digit_var *var, byte value)
{
    if (var->value != value)
    {
        var->set_ongoing = true;
        var->expected_value = value;
    }
}

void digit_update_vars()
{
    time_t curr_time = time(NULL);

    // first, process set requests
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *var = &g_digit_vars[i];
        if (var->set_ongoing == true)
        {  
            if (var->value != var->expected_value)
            {
                // send set request
                digit_send_set_var(var->id, var->expected_value);
                // set get flag in order to check set request
                var->get_ongoing = true;               
            }
            else
            {
                // value correct, no need to send set request
                var->set_ongoing = false;
            }
        }
    }    
    
    // second, process active get requests
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *var = &g_digit_vars[i];
        if (var->get_ongoing == true)
        {            
            digit_send_get_var(var->id);
        }
    }
    
    // third, process interval get requests
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *var = &g_digit_vars[i];
        // no active get request ongoing and the given interval has elapsed since last value received
        if (var->get_ongoing == false && curr_time - var->timestamp >= var->interval)
        {
            // set get flag in order to send get request
            var->get_ongoing = true;
        }
    }    
    
}

void digit_receive_msgs(void)
{
    unsigned char recv_msg[6];
  
    while(1)
    {

        if (rs485_recv_msg(6, recv_msg, 20))
        {
            if (digit_is_valid_msg(recv_msg))
            {
#if 0
                for (int i = 0; i < 6; i++)
                {
                    printf("%02X ", recv_msg[i]); 
                    
                }
                printf("\n");
#endif
                
                digit_recv_msg(recv_msg[3], recv_msg[4]);
            }

        }
    }
}


float digit_get_outside_temp()
{
    T_digit_var *var = &g_digit_vars[OUTSIDE_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

float digit_get_inside_temp()
{
    T_digit_var *var = &g_digit_vars[INSIDE_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

float digit_get_exhaust_temp()
{
    T_digit_var *var = &g_digit_vars[EXHAUST_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

float digit_get_incoming_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

float digit_get_incoming_target_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    return  NTC_to_celsius(var->value);
}

void digit_set_incoming_target_temp(float temp)
{
    byte value = celsius_to_NTC(temp);
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    digit_set_var(var, value);
}

float digit_get_post_heating_on_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_ON_CNT_INDEX];
    float ret = roundf((var->value / 2.5f) * 10.0f) / 10.0f;
    return ret;
}

float digit_get_post_heating_off_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_OFF_CNT_INDEX];
    float ret = roundf((var->value / 2.5f) * 10.0f) / 10.0f;
    return ret;
}

void digit_set_var_by_name(char *name, char *str_value)
{
    T_digit_var *var = digit_get_var_by_name(name);
    byte value = var->StrToValue(str_value);
    
    digit_set_var(var, value);
}

void digit_json_encode_vars(char *str)
{
    char sub_str1[2000];
    char sub_str2[1000];
    char sub_str3[200];    
    
    strcpy(sub_str1, "");
    strcpy(str, "{");   

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        strcpy(sub_str2, "");
    
        g_digit_vars[i].ValueToStr(g_digit_vars[i].value, sub_str3);
        json_encode_string(sub_str2,
                           "value",
                           sub_str3);
                           
        strncat(sub_str2, ",", 1);
        json_encode_integer(sub_str2,
                            "ts",
                            g_digit_vars[i].timestamp);
                            
        json_encode_object(sub_str1,
                           g_digit_vars[i].name_str,
                           sub_str2);
                           
        if (i != (NUM_OF_DIGIT_VARS - 1))
        {
            strncat(sub_str1, ",", 1);
        }
    }   
    json_encode_object(str,
                       DIGIT_VARS,
                       sub_str1);
    strncat(str, "}", 1);                   
}



