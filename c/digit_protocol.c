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

// Variable IDs
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

 #define NAME_MAX_SIZE                  30

// Variable names
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

// Variable indexes
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

// Combines variable's index, id and name
#define DIGIT_PARAM(var) \
    var##_INDEX , var, var##_NAME

// Message size
#define DIGIT_MSG_LEN       6

// Message fields (indexes to message)
#define DIGIT_MSG_SYSTEM    0
#define DIGIT_MSG_SENDER    1
#define DIGIT_MSG_RECEIVER  2
#define DIGIT_MSG_VARIABLE  3
#define DIGIT_MSG_DATA      4
#define DIGIT_MSG_CRC       5



// Bitfield coding
#define GET_BIT(value, bit) (bool)(value & (1 << bit))
#define SET_BIT(value, bit) (value |= (1 << bit))
#define CLEAR_BIT(value, bit) (value &= (~(1 << bit)))
#define BIT0   0
#define BIT1   1
#define BIT2   2
#define BIT3   3
#define BIT4   4
#define BIT5   5
#define BIT6   6
#define BIT7   7

// Encode help strings maximum sizes
#define ENCODE_STR1_SIZE 2000
#define ENCODE_STR2_SIZE 1000
#define ENCODE_STR3_SIZE 200

#define RS485_READ_ATTEMPTS 20
#define RS485_FILE          "/dev/ttyUSB0"

// #define DEBUG_DIGIT_RECV // uncomment for debugging

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/

// Decode function type
typedef uint8 (*u8_decode_t) (char*);
// Encode function type
typedef void (*encode_t) (uint8, char*);

typedef struct
{
    uint8 u8Id;                     // ID
    char sNameStr[NAME_MAX_SIZE];   // name
    uint8 u8Value;                  // Current value
    uint8 u8ExpectedValue;          // Expected value
    uint8 u8ExpectedMask;           // Mask used in comparison
    time_t tTimestamp;              // Timestamp when value updated
    time_t tInternal;               // Value update interval
    bool bGetOngoing;               // Status of get request
    bool bSetOngoing;               // Status of set request
    uint32 u32GetReqCnt;            // Get request sent counter
    uint32 u32SetReqCnt;            // Set request sent counter
    u8_decode_t decodeFunPtr;       // JSON decode function
    encode_t encodeFunPtr;          // Json encode function
} T_digit_var;

/******************************************************************************
 *  Local variables
 ******************************************************************************/

static T_digit_var g_digit_vars[NUM_OF_DIGIT_VARS];

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/

// Initialize module
static void digit_init(void);
static void digit_init_var(uint8 u8Index, uint8 u8Id, char *sName, time_t tInternal,
                           u8_decode_t decodeFunPtr, encode_t encodeFunPtr);

// Read message(s) from RS485 bus and process message(s)
static void digit_receive_msgs(void);

// Check through variable statuses. Send set or get request if needed.
static void digit_update_vars(void);

// Process received message
static void digit_process_msg(uint8 u8Id, uint8 value);

// Activate set request
static void digit_set_change_req(T_digit_var *ptVar, uint8 u8Value);

// Send set request
static void digit_send_set_req(uint8 u8Id, uint8 u8Value);

// Send get request
static void digit_send_get_req(uint8 u8Id);

// Calculates message CRC
static uint16 u16_digit_calc_crc(uint8 msg[DIGIT_MSG_LEN]);

// Check is message valid
static bool digit_is_valid_msg(uint8 msg[DIGIT_MSG_LEN]);

// Send message to RS485 bus
static void digit_send_msg(uint8 msg[DIGIT_MSG_LEN]);

// Search functions
static T_digit_var *digit_get_var_by_name(char *sName);
static T_digit_var *digit_get_var_by_id(uint8 u8Id);

