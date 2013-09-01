
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdbool.h>
#include <time.h>
#include "digit_protocol.h"
#include "rs485.h"
#include "temperature_conversion.h"
#include "DS18B20.h"

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
    byte fan_speed;
    sscanf(str, "%d", &fan_speed);
    return g_fan_speed_conversion_table[fan_speed - 1];
}

void ValueToStr_FanSpeed(byte value, char *str)
{
    sprintf(str, "%d", convert_fan_speed(value));
}

void ValueToStr_BitMap(byte value, char *str)
{
    sprintf(str, "%X", value);
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
    byte hyst;
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
    byte fan_power;
    sscanf(str, "%d", &fan_power);
    return fan_power;
}

void ValueToStr_FanPower(byte value, char *str)
{
    sprintf(str, "%d", value);
}

T_digit_var g_digit_vars[] = 
{
    { CUR_FAN_SPEED, INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { OUTSIDE_TEMP, INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { EXHAUST_TEMP, INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { INSIDE_TEMP, INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { INCOMING_TEMP, INVALID_VALUE, 0, 0, 15, false, NULL, &ValueToStr_Temperature },
    { PANEL_LEDS, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { RH_MAX, INVALID_VALUE, 0, 0, 200, false, NULL, &ValueToStr_RH },
    { FLAGS_2, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_4, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_5, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { FLAGS_6, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_BitMap },
    { POST_HEATING_ON_CNT, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_Counter },
    { POST_HEATING_OFF_CNT, INVALID_VALUE, 0, 0, 20, false, NULL, &ValueToStr_Counter },
    { INCOMING_TARGET_TEMP, INVALID_VALUE, 0, 0, 200, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { MAX_FAN_SPEED, INVALID_VALUE, 0, 0, 200, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { MIN_FAN_SPEED, INVALID_VALUE, 0, 0, 200, false, &StrToValue_FanSpeed, &ValueToStr_FanSpeed },
    { HRC_BYPASS_TEMP, INVALID_VALUE, 0, 0, 120, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { INPUT_FAN_STOP_TEMP, INVALID_VALUE, 0, 0, 120, false, &StrToValue_Temperature, &ValueToStr_Temperature },
    { CELL_DEFROSTING_HYSTERESIS, INVALID_VALUE, 0, 0, 120, false, &StrToValue_CellDeFroHyst, &ValueToStr_CellDeFroHyst },
    { DC_FAN_INPUT, INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { DC_FAN_OUTPUT, INVALID_VALUE, 0, 0, 120, false, &StrToValue_FanPower, &ValueToStr_FanPower },
    { RH1_SENSOR, INVALID_VALUE, 0, 0, 20, false, NULL, ValueToStr_RH},
    { BASIC_RH_LEVEL, INVALID_VALUE, 0, 0, 120, false, NULL, ValueToStr_RH} 
    
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
}

void update_digit_var(byte id, byte value)
{

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (id == g_digit_vars[i].id)
        {
            g_digit_vars[i].value = value;
            g_digit_vars[i].timestamp = time(NULL);
            g_digit_vars[i].req_ongoing = false;
            return;
        }
    }
    printf("id=%X not saved\n", id);
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
    byte value;
    

    if (id == DS18B20_SENSOR1)
    {
        sprintf(str, "%.1f %d", get_DS18B20_outside_temp(), get_DS18B20_outside_temp_ts());
    }
    else if (id == DS18B20_SENSOR2)
    {
        sprintf(str, "%.1f %d",  get_DS18B20_exhaust_temp(),  get_DS18B20_exhaust_temp_ts());
    }
    else
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
        printf("recv crc = %X, expected crc %X, msg_id %X\n", msg[5], checksum, msg[3]);
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

void digit_request_var(byte id)
{
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, id, 0 };
    digit_set_crc(msg);
    rs485_send_msg(6, msg);
}

void digit_set_var(byte id, byte value)
{
    byte msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, id, value, 0 };
    digit_set_crc(msg);
    rs485_send_msg(6, msg);
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

         

    sleep(30);
    
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *var = &g_digit_vars[i];
        if (curr_time - var->timestamp >= var->interval)
        {            
            digit_request_var(var->id);
            var->req_ongoing = true;
            printf("send var=%X req\n", var->id);
            sleep(1);
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


