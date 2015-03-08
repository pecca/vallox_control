/**
 * @file   ctrl_logic.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of control logic.
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/
 
#include "common.h"
#include "ctrl_logic.h"
#include "digit_protocol.h"
#include "post_heating_counter.h"
#include "pre_heating_resistor.h"
#include "defrost_resistor.h"
#include "DS18B20.h"
#include "json_codecs.h"

/******************************************************************************
 *  Constants
 ******************************************************************************/
 
#define DEFROST_MODE_OFF                (0)
#define DEFROST_MODE_ON                 (1)
#define DEFROST_MODE_AUTO               (2)

#define PRE_HEATING_EXHAUST_TEMP_MAX    (3.0f)
#define PRE_HEATING_MODE_OFF            (0)
#define PRE_HEATING_MODE_ON             (1)
#define PRE_HEATING_MODE_AUTO           (2)

#define MOVING_AVERAGE_SIZE             (60 * 60 / CTRL_LOGIC_TIMELEVEL)

#define DEFROST_MAX_DURATION            (15)
#define DEFROST_START_DURATION          (10)
#define DEFROST_TARGET_LEVEL            (72)
#define DEFROST_TARGET_TEMP             (17)
#define DEFROST_STOP_TIME               (10 * 60)
 
#define SUB_STR_MAX_SIZE                (1000)

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/
 
typedef enum
{
    e_Measuring,
    e_Below_Limit,
    e_Defrost_Ongoing,
    e_Defrost_Stopped
} E_DefrostState;

typedef struct
{
    real32 ar32Table[MOVING_AVERAGE_SIZE];
    real64 r64Sum;
    real32 r32Value;
} T_AvfFilter;

typedef struct
{
    uint32 u32CallCnt;
    real32 r32MinExhaustTemp;
    real32 r32DewPoint;
    real32 r32InEfficiency;
    real32 r32OutEfficiency;
    T_AvfFilter tInEff;
    T_AvfFilter tOutEff;
    byte u8PreHeatingMode;    
    byte u8DefrostMode;
    uint32 u32DefrostMaxDuration;
    uint32 u32DefrostStartDuration;
    real32 r32DefrostStartLevel;
    real32 r32DefrostTargetTemp;
} T_CtrlVars;

typedef struct
{
    bool bActive;
    uint16 u16CalcPower;
    uint16 u16CalcPowerOffset;
    time_t tCheckInterval;
    time_t tCheckTime;
    time_t tStartTime;
    real32 r32ExhaustTempDiffPrev;
} T_PreHeating;

typedef struct
{
    E_DefrostState eState;
    time_t tCheckTime;
    uint16 u16PreHeatingPower;
    real32 r32InEffPrev;
} T_Defrost; 
 
/******************************************************************************
 *  Local variables
 ******************************************************************************/
 
static T_PreHeating g_tPreHeating;
static T_Defrost g_tDefrostCtrl;
static T_CtrlVars g_tCtrlVars;

/******************************************************************************
 *  Local function declarations
 ******************************************************************************/

static void ctrl_init(void);
static void ctrl_run();
static void pre_heating_control(void);
static uint16 u16_pre_heating_calc_power(real32 r32ExhaustTargetTemp);
static void defrost_control(void);
static void ctrl_update_vars(void);
static void calc_in_out_effiency(real32 *pr32InEff, real32 *pr32OutEff);
static real32 r32_calc_dew_point(void);
static void avf_filter_init(T_AvfFilter *tFilter, real32 r32Value);
static void avf_filter_calc(T_AvfFilter *tFilter, real32 r32NewValue, uint32 i32index);

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/
 
void *ctrl_logic_thread(void *ptr)
{
    ctrl_init();
    while(1)
    {
        ctrl_run();
        sleep(CTRL_LOGIC_TIMELEVEL);
    }
    return NULL;
}