// JSON decode functions
static uint8 u8_decode_FanSpeed(char *sStr);
static uint8 u8_decode_int_FanSpeed(uint8 u8FanSpeed);
static uint8 u8_decode_Temperature(char *sStr);
static uint8 u8_decode_CellDeFroHyst(char *sStr);
static uint8 u8_decode_FanPower(char *sStr);

// JSON encode function
static void encode_Temperature(uint8 u8Value, char *sStr);
static void encode_FanSpeed(uint8 u8Value, char *sStr);
static void encode_BitMap(uint8 u8Value, char *sStr);
static void encode_IO_gate_1(uint8 u8Value, char *sStr);
static void encode_IO_gate_2(uint8 u8Value, char *sStr);
static void encode_IO_gate_3(uint8 u8Value, char *sStr);
static void encode_Leds(uint8 u8Value, char *sStr);
static void encode_RH(uint8 u8Value, char *sStr);
static void encode_Counter(uint8 u8Value, char *sStr);
static void encode_CellDeFroHyst(uint8 u8Value, char *sStr);
static void encode_FanPower(uint8 u8Value, char *sStr);
static uint8 u8_encode_fan_speed(uint8 u8Value);

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

// Listen RS485 bus and process received messages.
void *digit_receive_thread(void *ptr)
{
    rs485_open(RS485_FILE);
    sleep(2); // sec
    while (true)
    {
        digit_receive_msgs();
    }
    return NULL;
}

// Check variable statuses and send get and set requests.
void *digit_update_thread( void *ptr )
{
    digit_init();
    sleep(10); // sec
    while (true)
    {
        digit_update_vars();
        sleep(2); // sec
    }
    return NULL;
}

// Generates JSON message containing all variables
void digit_json_encode_vars(char *str)
{
    char sSubStr1[ENCODE_STR1_SIZE];
    char sSubStr2[ENCODE_STR2_SIZE];
    char sSubStr3[ENCODE_STR3_SIZE];

    strcpy(sSubStr1, "");
    strcpy(str, "{");

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        strcpy(sSubStr2, "");

        g_digit_vars[i].encodeFunPtr(g_digit_vars[i].u8Value, sSubStr3);
        json_encode_string(sSubStr2,
                           "value",
                           sSubStr3);

        strncat(sSubStr2, ",", 1);
        json_encode_integer(sSubStr2,
                            "ts",
                            g_digit_vars[i].tTimestamp);

        json_encode_object(sSubStr1,
                           g_digit_vars[i].sNameStr,
                           sSubStr2);

        if (i != (NUM_OF_DIGIT_VARS - 1))
        {
            strncat(sSubStr1, ",", 1);
        }
    }
    json_encode_object(str,
                       DIGIT_VARS,
                       sSubStr1);
    strncat(str, "}", 1);
}

// Process JSON message for changing value and update set request status.
void digit_set_var_by_name(char *name, char *str_value, char *str)
{
    T_digit_var *var = digit_get_var_by_name(name);
    uint8 value = var->decodeFunPtr(str_value);

    bool setOk = digit_set_change_req(var, value);
    if (setOk) {
        strcpy(str, "{\"status\": true}");
    } else {
        strcpy(str, "{\"status\": false}");
    }
}

// Check that variables are updated within intervals.
bool digit_vars_ok(void)
{
    time_t curr_time = time(NULL);

    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (curr_time - g_digit_vars[i].tTimestamp > 300)
        {
            return false;
        }
    }
    return true;
}

// Return air humidity
real32 r32_digit_rh1_sensor()
{
    T_digit_var *var = &g_digit_vars[RH1_SENSOR_INDEX];
    return (var->u8Value - 51)/2.04;
}

