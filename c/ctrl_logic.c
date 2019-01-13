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

#define MOVING_AVERAGE_SIZE             (60 * 60 / CTRL_LOGIC_TIMELEVEL)

#define DEFROST_MAX_DURATION            (15)
#define DEFROST_START_DURATION          (10)
#define DEFROST_TARGET_LEVEL            (72)
#define DEFROST_TARGET_IN_EFF           (85)
#define DEFROST_TARGET_TEMP             (18)
#define DEFROST_STOP_TIME               (10 * 60)

#define SUB_STR_MAX_SIZE                (2000)

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/

typedef enum
{
    e_Measuring,
    e_Defrost_Heating,
    e_Defrost_PreHeating,
    e_Defrost_Stopped,
    e_Defrost_InputFanStop
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
    byte u8DefrostMode;
    uint32 u32DefrostMaxDuration;
    uint32 u32DefrostStartDuration;
    real32 r32DefrostStartLevel;
    real32 r32DefrostTargetInEff;
    real32 r32DefrostTargetTemp;
    real32 r32pressureOut;
    real32 r32pressureIn;
    real32 r32pressureOffset;
    time_t tPressureOut_ts;
    time_t tPressureIn_ts;
    real32 r32pressureDiff;
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

void ctrl_set_var_by_name(char *sName, char *sValue, char *str)
{
    if (!strcmp(sName, "defrost_mode"))
    {
        uint32 u32Temp;
        sscanf(sValue, "%d", &u32Temp);
        g_tCtrlVars.u8DefrostMode = u32Temp;
    }
    else if (!strcmp(sName, "defrost_start_level"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32DefrostStartLevel = fTemp;
    }
    else if (!strcmp(sName, "defrost_target_in_eff"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32DefrostTargetInEff = fTemp;
    }
    else if (!strcmp(sName, "defrost_target_temp"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32DefrostTargetTemp = fTemp;
    }
    else if (!strcmp(sName, "defrost_max_duration"))
    {
        uint32 u32Temp;
        sscanf(sValue, "%d", &u32Temp);
        g_tCtrlVars.u32DefrostMaxDuration = u32Temp;
    }
    else if (!strcmp(sName, "defrost_start_duration"))
    {
        uint32 u32Temp;
        sscanf(sValue, "%d", &u32Temp);
        g_tCtrlVars.u32DefrostStartDuration = u32Temp;
    }
    else if (!strcmp(sName, "pressureOut"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32pressureOut = fTemp;
        g_tCtrlVars.tPressureOut_ts = time(NULL);
    }
    else if (!strcmp(sName, "pressureIn"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32pressureIn = fTemp;
        g_tCtrlVars.tPressureIn_ts = time(NULL);
    }
    else if (!strcmp(sName, "pressure_offset"))
    {
        real32 fTemp;
        sscanf(sValue, "%f", &fTemp);
        g_tCtrlVars.r32pressureOffset = fTemp;
    }
    strcpy(str, "{\"status\": true}");
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

     // defrost_target_in_eff
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32DefrostTargetInEff);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "defrost_target_in_eff",
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

    // pressure out
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32pressureOut);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        g_tCtrlVars.tPressureOut_ts);
    json_encode_object(sSubStr1,
                       "pressure_outdoor",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);

    // pressure in
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32pressureIn);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        g_tCtrlVars.tPressureIn_ts );
    json_encode_object(sSubStr1,
                       "pressure_indoor",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);

    // pressure diff
    real32 r32Diff = g_tCtrlVars.r32pressureIn - g_tCtrlVars.r32pressureOut + g_tCtrlVars.r32pressureOffset;
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      r32Diff);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        min(g_tCtrlVars.tPressureIn_ts, g_tCtrlVars.tPressureOut_ts));
    json_encode_object(sSubStr1,
                       "pressure_diff",
                       sSubStr2);
    strncat(sSubStr1, ",", 1);

    // pressure offset
    strcpy(sSubStr2, "");
    json_encode_real32(sSubStr2,
                      "value",
                      g_tCtrlVars.r32pressureOffset);
    strncat(sSubStr2, ",", 1);
    json_encode_integer(sSubStr2,
                        "ts",
                        time(NULL));
    json_encode_object(sSubStr1,
                       "pressure_offset",
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

    g_tCtrlVars.r32MinExhaustTemp = -3.0f;
    g_tCtrlVars.u8DefrostMode = DEFROST_MODE_OFF;

    g_tCtrlVars.u32DefrostMaxDuration = DEFROST_MAX_DURATION;
    g_tCtrlVars.u32DefrostStartDuration = DEFROST_START_DURATION;
    g_tCtrlVars.r32DefrostStartLevel = DEFROST_TARGET_LEVEL;
    g_tCtrlVars.r32DefrostTargetInEff = DEFROST_TARGET_IN_EFF;
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
            defrost_control();
        }
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

        if (g_tDefrostCtrl.eState == e_Defrost_Heating)
        {
            out_eff = g_tCtrlVars.tOutEff.r32Value;
        }

        avf_filter_calc(&g_tCtrlVars.tInEff, in_eff, g_tCtrlVars.u32CallCnt);
        avf_filter_calc(&g_tCtrlVars.tOutEff, out_eff, g_tCtrlVars.u32CallCnt);
    }
    g_tCtrlVars.u32CallCnt++;
}