void ctrl_set_var_by_name(char *sName, char *sValue)
{
    if (!strcmp(sName, "pre_heating_power"))
    {
        uint16 u16Temp;
        sscanf(sValue, "%d", &u16Temp);
        pre_heating_set_power(u16Temp);
    }
    else if (!strcmp(sName, "pre_heating_mode"))
    {
        uint16 u16Temp;
        sscanf(sValue, "%d", &u16Temp);
        g_tCtrlVars.u8PreHeatingMode = u16Temp;
    }
    else if (!strcmp(sName, "defrost_mode"))
    {
        uint16 u16Temp;
        sscanf(sValue, "%d", &u16Temp);
        g_tCtrlVars.u8DefrostMode = u16Temp;
    }
    else if (!strcmp(sName, "min_exhaust_temp"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32MinExhaustTemp = fTemp;
    }
    else if (!strcmp(sName, "defrost_start_level"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32DefrostStartLevel = fTemp;
    }
    else if (!strcmp(sName, "defrost_target_temp"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32DefrostTargetTemp = fTemp;
    }  
    else if (!strcmp(sName, "defrost_max_duration"))
    {
        uint16 u16Temp;
        sscanf(sValue, "%d", &u16Temp);
        g_tCtrlVars.u32DefrostMaxDuration = u16Temp;
    }
    else if (!strcmp(sName, "defrost_start_duration"))
    {
        uint16 u16Temp;
        sscanf(sValue, "%d", &u16Temp);
        g_tCtrlVars.u32DefrostStartDuration = u16Temp;
    }     
}

void ctrl_json_encode(char *sMesg)
{
    char sSubStr1[SUB_STR_MAX_SIZE];
    char sSubStr2[SUB_STR_MAX_SIZE];
    
    strcpy(sMesg, "{");
    strcpy(sSubStr1, "");
    strcpy(sSubStr2, "");
    
    // pre_heating_time
    json_encode_real32(sSubStr2,
                      "value",
                      u32_pre_heating_resistor_get_on_time_total());
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "pre_heating_time",
                       sSubStr2);
    strncat(sSubStr1, ",", 1); 

    // post_heating_time
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      u32_post_heating_counter_get_on_time_total());
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "post_heating_time",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);  
    
    // defrost_time total
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      u32_defrost_resistor_get_on_time_total());
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_time",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);      

    // defrost_time
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      u32_defrost_resistor_get_on_time());
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_on_time",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);  
    
    // defrost_mode
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.u8DefrostMode);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_mode",
                       sSubStr2);
    strncat(sSubStr1, ",", 1); 
   
    // pre_heating_mode
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.u8PreHeatingMode);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "pre_heating_mode",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);   
  
     // pre_heating_power
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      u16_pre_heating_get_power());
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "pre_heating_power",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);   
  
     // dew_point
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32DewPoint);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "dew_point",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);   
 
     // dew_point
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32MinExhaustTemp);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "min_exhaust_temp",
                       sSubStr2);
    strncat(sSubStr1, ",", 1); 
 
     // in_efficiency
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32InEfficiency);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "in_efficiency",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);   
  
     // out_efficiency
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32OutEfficiency);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "out_efficiency",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);                   

     // in_efficiency filtered
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.tInEff.r32Value);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "in_efficiency_filtered",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);   
  
     // out_efficiency filtered
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.tOutEff.r32Value);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "out_efficiency_filtered",
                       sSubStr2);                       
    strncat(sSubStr1, ",", 1);                   

     // defrost_start_level
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32DefrostStartLevel);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_start_level",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);  

     // defrost_target_temp
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32DefrostTargetTemp);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_target_temp",
                       sSubStr2);
    strncat(sSubStr1, ",", 1); 
    
    // defrost_max_duration
    strcpy(sSubStr2, "");
    json_encode_integer(sSubStr2,
                      "value",
                      g_tCtrlVars.u32DefrostMaxDuration);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_max_duration",
                       sSubStr2);
    strncat(sSubStr1, ",", 1); 
  
    // defrost_start_duration
    strcpy(sSubStr2, "");
    json_encode_integer(sSubStr2,
                      "value",
                      g_tCtrlVars.u32DefrostStartDuration);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_start_duration",
                       sSubStr2);
     
    json_encode_object(sMesg,
                       CONTROL_VARS,
                       sSubStr1);
    strncat(sMesg, "}", 1);
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

