/**
 * @file   digit_protocol.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of Vallox digit protocol
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/ 
 
#include "common.h"
#include "digit_protocol.h"
#include "rs485.h"
#include "temperature_conversion.h"
#include "json_codecs.h"

/******************************************************************************
 *  Constants and macros
 ******************************************************************************/ 

// Device addresses
#define SYSTEM_ID      0x01
#define DEVICE_ADDRESS 0x11
#define PANEL_ADDRESS  0x21
#define PI_ADDRESS     0x22

// Parameters IDs
#define CUR_FAN_SPEED                   0x29
#define OUTSIDE_TEMP                    0x32
#define EXHAUST_TEMP                    0x33
#define INSIDE_TEMP                     0x34
#define INCOMING_TEMP                   0x35
#define POST_HEATING_ON_CNT             0x55
#define POST_HEATING_OFF_CNT            0x56
#define INCOMING_TARGET_TEMP            0xA4
#define PANEL_LEDS                      0xA3
#define MAX_FAN_SPEED                   0xA5
#define MIN_FAN_SPEED                   0xA9
#define HRC_BYPASS_TEMP                 0xAF
#define INPUT_FAN_STOP_TEMP             0xA8 
#define CELL_DEFROSTING_HYSTERESIS      0xB2
#define DC_FAN_INPUT                    0xB0
#define DC_FAN_OUTPUT                   0xB1
#define FLAGS_2                         0x6D
#define FLAGS_4                         0x6F
#define FLAGS_5                         0x70
#define FLAGS_6                         0x71
#define RH_MAX                          0x2A
#define RH1_SENSOR                      0x2F
#define BASIC_RH_LEVEL                  0xAE
#define PRE_HEATING_TEMP                0xA7
#define IO_GATE_1                       0x06
#define IO_GATE_2                       0x07
#define IO_GATE_3                       0x08

// Parameter names
#define CUR_FAN_SPEED_NAME              "cur_fan_speed"
#define OUTSIDE_TEMP_NAME               "outside_temp"
#define EXHAUST_TEMP_NAME               "exhaust_temp"
#define INSIDE_TEMP_NAME                "inside_temp"
#define INCOMING_TEMP_NAME              "incoming_temp"
#define POST_HEATING_ON_CNT_NAME        "post_heating_on_cnt"
#define POST_HEATING_OFF_CNT_NAME       "post_heating_off_cnt"
#define INCOMING_TARGET_TEMP_NAME       "incoming_target_temp"
#define PANEL_LEDS_NAME                 "panel_leds"
#define MAX_FAN_SPEED_NAME              "max_fan_speed"
#define MIN_FAN_SPEED_NAME              "min_fan_speed"
#define HRC_BYPASS_TEMP_NAME            "hrc_bypass_temp"
#define INPUT_FAN_STOP_TEMP_NAME        "input_fan_stop_temp" 
#define CELL_DEFROSTING_HYSTERESIS_NAME "cell_defrosting_hysteresis"
#define DC_FAN_INPUT_NAME               "dc_fan_input"
#define DC_FAN_OUTPUT_NAME              "dc_fan_output"
#define FLAGS_2_NAME                    "flags_2"
#define FLAGS_4_NAME                    "flags_4"
#define FLAGS_5_NAME                    "flags_5"
#define FLAGS_6_NAME                    "flags_6"
#define RH_MAX_NAME                     "rh_max"
#define RH1_SENSOR_NAME                 "rh1_sensor"
#define BASIC_RH_LEVEL_NAME             "basic_rh_level"
#define PRE_HEATING_TEMP_NAME           "pre_heating_temp"
#define IO_GATE_1_NAME                  "IO_gate_1"
#define IO_GATE_2_NAME                  "IO_gate_2"
#define IO_GATE_3_NAME                  "IO_gate_3"

// Parameter indexes
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

#define NUM_OF_DIGIT_VARS                 27 

// Combines parameter index, id, name 
#define DIGIT_PARAM(var) var##_INDEX , var, var##_NAME  
 
#define NAME_MAX_SIZE                     30
 