// Return outdoor temperature
real32 r32_digit_outside_temp()
{
    T_digit_var *var = &g_digit_vars[OUTSIDE_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

// Return inside temperature
real32 r32_digit_inside_temp()
{
    T_digit_var *var = &g_digit_vars[INSIDE_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

// Return exhaust air temperature
real32 r32_digit_exhaust_temp()
{
    T_digit_var *var = &g_digit_vars[EXHAUST_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

// Return incoming air temperature
real32 r32_digit_incoming_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

// Return temperature of target incoming air
real32 r32_digit_incoming_target_temp()
{
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

// Set incoming target temperature
void digit_set_incoming_target_temp(real32 temp)
{
    uint8 value = u16_celsius_to_NTC(temp);
    T_digit_var *var = &g_digit_vars[INCOMING_TARGET_TEMP_INDEX];
    digit_set_change_req(var, value);
}

// Set input fan stop temperature (exhaust)
void digit_set_input_fan_stop(real32 temp)
{
    uint8 value = u16_celsius_to_NTC(temp);
    T_digit_var *var = &g_digit_vars[INPUT_FAN_STOP_TEMP_INDEX];
    digit_set_change_req(var, value);
}

real32 r32_digit_input_fan_stop_temp()
{
    T_digit_var *var = &g_digit_vars[INPUT_FAN_STOP_TEMP_INDEX];
    return  r32_NTC_to_celsius(var->u8Value);
}

void digit_set_input_fan_off(bool off)
{
    uint8 value = 0;
    uint8 mask = 0;
    SET_BIT(mask, BIT3);

    if (off)
    {
        SET_BIT(value, BIT3);
    }
    else
    {
        CLEAR_BIT(value, BIT3);
    }
    SET_BIT(value, BIT4);

    T_digit_var *var = &g_digit_vars[IO_GATE_3_INDEX];
    var->u8ExpectedMask = mask;
    digit_set_change_req(var, value);
}

bool b_digit_input_fan_off(void)
{
    T_digit_var *var = &g_digit_vars[IO_GATE_3_INDEX];
    return (bool) GET_BIT(var->u8Value, BIT3);
}

// Return post heating ON counter
real32 r32_digit_post_heating_on_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_ON_CNT_INDEX];
    real32 ret = roundf(((var->u8Value / 2.5f) * 10.0f) / 10.0f);
    return ret;
}

// Return post heating ON counter
real32 r32_digit_post_heating_off_cnt(void)
{
    T_digit_var *var = &g_digit_vars[POST_HEATING_OFF_CNT_INDEX];
    real32 ret = roundf(((var->u8Value / 2.5f) * 10.0f) / 10.0f);
    return ret;
}

// Return current fan speed
uint8 u8_digit_cur_fan_speed(void)
{
    T_digit_var *var = &g_digit_vars[CUR_FAN_SPEED_INDEX];
    int fan_speed = u8_encode_fan_speed(var->u8Value);
    return fan_speed;
}

void digit_set_min_fan_speed(uint8 u8MinSpeed)
{
    uint8 value = u8_decode_int_FanSpeed(u8MinSpeed);
    T_digit_var *var = &g_digit_vars[MIN_FAN_SPEED_INDEX];
    digit_set_change_req(var, value);
}

uint8 u8_digit_min_fan_speed(void)
{
     T_digit_var *var = &g_digit_vars[MIN_FAN_SPEED_INDEX];
    int fan_speed = u8_encode_fan_speed(var->u8Value);
    return fan_speed;
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

static void digit_init(void)
{
    memset(&g_digit_vars, 0, sizeof(g_digit_vars));

    digit_init_var(DIGIT_PARAM(CUR_FAN_SPEED), // variable info (id, name, index)
                   120,                        // update interval in seconds
                   &u8_decode_FanSpeed,        // JSON decode function
                   &encode_FanSpeed);          // JSON encode function
    digit_init_var(DIGIT_PARAM(OUTSIDE_TEMP), 15, NULL, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(EXHAUST_TEMP), 15, NULL, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(INSIDE_TEMP), 15, NULL, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(INCOMING_TEMP), 15, NULL, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(POST_HEATING_ON_CNT), 5, NULL, &encode_Counter);
    digit_init_var(DIGIT_PARAM(POST_HEATING_OFF_CNT), 5, NULL, &encode_Counter);
    digit_init_var(DIGIT_PARAM(INCOMING_TARGET_TEMP), 200, &u8_decode_Temperature, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(PANEL_LEDS), 20, NULL, &encode_Leds);
    digit_init_var(DIGIT_PARAM(MAX_FAN_SPEED), 200, &u8_decode_FanSpeed, &encode_FanSpeed);
    digit_init_var(DIGIT_PARAM(MIN_FAN_SPEED), 20, &u8_decode_FanSpeed, &encode_FanSpeed);
    digit_init_var(DIGIT_PARAM(HRC_BYPASS_TEMP), 120, &u8_decode_Temperature, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(INPUT_FAN_STOP_TEMP), 120, &u8_decode_Temperature, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(CELL_DEFROSTING_HYSTERESIS), 120, &u8_decode_CellDeFroHyst, &encode_CellDeFroHyst);
    digit_init_var(DIGIT_PARAM(DC_FAN_INPUT), 20, &u8_decode_FanPower, &encode_FanPower);
    digit_init_var(DIGIT_PARAM(DC_FAN_OUTPUT), 20, &u8_decode_FanPower, &encode_FanPower);
    digit_init_var(DIGIT_PARAM(FLAGS_2), 20, NULL, &encode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_4), 20, NULL, &encode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_5), 20, NULL, &encode_BitMap);
    digit_init_var(DIGIT_PARAM(FLAGS_6), 20, NULL, &encode_BitMap);
    digit_init_var(DIGIT_PARAM(RH_MAX), 200, NULL, &encode_RH);
    digit_init_var(DIGIT_PARAM(RH1_SENSOR), 20, NULL, encode_RH);
    digit_init_var(DIGIT_PARAM(BASIC_RH_LEVEL), 120, NULL, encode_RH);
    digit_init_var(DIGIT_PARAM(PRE_HEATING_TEMP), 200, &u8_decode_Temperature, &encode_Temperature);
    digit_init_var(DIGIT_PARAM(IO_GATE_1), 200, NULL, &encode_IO_gate_1);
    digit_init_var(DIGIT_PARAM(IO_GATE_2), 200, NULL, &encode_IO_gate_2);
    digit_init_var(DIGIT_PARAM(IO_GATE_3), 20, NULL, &encode_IO_gate_3);
}

static void digit_init_var(uint8 u8Index, uint8 u8Id, char *sName, time_t tInternal,
                           u8_decode_t decodeFunPtr, encode_t encodeFunPtr)
{
    T_digit_var *digit_var = &g_digit_vars[u8Index];

    digit_var->u8Id = u8Id;
    strcpy(digit_var->sNameStr, sName);
    digit_var->tInternal = tInternal;
    digit_var->decodeFunPtr = decodeFunPtr;
    digit_var->encodeFunPtr = encodeFunPtr;
    digit_var->u8ExpectedMask = 0xFF;
}

static T_digit_var *digit_get_var_by_id(uint8 u8Id)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (u8Id == g_digit_vars[i].u8Id)
        {
            return &g_digit_vars[i];
        }
    }
    return NULL;
}

static T_digit_var *digit_get_var_by_name(char *name)
{
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        if (!strcmp(name, g_digit_vars[i].sNameStr))
        {
            return &g_digit_vars[i];
        }
    }
    return NULL;
}

static void digit_process_msg(uint8 u8Id, uint8 u8Value)
{
    T_digit_var *ptVar = digit_get_var_by_id(u8Id);
    if (ptVar)
    {
        /*
        if (ptVar->bSetOngoing)
        {
            if ((u8Value & ptVar->u8ExpectedMask) == (ptVar->u8ExpectedValue & ptVar->u8ExpectedMask))
            {
                ptVar->bSetOngoing = false;
                ptVar->u32SetReqCnt = 0;
            }
        }
        */
        ptVar->u8Value = u8Value;
        ptVar->tTimestamp = time(NULL);
        if (ptVar->bGetOngoing)
        {
            ptVar->bGetOngoing = false;
            ptVar->u32GetReqCnt = 0;
        }
    }
}

static uint16 u16_digit_calc_crc(uint8 au8Msg[DIGIT_MSG_LEN])
{
    uint16 u16Checksum = 0;
    for (int i = 0; i < (DIGIT_MSG_LEN - 1); i++)
    {
        u16Checksum += au8Msg[i];
    }
    u16Checksum %= 256;

    return u16Checksum;
}

static bool digit_is_valid_msg(uint8 au8Msg[DIGIT_MSG_LEN])
{
    uint16 u16Checksum;

    if (au8Msg[DIGIT_MSG_SYSTEM] != SYSTEM_ID ||
        au8Msg[DIGIT_MSG_SENDER] != DEVICE_ADDRESS)
    {
        return false;
    }

    u16Checksum = u16_digit_calc_crc(au8Msg);

    if (u16Checksum == au8Msg[DIGIT_MSG_CRC])
    {
        return true;
    }
    else
    {
        return false;
    }
}

static void digit_send_msg(uint8 au8Msg[DIGIT_MSG_LEN])
{
    au8Msg[DIGIT_MSG_CRC] = u16_digit_calc_crc(au8Msg);
    rs485_send_msg(DIGIT_MSG_LEN, au8Msg);
    usleep(50000); // sleep 50 ms
}

static void digit_send_get_req(uint8 u8Id)
{
    uint8 au8Msg[DIGIT_MSG_LEN] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, 0, u8Id, 0 };
    digit_send_msg(au8Msg);
}