static void ctrl_init()
{
    memset(&g_tCtrlVars, 0x0, sizeof(g_tCtrlVars));
    memset(&g_tPreHeating, 0x0, sizeof(g_tPreHeating));
    memset(&g_tDefrostCtrl, 0x0, sizeof(g_tDefrostCtrl));
    
    g_tDefrostCtrl.u16PreHeatingPower = 1000;
    g_tCtrlVars.r32MinExhaustTemp = -3.0f;
    g_tCtrlVars.u8DefrostMode = DEFROST_MODE_OFF;
    g_tCtrlVars.u8PreHeatingMode = PRE_HEATING_MODE_OFF;
    pre_heating_set_power(0);
    
    g_tCtrlVars.u32DefrostMaxDuration = DEFROST_MAX_DURATION;
    g_tCtrlVars.u32DefrostStartDuration = DEFROST_START_DURATION;
    g_tCtrlVars.r32DefrostStartLevel = DEFROST_TARGET_LEVEL;
    g_tCtrlVars.r32DefrostTargetTemp = DEFROST_TARGET_TEMP;
    
    post_heating_counter_init();
    pre_heating_resistor_init();
    defrost_resistor_init();
}

static void ctrl_run()
{
    post_heating_counter_update();
    pre_heating_resistor_counter_update();
    defrost_resistor_counter_update();
    
    if (digit_vars_ok() && DS18B20_vars_ok())
    {
        ctrl_update_vars();
        
        if (g_tCtrlVars.u32CallCnt % 2)
        {
            pre_heating_control();
            defrost_control();
        }
    }
    else
    {
        ctrl_init();
        pre_heating_set_power(0);
        defrost_resistor_stop();
    }
}

static void ctrl_update_vars()
{
    g_tCtrlVars.r32DewPoint = r32_calc_dew_point();
    calc_in_out_effiency(&g_tCtrlVars.r32InEfficiency, &g_tCtrlVars.r32OutEfficiency);
    if (!g_tCtrlVars.u32CallCnt)
    {
        avf_filter_init(&g_tCtrlVars.tInEff, g_tCtrlVars.r32InEfficiency);
        avf_filter_init(&g_tCtrlVars.tOutEff, g_tCtrlVars.r32OutEfficiency);   
    }
    else
    {
        real32 in_eff = g_tCtrlVars.r32InEfficiency;
        real32 out_eff = g_tCtrlVars.r32OutEfficiency;        
        
        if (g_tDefrostCtrl.eState == e_Defrost_Ongoing)
        {
            in_eff = g_tCtrlVars.tInEff.r32Value;
            out_eff = g_tCtrlVars.tOutEff.r32Value;
        }
    
        avf_filter_calc(&g_tCtrlVars.tInEff, in_eff, g_tCtrlVars.u32CallCnt);
        avf_filter_calc(&g_tCtrlVars.tOutEff, out_eff, g_tCtrlVars.u32CallCnt);
    }
    g_tCtrlVars.u32CallCnt++;
} 
 