// Bitfield coding
#define GET_BIT(value, bit) (bool)(value & (1 << bit)) 
#define BIT0   0
#define BIT1   1
#define BIT2   2
#define BIT3   3
#define BIT4   4
#define BIT5   5
#define BIT6   6
#define BIT7   7

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/

// Encode function type
typedef uint8 (*u8_encode_t) (char*);
// Decode function type
typedef void (*decode_t) (uint8, char*);

typedef struct
{
    uint8 id;
    char name_str[NAME_MAX_SIZE];
    uint8 value;
    uint8 expected_value;
    time_t timestamp;
    time_t interval;
    bool get_ongoing;
    bool set_ongoing;
    uint32 get_req_cnt;
    uint32 set_req_cnt;
    u8_encode_t StrToValue;
    decode_t ValueToStr;
} T_digit_var;

/******************************************************************************
 *  Local variables
 ******************************************************************************/
 
T_digit_var g_digit_vars[NUM_OF_DIGIT_VARS];

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/

void digit_receive_msgs(void); 

void digit_update_vars(void);

int get_fan_speed(uint8 value);

T_digit_var *digit_get_var_by_name(char *name);

void digit_set_var(T_digit_var *var, uint8 value);

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void *digit_receive_thread(void *ptr)
{
    rs485_open();
    sleep(2); // sec
    while (true)
    {
        digit_receive_msgs();
    }
    return NULL;
} 

