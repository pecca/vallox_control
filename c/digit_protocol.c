
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

byte g_fan_speed_conversion_table [] =
{
    0x01,
    0x03,
    0x07,
    0x0F,
    0x1F,
    0x3F,
    0x7F,
    0xFF
};

byte convert_fan_speed(byte value)
{
    for (byte i = 0; i < 8; i++)
    {
        if (value == g_fan_speed_conversion_table[i])
        {
            return i+1;
        }
    }  
}


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
    int fan_speed;
    sscanf(str, "%d", &fan_speed);
    return g_fan_speed_conversion_table[fan_speed - 1];
}

void ValueToStr_FanSpeed(byte value, char *str)
{
    sprintf(str, "%d", convert_fan_speed(value));
}

void ValueToStr_BitMap(byte value, char *str)
{
    sprintf(str, "\"%X\"", value);
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
    { CUR_FAN_SPEED, "cur_fan_speed", INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { OUTSIDE_TEMP, "outside_temp", INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { EXHAUST_TEMP, "exhaust_temp", INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { INSIDE_TEMP, "inside_temp", INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { INCOMING_TEMP, "incoming_temp", INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { POST_HEATING_ON_CNT, "post_heating_on_cnt", INVALID_VALUE, 0, 0, 5, false, NULL, &ValueToStr_Counter },
    { POST_HEATING_OFF_CNT, "post_heating_off_cnt", INVALID_VALUE, 0, 0, 5, false, NULL, &ValueToStr_Counter },
    { INCOMING_TARGET_TEMP, "incoming_target_temp", INVALID_VALUE, 0, 0, 200, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { PANEL_LEDS, "panel_leds", INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { MAX_FAN_SPEED, "max_fan_speed", INVALID_VALUE, 0, 0, 200, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { MIN_FAN_SPEED, "min_fan_speed", INVALID_VALUE, 0, 0, 20, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { HRC_BYPASS_TEMP, "hrc_bypass_temp", INVALID_VALUE, 0, 0, 120, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { INPUT_FAN_STOP_TEMP, "input_fan_stop_temp", INVALID_VALUE, 0, 0, 120, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { CELL_DEFROSTING_HYSTERESIS, "cell_defrosting_hysteresis", INVALID_VALUE, 0, 0, 120, false, &StrToValue_CellDeFroHyst, &ValueToStr_CellDeFroHyst },
    { DC_FAN_INPUT, "dc_fan_input", INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { DC_FAN_OUTPUT, "dc_fan_output", INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { FLAGS_2, "flags_2", INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_4, "flags_4", INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_5, "flags_5", INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_6, "flags_6", INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { RH_MAX, "rh_max", INVALID_VALUE, 0, 0, 200, false, NULL, &ValueToStr_RH },
    { RH1_SENSOR, "rh1_sensor", INVALID_VALUE, 0, 0, 20, false, NULL, ValueToStr_RH},
    { BASIC_RH_LEVEL, "basic_rh_level", INVALID_VALUE, 0, 0, 120, false, NULL, ValueToStr_RH},
    { PRE_HEATING_TEMP, "pre_heating_temp", INVALID_VALUE, 0, 0, 200, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    
};


T_digit_var *get_digit_var(byte id)
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

T_digit_var *get_digit_var_by_name(char *name)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
		printf("cmp: name1 = %s, name2 = %s\n", name, g_digit_vars[i].name_str);
        if (!strcmp(name, g_digit_vars[i].name_str))
        {
            return &g_digit_vars[i];
        }
    }
    return NULL;
}


void update_digit_var(byte id, byte value)
{
    //  printf("msg saved: id %X, value %X\n", id, value);
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {


        if (id == g_digit_vars[i].id)
        {
			if (id == 0x29) {
				printf("get 0x29, value = %X\n");
			}
			
            g_digit_vars[i].value = value;
            g_digit_vars[i].timestamp = time(NULL);
            g_digit_vars[i].req_ongoing = false;
            return;
        }
    }
    //  printf("id=%X not saved\n", id);
}

byte get_digit_var_value(byte id, time_t *timestamp)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (id == g_digit_vars[i].id)
        {
            *timestamp = g_digit_vars[i].timestamp;
            return g_digit_vars[i].value;
        }
    } 
}

void convert_digit_var_value_to_str(byte id, char *str)
{   
    char sub_str[20];
    
    T_digit_var *digit_var = get_digit_var(id);
    byte value = digit_var->value;
    
    if (value == INVALID_VALUE)
    {
        sprintf(sub_str, "-");
    }
    else
    {  
        digit_var->ValueToStr(value, sub_str);
    }
    sprintf(str, "%s %d", sub_str, digit_var->timestamp);
}




bool digit_is_valid_msg(unsigned char msg[6])
{
    int checksum = 0;
 
    if (msg[0] != SYSTEM_ID ||
        msg[1] != DEVICE_ADDRESS)
    {
        return false;
    }

   
    for (int i = 0; i < 5; i++)
    {
        checksum += msg[i];
    }


    checksum %= 256;
    if (checksum == msg[5])
        return true;
    else
    {
        return false;
    }
}


void digit_set_crc(byte msg[6])
{
    int checksum = 0;
    
    for (int i = 0; i < 5; i++)
    {
        checksum += msg[i];
    }
    checksum %= 256;
    msg[5] = checksum;
}

void digit_send_get_var(byte id)
{
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, id, 0 };
    digit_set_crc(msg);
    rs485_send_msg(6, msg);
}

void digit_send_set_var(T_digit_var *digit_var, byte value)
{
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, digit_var->id, value, 0 };
    digit_set_crc(msg);
    rs485_send_msg(6, msg);
    digit_var->req_ongoing = true;
}


bool digit_set_var(byte id, char* str_value)
{
	printf("id = %X, str = %s\n", id, str_value);
    T_digit_var *digit_var = get_digit_var(id);
    if (digit_var != NULL && digit_var->StrToValue != NULL)
    {
        int i;
        byte value = digit_var->StrToValue(str_value);
		printf("id = %X, str = %s, value =%X\n", id, str_value, value);
        digit_send_set_var(digit_var, value);
        usleep(10000); // sleep for 10ms
        digit_send_get_var(id);
        usleep(100000); // sleep for 100ms
        for (int i = 0; i < 1000; i++)
        {
            if (value == digit_var->value)
            {
                break;
            }
            else
            {
                usleep(10000); // sleep for 10ms
            }
        }
        if (value == digit_var->value)
        {
            return true;
        }
    }
    return false;
}



bool digit_recv_response(byte id, byte *value)
{
    byte recv_msg[6];
    int recv_msg_max_cnt = 5;
    
    while(recv_msg_max_cnt > 0)
    {
        if (rs485_recv_msg(6, recv_msg, 20))
        {
            if (recv_msg[0] == SYSTEM_ID &&
                recv_msg[1] == DEVICE_ADDRESS &&
                recv_msg[2] == PI_ADDRESS &&
                recv_msg[3] == id  &&
                digit_is_valid_msg(recv_msg))
            {
                *value = recv_msg[4];
				printf("msg id = %02X received\n", id);
                return true;
            }
            recv_msg_max_cnt--;
        }
	
    }
    return false;    
}


void digit_update_vars()
{
    byte recv_msg[6];
    time_t curr_time = time(NULL);
    byte recv_msg_max_cnt;
	
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *var = &g_digit_vars[i];
        if (var->req_ongoing == true || curr_time - var->timestamp >= var->interval)
        {            
            digit_send_get_var(var->id);
            var->req_ongoing = true;
            usleep(100000); // sleep for 100ms
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
                
                update_digit_var(recv_msg[3], recv_msg[4]);
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
    digit_send_set_var(&g_digit_vars[INCOMING_TARGET_TEMP_INDEX], value);
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

void digit_process_set_var(char *name, char *value)
{
	printf("name = %s\n", name);
	T_digit_var *var = get_digit_var_by_name(name);
	printf("name2 = %s, id = %X\n", var->name_str, var->id);
	digit_set_var(var->id, value);
}

void digit_json_encode_vars(char *str)
{
    char str_temp[100];
    char *str_temp2;

    strcpy(str_temp, "{ \"digit_vars\" : { ");
    strcpy(str, str_temp);

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        char sub_str[20];
        byte value = g_digit_vars[i].value;

        
        str_temp2 = g_digit_vars[i].name_str;
        strncat(str, "\"", 1);
        strncat(str, str_temp2, strlen(str_temp2));
        strncat(str, "\"", 1);
        
        strcpy(str_temp, " : { \"value\" : ");
        strncat(str, str_temp, strlen(str_temp));
        g_digit_vars[i].ValueToStr(value, sub_str);
        strncat(str, sub_str, strlen(sub_str));
        strcpy(str_temp, ", \"ts\" : ");
        strncat(str, str_temp, strlen(str_temp));
        sprintf(sub_str, "%d", g_digit_vars[i].timestamp);
        strncat(str, sub_str, strlen(sub_str));
        strncat(str, "}", 1);
        if (i != (NUM_OF_DIGIT_VARS - 1))
        {
            strncat(str, ", ", 2);
        } 
    }
    strncat(str, "}}", 2);
}