static void pre_heating_control(void)
{
    if (g_tCtrlVars.u8PreHeatingMode == PRE_HEATING_MODE_ON)
    {
        if (!g_tPreHeating.bActive)
        {
            g_tPreHeating.bActive = true;
            g_tPreHeating.tCheckTime = time(NULL);
        }
    }
    else if (g_tCtrlVars.u8PreHeatingMode == PRE_HEATING_MODE_AUTO)
    {
        real32 r32CurrentExhaustTemp = r32_digit_exhaust_temp();
        real32 r32TargetExhaustTemp = max(g_tCtrlVars.r32DewPoint, g_tCtrlVars.r32MinExhaustTemp);
        uint16 u16Power = 0;
        
       
        if (r32TargetExhaustTemp > PRE_HEATING_EXHAUST_TEMP_MAX)
        {
            r32TargetExhaustTemp = PRE_HEATING_EXHAUST_TEMP_MAX;
        }

        real32 r32ExhaustTempDiff = r32TargetExhaustTemp - r32CurrentExhaustTemp; 
        real32 r32ExhaustTempDerivate = g_tPreHeating.r32ExhaustTempDiffPrev - r32ExhaustTempDiff;
        
        uint16 u16CalcPower = u16_pre_heating_calc_power(r32TargetExhaustTemp);
        
        g_tPreHeating.r32ExhaustTempDiffPrev = r32ExhaustTempDiff;
        
   
        if (r32ExhaustTempDiff > 0 && r32ExhaustTempDerivate < 0)
        {
            u16Power = 1500;
        }
        else if (r32ExhaustTempDiff > 4.0f)
        {
            u16Power = 1500;
        }
        else if (r32ExhaustTempDiff > 3.0f)
        {
            u16Power = 1000;
        }
        else if (r32ExhaustTempDiff > 2.0f)
        {
            u16Power = u16CalcPower * 2;
        }          
        else if (r32ExhaustTempDiff > 1.0f)
        {
            u16Power = u16CalcPower * 1.5f;
        }       
        else if (r32ExhaustTempDiff > 0)
        {
            u16Power = u16CalcPower;
        }
        else if (r32ExhaustTempDiff > -1.0f)
        {
            u16Power = u16CalcPower / 2;
        }
        else if (r32ExhaustTempDiff > -1.5f)
        {
            u16Power  = u16CalcPower / 3;
        }
        else 
        {
            u16Power  = 0;
        }
        
        if (g_tDefrostCtrl.eState == e_Defrost_Ongoing)
        {
            pre_heating_set_power(g_tDefrostCtrl.u16PreHeatingPower);
        }
        else
        {
            pre_heating_set_power(u16Power);
        }
    }
    else
    {
        pre_heating_set_power(0);
    }
}

static uint16 u16_pre_heating_calc_power(real32 r32ExhaustTargetTemp)
{
    real32 r32InsideTemp = r32_digit_inside_temp();
    
    real32 r32TargetIncomingTemp = r32InsideTemp - ((r32InsideTemp - r32ExhaustTargetTemp) / 0.8f);
    real32 r32TempDiff = r32TargetIncomingTemp - r32_digit_outside_temp();
    
    uint16 u16AirFlow = 15 + 10 * u8_digit_cur_fan_speed();
    real32 r32PreHeatingPower = (u16AirFlow * 1.225 * r32TempDiff);

    int32 i32Ret = ceil(r32PreHeatingPower / 100.0f) * 100;
    if (i32Ret < 0)
        i32Ret = 0;
    
    return (uint16)i32Ret;
}

