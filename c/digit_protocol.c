
#if 0

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <time.h>
#include <math.h>
#include <string.h>

#endif

#include "common.h"

#include "digit_protocol.h"
#include "rs485.h"
#include "temperature_conversion.h"
#include "json_codecs.h"

T_digit_var g_digit_vars[NUM_OF_DIGIT_VARS];

void *pvDigit_receive_thread( void *ptr )
{
    rs485_open();
    sleep(2);
    while (true)
    {
        digit_receive_msgs();
    }
    return NULL;
} 


void *pvDigit_update_thread( void *ptr )
{
    digit_init();
    sleep(10);
    while (1)
    {
        digit_update_vars();
        sleep(3);
    }
    return NULL;
} 

int get_fan_speed(byte value)
{
    int fan_speed = 1;
    for (int i = 8; i >= 1; i--)
    {
        if (value & (0x1 << (i-1)))
        {
            fan_speed = i;
            break;
        }
    }
    return fan_speed;
}

byte StrToValue_Temperature(char *str)
{
    real32 temp;
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
    int fan_speed = get_fan_speed(value);
    sprintf(str, "%d", fan_speed);
}

void ValueToStr_BitMap(byte value, char *str)
{
    sprintf(str, "\"%X\"", value);
}

void ValueToStr_IO_gate_1(byte value, char *str)
{
    int fan_speed = get_fan_speed(value);

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

void digit_init_var(byte index, byte id, char *name, time_t interval, StrToValue_t strToValue, ValueToStr_t valueToStr)
{
    T_digit_var *digit_var = &g_digit_vars[index];
    
    digit_var->id = id;
    strcpy(digit_var->name_str, name);
    digit_var->interval = interval;
    digit_var->StrToValue = strToValue;
    digit_var->ValueToStr = valueToStr;
} 

void digit_init(void)
{
    memset(&g_digit_vars, 0, sizeof(g_digit_vars));
    
    digit_init_var(DIGIT_PARAM(CUR_FAN_SPEED), 120, &StrToValue_FanSpeed, &ValueToStr_FanSpeed);
    digit_init_var(DIGIT_PARAM(OUTSIDE_TEMP), 15, NULL, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(EXHAUST_TEMP), 15, NULL, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(INSIDE_TEMP), 15, NULL, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(INCOMING_TEMP), 15, NULL, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(POST_HEATING_ON_CNT), 5, NULL, &ValueToStr_Counter);
    digit_init_var(DIGIT_PARAM(POST_HEATING_OFF_CNT), 5, NULL, &ValueToStr_Counter);
    digit_init_var(DIGIT_PARAM(INCOMING_TARGET_TEMP), 200, &StrToValue_Temperature, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(PANEL_LEDS), 20, NULL, &ValueToStr_Leds);
    digit_init_var(DIGIT_PARAM(MAX_FAN_SPEED), 200, &StrToValue_FanSpeed, &ValueToStr_FanSpeed);
    digit_init_var(DIGIT_PARAM(MIN_FAN_SPEED), 20, &StrToValue_FanSpeed, &ValueToStr_FanSpeed);
    digit_init_var(DIGIT_PARAM(HRC_BYPASS_TEMP), 120, &StrToValue_Temperature, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(INPUT_FAN_STOP_TEMP), 120, &StrToValue_Temperature, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(CELL_DEFROSTING_HYSTERESIS), 120, &StrToValue_CellDeFroHyst, &ValueToStr_CellDeFroHyst);
    digit_init_var(DIGIT_PARAM(DC_FAN_INPUT), 120, &StrToValue_FanPower, &ValueToStr_FanPower);
    digit_init_var(DIGIT_PARAM(DC_FAN_OUTPUT), 120, &StrToValue_FanPower, &ValueToStr_FanPower);
    digit_init_var(DIGIT_PARAM(FLAGS_2), 20, NULL, &ValueToStr_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_4), 20, NULL, &ValueToStr_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_5), 20, NULL, &ValueToStr_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_6), 20, NULL, &ValueToStr_BitMap);
    digit_init_var(DIGIT_PARAM(RH_MAX), 200, NULL, &ValueToStr_RH);
    digit_init_var(DIGIT_PARAM(RH1_SENSOR), 20, NULL, ValueToStr_RH);
    digit_init_var(DIGIT_PARAM(BASIC_RH_LEVEL), 120, NULL, ValueToStr_RH);
    digit_init_var(DIGIT_PARAM(PRE_HEATING_TEMP), 200, &StrToValue_Temperature, &ValueToStr_Temperature);
    digit_init_var(DIGIT_PARAM(IO_GATE_1), 200, NULL, &ValueToStr_IO_gate_1);
    digit_init_var(DIGIT_PARAM(IO_GATE_2), 200, NULL, &ValueToStr_IO_gate_2);
    digit_init_var(DIGIT_PARAM(IO_GATE_3), 200, NULL, &ValueToStr_IO_gate_3);
}

bool digit_vars_ok(void)
{
    time_t curr_time = time(NULL);

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (curr_time - g_digit_vars[i].timestamp > 300) 
        {
            return false;
        }
    }
    return true;
}

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
    //printf("recv_msg: id = %02X\n", id);
    if (var)
    {
        if (var->set_ongoing)
        {
            if (value == var->expected_value)
            {
                // set request accomplished
                //printf("set resp received: id = %02X, cnt = %d\n", id, var->set_req_cnt);
                var->set_ongoing = false;
                var->set_req_cnt = 0;
            }            
        }
        var->value = value;
        var->timestamp = time(NULL);
        if (var->get_ongoing)
        {
            //printf("get resp received: id = %02X, cnt = %d\n", id, var->get_req_cnt);
            var->get_ongoing = false;
            var->get_req_cnt = 0;
        }
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
//  printf("send_msg: id = %02X\n", id);
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
                var->set_req_cnt++;
            }
            else
            {
                // value correct, no need to send set request
                var->set_ongoing = false;
                var->set_req_cnt = 0;
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
           var->get_req_cnt++;
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
            var->get_req_cnt++;
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

real32 digit_get_rh1_sensor()
{
    T_digit_var *var = &g_digit_vars[RH1_SENSOR_INDEX]; 
    return (var->value - 51)/2.04;
}

real32 digit_get_outside_temp()
{
    T_digit_var *var = &g_digit_vars[OUTSIDE_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

real32 digit_get_inside_temp()
{
    T_digit_var *var = &g_digit_vars[INSIDE_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

real32 digit_get_exhaust_temp()
{
    T_digit_var *var = &g_digit_vars[EXHAUST_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

real32 digit_get_incoming_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TEMP_INDEX];
    return  NTC_to_celsius(var->value); 
}

real32 digit_get_incoming_target_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    return  NTC_to_celsius(var->value);
}

void digit_set_incoming_target_temp(real32 temp)
{
    byte value = celsius_to_NTC(temp);
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    digit_set_var(var, value);
}

real32 digit_get_post_heating_on_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_ON_CNT_INDEX];
    real32 ret = roundf(((var->value / 2.5f) * 10.0f) / 10.0f);
    return ret;
}

int digit_get_cur_fan_speed(void)
{
    T_digit_var *var = &g_digit_vars[CUR_FAN_SPEED_INDEX];
    int fan_speed = get_fan_speed(var->value);
    return fan_speed;
}

real32 digit_get_post_heating_off_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_OFF_CNT_INDEX];
    real32 ret = roundf(((var->value / 2.5f) * 10.0f) / 10.0f);
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