void *digit_update_thread( void *ptr )
{
    digit_init();
    sleep(10); // sec
    while (true)
    {
        digit_update_vars();
        sleep(3); // sec
    }
    return NULL;
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

void digit_set_var_by_name(char *name, char *str_value)
{
    T_digit_var *var = digit_get_var_by_name(name);
    uint8 value = var->StrToValue(str_value);
    
    digit_set_var(var, value);
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

int get_fan_speed(uint8 value)
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



uint8 u8_encode_Temperature(char *str)
{
    real32 temp;
    sscanf(str, "%f", &temp); 
    return celsius_to_NTC(temp);
}

void decode_Temperature(uint8 value, char *str)
{
    sprintf(str, "%.1f", NTC_to_celsius(value));
}

uint8 u8_encode_FanSpeed(char *str)
{
    uint8 ret = 0;
    int fan_speed;
    sscanf(str, "%d", &fan_speed);
    for (int i = 0; i < fan_speed; i++)
    {
       ret |= (0x1 << i); 
    }
    return ret;
}

void decode_FanSpeed(uint8 value, char *str)
{
    int fan_speed = get_fan_speed(value);
    sprintf(str, "%d", fan_speed);
}

void decode_BitMap(uint8 value, char *str)
{
    sprintf(str, "\"%X\"", value);
}

void decode_IO_gate_1(uint8 value, char *str)
{
    int fan_speed = get_fan_speed(value);

    strcpy(str, "{");
    json_encode_integer(str,
                        "fan_speed",
                        fan_speed);    
    strncat(str, "}", 1);
}

void decode_IO_gate_2(uint8 value, char *str)
{
    strcpy(str, "{");
    json_encode_integer(str,
                        "post-heating",
                        GET_BIT(value, BIT5));
    strncat(str, "}", 1);                         
}

void decode_IO_gate_3(uint8 value, char *str)
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

void decode_Leds(uint8 value, char *str)
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

void decode_RH(uint8 value, char *str)
{
    int rh = (value - 51)/2.04;
    sprintf(str, "%d", rh);
}

void decode_Counter(uint8 value, char *str)
{
    int secs = value/2.5;
    sprintf(str, "%d", secs);
}

uint8 u8_encode_CellDeFroHyst(char *str)
{
    int hyst;
    sscanf(str, "%d", &hyst);
    return hyst + 2;
}

void decode_CellDeFroHyst(uint8 value, char *str)
{
    int hysteresis = value - 2;
    sprintf(str, "%d", hysteresis);
}

uint8 u8_encode_FanPower(char *str)
{
    int fan_power;
    sscanf(str, "%d", &fan_power);
    return fan_power;
}

void decode_FanPower(uint8 value, char *str)
{
    sprintf(str, "%d", value);
}

void digit_init_var(uint8 index, uint8 id, char *name, time_t interval, u8_encode_t strToValue, decode_t valueToStr)
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
    
    digit_init_var(DIGIT_PARAM(CUR_FAN_SPEED), 120, &u8_encode_FanSpeed, &decode_FanSpeed);
    digit_init_var(DIGIT_PARAM(OUTSIDE_TEMP), 15, NULL, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(EXHAUST_TEMP), 15, NULL, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(INSIDE_TEMP), 15, NULL, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(INCOMING_TEMP), 15, NULL, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(POST_HEATING_ON_CNT), 5, NULL, &decode_Counter);
    digit_init_var(DIGIT_PARAM(POST_HEATING_OFF_CNT), 5, NULL, &decode_Counter);
    digit_init_var(DIGIT_PARAM(INCOMING_TARGET_TEMP), 200, &u8_encode_Temperature, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(PANEL_LEDS), 20, NULL, &decode_Leds);
    digit_init_var(DIGIT_PARAM(MAX_FAN_SPEED), 200, &u8_encode_FanSpeed, &decode_FanSpeed);
    digit_init_var(DIGIT_PARAM(MIN_FAN_SPEED), 20, &u8_encode_FanSpeed, &decode_FanSpeed);
    digit_init_var(DIGIT_PARAM(HRC_BYPASS_TEMP), 120, &u8_encode_Temperature, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(INPUT_FAN_STOP_TEMP), 120, &u8_encode_Temperature, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(CELL_DEFROSTING_HYSTERESIS), 120, &u8_encode_CellDeFroHyst, &decode_CellDeFroHyst);
    digit_init_var(DIGIT_PARAM(DC_FAN_INPUT), 120, &u8_encode_FanPower, &decode_FanPower);
    digit_init_var(DIGIT_PARAM(DC_FAN_OUTPUT), 120, &u8_encode_FanPower, &decode_FanPower);
    digit_init_var(DIGIT_PARAM(FLAGS_2), 20, NULL, &decode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_4), 20, NULL, &decode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_5), 20, NULL, &decode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_6), 20, NULL, &decode_BitMap);
    digit_init_var(DIGIT_PARAM(RH_MAX), 200, NULL, &decode_RH);
    digit_init_var(DIGIT_PARAM(RH1_SENSOR), 20, NULL, decode_RH);
    digit_init_var(DIGIT_PARAM(BASIC_RH_LEVEL), 120, NULL, decode_RH);
    digit_init_var(DIGIT_PARAM(PRE_HEATING_TEMP), 200, &u8_encode_Temperature, &decode_Temperature);
    digit_init_var(DIGIT_PARAM(IO_GATE_1), 200, NULL, &decode_IO_gate_1);
    digit_init_var(DIGIT_PARAM(IO_GATE_2), 200, NULL, &decode_IO_gate_2);
    digit_init_var(DIGIT_PARAM(IO_GATE_3), 200, NULL, &decode_IO_gate_3);
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

T_digit_var *digit_get_var_by_id(uint8 id)
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


void digit_recv_msg(uint8 id, uint8 value)
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

uint16 digit_calc_crc(uint8 msg[6])
{
    uint16 checksum = 0;
    for (int i = 0; i < 5; i++)
    {
        checksum += msg[i];
    }
    checksum %= 256;
    
    return checksum;
}


bool digit_is_valid_msg(uint8 msg[6])
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

void digit_send_msg(uint8 msg[6])
{
    msg[5] = digit_calc_crc(msg);
    rs485_send_msg(6, msg);
    usleep(100000); // sleep 100 ms
}

void digit_send_get_var(uint8 id)
{
    // encode get request
    uint8 msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, id, 0 };
    digit_send_msg(msg);
//  printf("send_msg: id = %02X\n", id);
}

void digit_send_set_var(uint8 id, uint8 value)
{
    // encode set request
    uint8 msg[6] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, id, value, 0 };
    digit_send_msg(msg);
}


void digit_set_var(T_digit_var *var, uint8 value)
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
    uint8 value = celsius_to_NTC(temp);
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