static void digit_send_set_req(uint8 u8Id, uint8 u8Value)
{
    uint8 au8Msg[DIGIT_MSG_LEN] = { SYSTEM_ID, PI_ADDRESS, DEVICE_ADDRESS, u8Id, u8Value, 0 };
    digit_send_msg(au8Msg);
}

static bool digit_set_change_req(T_digit_var *ptVar, uint8 u8Value)
{
    bool ret = false;
    printf("digit_set_change_req: expected %d, actual %d\n", u8Value, ptVar->u8Value);
    for (int i = 0; i < 100; i++) {
        if ((ptVar->u8Value & ptVar->u8ExpectedMask) != (u8Value & ptVar->u8ExpectedMask)) {
            ptVar->bSetOngoing = true;
            ptVar->u8ExpectedValue = u8Value;
            digit_send_set_req(ptVar->u8Id, u8Value);
            digit_send_get_req(ptVar->u8Id);
        } else {
            printf("set succesfull: cnt %d\n", i);
            ret = true;
            break;
        }
    }
    return true;
    printf("digit_set_ready: %d\n", ptVar->u8Value);
}

static void digit_update_vars()
{
    time_t tCurrentTime = time(NULL);

    // First process ongoing set requests
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *ptVar = &g_digit_vars[i];
        if (ptVar->bSetOngoing == true)
        {
            if ((ptVar->u8Value & ptVar->u8ExpectedMask)  != (ptVar->u8ExpectedValue & ptVar->u8ExpectedMask))
            {
                digit_send_set_req(ptVar->u8Id, ptVar->u8ExpectedValue);
                ptVar->bGetOngoing = true;
                ptVar->u32SetReqCnt++;
            }
            else
            {
                ptVar->bSetOngoing = false;
                ptVar->u32SetReqCnt = 0;
            }
        }
    }
    // Second process ongoing get requests
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *ptVar = &g_digit_vars[i];
        if (ptVar->bGetOngoing == true)
        {
           digit_send_get_req(ptVar->u8Id);
           ptVar->u32GetReqCnt++;
        }
    }
    // Last process get request if interval has expired
    for (int i = 0; i < NUM_OF_DIGIT_VARS; i++)
    {
        T_digit_var *ptVar = &g_digit_vars[i];
        if (ptVar->bGetOngoing == false && tCurrentTime - ptVar->tTimestamp >= ptVar->tInternal)
        {
            ptVar->bGetOngoing = true;
            ptVar->u32GetReqCnt++;
        }
    }
}