static void defrost_control()
{    
    if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_ON)
    {
        defrost_resistor_start();
        g_tDefrostCtrl.eState = e_Measuring;
    }        
    else if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_OFF)
    {
        defrost_resistor_stop();
        g_tDefrostCtrl.eState = e_Measuring;
    }
    else
    {
        real32 r32InEff = g_tCtrlVars.tInEff.r32Value;
        time_t tCurrentTime = time(NULL);
                
        if (g_tDefrostCtrl.eState == e_Measuring ||
            g_tDefrostCtrl.eState == e_Below_Limit)
        {  
            if (r32InEff < g_tCtrlVars.r32DefrostStartLevel)
            {
                if (g_tDefrostCtrl.eState == e_Measuring)
                {
                    g_tDefrostCtrl.tCheckTime = tCurrentTime;
                    g_tDefrostCtrl.eState = e_Below_Limit;
                }
                else if (tCurrentTime - g_tDefrostCtrl.tCheckTime > 
                           (g_tCtrlVars.u32DefrostStartDuration * 60))
                {
                    g_tDefrostCtrl.tCheckTime = tCurrentTime;
                    g_tDefrostCtrl.eState = e_Defrost_Ongoing;
                }
            }
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_Ongoing)
        {
            if (r32_DS18B20_incoming_temp() > g_tCtrlVars.r32DefrostTargetTemp ||
                tCurrentTime - g_tDefrostCtrl.tCheckTime > (g_tCtrlVars.u32DefrostMaxDuration * 60))
            {
                g_tDefrostCtrl.tCheckTime = tCurrentTime;
                g_tDefrostCtrl.eState = e_Defrost_Stopped;
            }
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_Stopped)
        {
            if (tCurrentTime - g_tDefrostCtrl.tCheckTime > DEFROST_STOP_TIME)
            {
                g_tDefrostCtrl.eState = e_Measuring;
            }
        }
        
        if (g_tDefrostCtrl.eState == e_Defrost_Ongoing)
        {
            defrost_resistor_start();
        }
        else
        {
            defrost_resistor_stop();
        }
        g_tDefrostCtrl.r32InEffPrev = r32InEff;
    }
}

static void calc_in_out_effiency(real32 *pr32InEff, real32 *pr32OutEff)
{
    real32 r32IncomingTemp = r32_DS18B20_incoming_temp();
    real32 r32OutsideTemp = r32_DS18B20_outside_temp();
    real32 r32InsideTemp = r32_digit_inside_temp();
    real32 r32ExhaustTemp = r32_digit_exhaust_temp();

    real32 r32IncomingEff =  ((r32IncomingTemp - r32OutsideTemp) /
                           (r32InsideTemp - r32OutsideTemp)) * 100.0f;

    real32 r32OutcomingEff = ((r32InsideTemp - r32ExhaustTemp) /
                           (r32InsideTemp - r32OutsideTemp)) * 100.0f;   

    if (r32IncomingEff > 100.0f)
    {
        r32IncomingEff = 100.0f;
    }
    else if (r32IncomingEff < 0.0f)
    {
        r32IncomingEff = 0.0f;
    }

    if (r32OutcomingEff > 100.0f)
    {
        r32OutcomingEff = 100.0f;
    }
    else if (r32OutcomingEff < 0.0f)
    {
        r32OutcomingEff = 0.0f;
    }
    *pr32InEff = r32IncomingEff;
    *pr32OutEff = r32OutcomingEff;
}

static real32 r32_calc_dew_point(void)
{
    real32 inside_temp = r32_digit_inside_temp();
    real32 r32Rh = r32_digit_rh1_sensor() / 100.0f;
    real32 tempA = 17.27f;
    real32 tempB = 237.7f;
    real32 tempZ =  (((tempA * inside_temp) / (tempB + inside_temp)) + log(r32Rh));
    real32 dew_point = ((tempB * tempZ) / (tempA - tempZ));
    return dew_point;
}

static void avf_filter_init(T_AvfFilter *tFilter, real32 r32Value)
{
    tFilter->r64Sum = 0;
    for (int i = 0; i < MOVING_AVERAGE_SIZE; i++)
    {
        tFilter->ar32Table[i] = r32Value;
        tFilter->r64Sum += r32Value;
    }
    tFilter->r32Value = r32Value;
}

static void avf_filter_calc(T_AvfFilter *tFilter, real32 r32NewValue, uint32 i32index)
{
    real32 r32OldValue = tFilter->ar32Table[i32index % MOVING_AVERAGE_SIZE];
    tFilter->r64Sum -= r32OldValue;
    tFilter->ar32Table[i32index % MOVING_AVERAGE_SIZE] = r32NewValue;
    tFilter->r64Sum += r32NewValue;
    tFilter->r32Value = tFilter->r64Sum / MOVING_AVERAGE_SIZE;
}