static void defrost_control()
{
    if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_ON)
    {
        defrost_resistor_start();
        pre_heating_resistor_start();
        g_tDefrostCtrl.eState = e_Measuring;
    }
    else if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_OFF)
    {
        defrost_resistor_stop();
        pre_heating_resistor_stop();
        g_tDefrostCtrl.eState = e_Measuring;
    }
    else
    {
        real32 r32InEffFiltered = g_tCtrlVars.tInEff.r32Value;
        real32 r32InEff =  g_tCtrlVars.r32InEfficiency;
        time_t tCurrentTime = time(NULL);
        real32 r32CurrentIncomingTemp = r32_digit_incoming_temp();
        real32 r32ExhaustTemp = r32_DS18B20_exhaust_temp();
        real32 r32CurrentExhaustTemp = r32_digit_exhaust_temp();

        if (g_tDefrostCtrl.eState == e_Measuring)
        {
            if (r32InEffFiltered < g_tCtrlVars.r32DefrostStartLevel &&
                r32InEff <  r32InEffFiltered)
            {
                g_tDefrostCtrl.tCheckTime = tCurrentTime;
                g_tDefrostCtrl.eState = e_Defrost_Heating;
				printf("heating started\n");
				printf("target eff %f\n", g_tCtrlVars.r32DefrostTargetInEff);
				printf("target temp %f\n", g_tCtrlVars.r32DefrostTargetTemp);
				
            }
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_Heating ||
                g_tDefrostCtrl.eState == e_Defrost_PreHeating)
        {
            if (r32ExhaustTemp > 18)
            {
                g_tDefrostCtrl.eState = e_Defrost_PreHeating;
            }

            if (r32InEff > g_tCtrlVars.r32DefrostTargetInEff ||
                r32CurrentIncomingTemp > g_tCtrlVars.r32DefrostTargetTemp)
            {
                g_tDefrostCtrl.tCheckTime = tCurrentTime;
                g_tDefrostCtrl.eState = e_Defrost_Stopped;
            }
            else if (tCurrentTime - g_tDefrostCtrl.tCheckTime > (g_tCtrlVars.u32DefrostMaxDuration * 60))
            {
                digit_set_input_fan_stop(14.0f);
                g_tDefrostCtrl.eState = e_Defrost_InputFanStop;
				printf("input fan stopped\n");
            }
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_InputFanStop)
        {
            if (r32InEff > g_tCtrlVars.r32DefrostTargetInEff ||
                r32CurrentIncomingTemp > g_tCtrlVars.r32DefrostTargetTemp)
            {
                digit_set_input_fan_stop(-6.0f);
                g_tDefrostCtrl.tCheckTime = tCurrentTime;
                g_tDefrostCtrl.eState = e_Defrost_Stopped;
				printf("current eff %f\n", r32InEff);
				printf("current temp %f\n", r32CurrentIncomingTemp);
				printf("heating stopped\n");
            }
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_Stopped)
        {
            if (tCurrentTime - g_tDefrostCtrl.tCheckTime > DEFROST_STOP_TIME)
            {
                g_tDefrostCtrl.eState = e_Measuring;
            }
        }

        if (g_tDefrostCtrl.eState == e_Defrost_Heating)
        {
            defrost_resistor_start();
            pre_heating_resistor_start();
        }
        else if (g_tDefrostCtrl.eState == e_Defrost_PreHeating)
        {
            defrost_resistor_stop();
            pre_heating_resistor_start();
        }
        else
        {
            defrost_resistor_stop();
            pre_heating_resistor_stop();
        }
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