static void digit_receive_msgs(void)
{
    uint8 au8RecvMsg[DIGIT_MSG_LEN];

    while(true)
    {
        if (rs485_recv_msg(DIGIT_MSG_LEN, au8RecvMsg, RS485_READ_ATTEMPTS))
        {
            if (digit_is_valid_msg(au8RecvMsg))
            {
#ifdef DEBUG_DIGIT_RECV
                for (int i = 0; i < DIGIT_MSG_LEN; i++)
                {
                    printf("%02X ", au8RecvMsg[i]);

                }
                printf("\n");
#endif
                digit_process_msg(au8RecvMsg[DIGIT_MSG_VARIABLE], au8RecvMsg[DIGIT_MSG_DATA]);
            }
        }
    }
}

static uint8 u8_encode_fan_speed(uint8 u8Value)
{
    uint8 u8FanSpeed = 1;
    for (int i = 8; i >= 1; i--)
    {
        if (u8Value & (0x1 << (i-1)))
        {
            u8FanSpeed = i;
            break;
        }
    }
    return u8FanSpeed;
}

static uint8 u8_decode_Temperature(char *str)
{
    real32 temp;
    sscanf(str, "%f", &temp);
    return u16_celsius_to_NTC(temp);
}

static void encode_Temperature(uint8 value, char *str)
{
    sprintf(str, "%.1f", r32_NTC_to_celsius(value));
}

static uint8 u8_decode_int_FanSpeed(uint8 u8FanSpeed)
{
    uint8 ret = 0;
    for (int i = 0; i < u8FanSpeed; i++)
    {
       ret |= (0x1 << i);
    }
    return ret;
}

static uint8 u8_decode_FanSpeed(char *str)
{
    int fan_speed;
    sscanf(str, "%d", &fan_speed);
    return u8_decode_int_FanSpeed(fan_speed);
}

static void encode_FanSpeed(uint8 value, char *str)
{
    int fan_speed = u8_encode_fan_speed(value);
    sprintf(str, "%d", fan_speed);
}

static void encode_BitMap(uint8 value, char *str)
{
    sprintf(str, "\"%X\"", value);
}

static void encode_IO_gate_1(uint8 value, char *str)
{
    int fan_speed = u8_encode_fan_speed(value);

    strcpy(str, "{");
    json_encode_integer(str,
                        "fan_speed",
                        fan_speed);
    strncat(str, "}", 1);
}

static void encode_IO_gate_2(uint8 value, char *str)
{
    strcpy(str, "{");
    json_encode_integer(str,
                        "post-heating",
                        GET_BIT(value, BIT5));
    strncat(str, "}", 1);
}

static void encode_IO_gate_3(uint8 value, char *str)
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

static void encode_Leds(uint8 value, char *str)
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

static void encode_RH(uint8 value, char *str)
{
    int rh = (value - 51)/2.04;
    sprintf(str, "%d", rh);
}

static void encode_Counter(uint8 value, char *str)
{
    int secs = value/2.5;
    sprintf(str, "%d", secs);
}

static uint8 u8_decode_CellDeFroHyst(char *str)
{
    int hyst;
    sscanf(str, "%d", &hyst);
    return hyst + 2;
}

static void encode_CellDeFroHyst(uint8 value, char *str)
{
    int hysteresis = value - 2;
    sprintf(str, "%d", hysteresis);
}

static uint8 u8_decode_FanPower(char *str)
{
    int fan_power;
    sscanf(str, "%d", &fan_power);
    return fan_power;
}

static void encode_FanPower(uint8 value, char *str)
{
    sprintf(str, "%d", value);
}
