/**
 * @file   ctrl_logic.c
 * @Author Pekka Mäkelä (pekka.makela@iki.fi)
 * @brief  Implementation of control logic.
 */

/******************************************************************************
 *  Includes
 ******************************************************************************/

#include "ctrl_logic.h"
#include "DS18B20.h"
#include "common.h"
#include "defrost_resistor.h"
#include "digit_protocol.h"
#include "json_codecs.h"
#include "post_heating_counter.h"
#include "pre_heating_resistor.h"

/******************************************************************************
 *  Constants
 ******************************************************************************/

#define DEFROST_MODE_OFF (0)
#define DEFROST_MODE_ON (1)
#define DEFROST_MODE_AUTO (2)
#define DEFROST_MODE_AI (3)

#define MOVING_AVERAGE_SIZE (60 * 60 / CTRL_LOGIC_TIMELEVEL)

#define DEFROST_TARGET_LEVEL (72)
#define DEFROST_STOP_TIME (10 * 60)
#define DEFROST_HEATING_SAFETY_TIMEOUT                                         \
  (30 * 60) /* max heating duration before emergency shutoff */

#define DEFROST_EXHAUST_MIN_TEMP                                               \
  (3.0f) /* both exhaust sensors must exceed this to end heating */
#define DEFROST_EFF_SAMPLE_INTERVAL                                            \
  (30) /* seconds between efficiency samples for trend detection */
#define DEFROST_EFF_IMPROVEMENT_THRESH                                         \
  (0.5f) /* min % improvement to count as improving */
#define DEFROST_HEATING_NO_IMPROV_TIME                                         \
  (3 * 60) /* 3 min no-improvement -> heating done (if exhaust > 5C) */
#define DEFROST_FANSTOP_NO_IMPROV_TIME                                         \
  (5 * 60) /* 5 min no-improvement -> fan stop done */
#define DEFROST_FANSTOP_MAX_DURATION                                           \
  (45 * 60) /* 45 min max fan stop duration (increased for one-phase) */
#define DEFROST_STOP_DURATION (30 * 60) /* 30 min cooldown after defrost */

#define CYCLE_COUNT_FILE "defrost_cycle_count.txt"

#define SUB_STR_MAX_SIZE (5000)

/******************************************************************************
 *  Data type declarations
 ******************************************************************************/

typedef enum {
  e_Measuring,
  e_Defrost_Heating,
  e_Defrost_Stopped,
  e_Defrost_InputFanStop
} E_DefrostState;

typedef enum {
  e_EndReason_None,
  e_EndReason_EffRecovered,
  e_EndReason_TempTarget,
  e_EndReason_FanStopResolved,
  e_EndReason_Timeout,
  e_EndReason_SafetyShutoff,
  e_EndReason_EffPlateau, /* heating ended: eff plateaued AND both exhaust > 5C
                           */
  e_EndReason_FanStopPlateau,   /* fan stop ended: eff plateaued */
  e_EndReason_StopStateComplete /* stop state completed */
} E_DefrostEndReason;

typedef struct {
  real32 ar32Table[MOVING_AVERAGE_SIZE];
  real64 r64Sum;
  real32 r32Value;
} T_AvfFilter;

typedef struct {
  uint32 u32CallCnt;
  real32 r32MinExhaustTemp;
  real32 r32DewPoint;
  real32 r32InEfficiency;
  real32 r32OutEfficiency;
  T_AvfFilter tInEff;
  T_AvfFilter tOutEff;
  byte u8DefrostMode;
  real32 r32DefrostStartLevel;
  real32 r32pressureOut;
  real32 r32pressureIn;
  real32 r32pressureOffset;
  time_t tPressureOut_ts;
  time_t tPressureIn_ts;
  real32 r32pressureDiff;
  real32 r32EffImpThresh;       // % improvement to count as improving
  real32 r32DefrostTargetLevel; // target efficiency to reach before stopping
                                // defrost
  uint32 u32FanStopNoImpTime;   // seconds of no improvement to end fan stop
  uint32 u32FanStopMaxDur;      // max fan stop duration in seconds
  uint32 u32StopDuration;       // cooldown duration after defrost
} T_CtrlVars;

typedef struct {
  bool bActive;
  uint16 u16CalcPower;
  uint16 u16CalcPowerOffset;
  time_t tCheckInterval;
  time_t tCheckTime;
  time_t tStartTime;
  real32 r32ExhaustTempDiffPrev;
} T_PreHeating;

typedef struct {
  E_DefrostState eState;
  time_t tCheckTime;
  /* Cycle timing */
  time_t tCycleStart;
  time_t tHeatingEnd;
  time_t tFanStopStart;
  time_t tCycleEnd;
  time_t tStopStateStart; /* when the stop state (cooldown) began */
  uint32 u32CycleCount;

  /* Cycle Data Snapshots */
  struct {
    real32 r32InEff;
    real32 r32InEffFiltered;
    real32 r32OutsideTemp;

    real32 r32IncomingTemp;
    real32 r32Rh; /* Captured humidity */
    real32 r32DewPoint;
    /* DS18B20 sensors */
    real32 r32DsOutsideTemp;
    real32 r32DsExhaustTemp;
    real32 r32DsIncomingTemp;
  } tStart;

  struct {
    real32 r32InEff;
    real32 r32IncomingTemp;

    E_DefrostEndReason eReason;
    /* DS18B20 sensors */
    real32 r32DsOutsideTemp;
    real32 r32DsExhaustTemp;
    real32 r32DsIncomingTemp;
  } tEnd;

  /* AI control commands */
  byte u8AiHeating;  /* 0=off, 1=on */
  byte u8AiFanStop;  /* 0=off, 1=on */
  time_t tAiLastCmd; /* timestamp of last AI command */
  /* Safety */
  time_t
      tHeatingOnSince; /* when defrost resistor was last turned on (0 = off) */
  /* Efficiency improvement tracking (improved AUTO mode) */
  real32 r32PrevEffSample;     /* last sampled efficiency for trend detection */
  time_t tLastEffSampleTime;   /* when we last sampled efficiency */
  time_t tLastImprovementTime; /* when efficiency last showed improvement */
  real32 r32EffAtPhaseStart;   /* efficiency when current phase began */
} T_Defrost;

typedef struct {
  bool bActive;
  time_t tStartTime;
  uint32 u32MaxTimeSec; /* auto-disable after this many seconds */
} T_Fireplace;

/******************************************************************************
 *  Local variables
 ******************************************************************************/

static T_PreHeating g_tPreHeating;
static T_Defrost g_tDefrostCtrl;
static T_CtrlVars g_tCtrlVars;
static T_Fireplace g_tFireplace;

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
static void avf_filter_calc(T_AvfFilter *tFilter, real32 r32NewValue,
                            uint32 i32index);
static void input_fan_stop(void);
static void input_fan_start(void);
static void load_cycle_count(void);
static void input_fan_start(void);
static void load_cycle_count(void);
static void save_cycle_count(void);
static void capture_cycle_start_data(void);
static void capture_cycle_end_data(E_DefrostEndReason eReason);

/******************************************************************************
 *  Global function implementation
 ******************************************************************************/

void *ctrl_logic_thread(void *ptr) {
  ctrl_init();
  while (1) {
    ctrl_run();
    sleep(CTRL_LOGIC_TIMELEVEL);
  }
  return NULL;
}

void ctrl_set_var_by_name(char *sName, char *sValue, char *str) {
  if (!strcmp(sName, "defrost_mode")) {
    uint32 u32Temp;
    sscanf(sValue, "%d", &u32Temp);
    g_tCtrlVars.u8DefrostMode = u32Temp;
  } else if (!strcmp(sName, "defrost_start_level")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32DefrostStartLevel = fTemp;
  } else if (!strcmp(sName, "defrost_target_level")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32DefrostTargetLevel = fTemp;
  } else if (!strcmp(sName, "min_exhaust_temp")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32MinExhaustTemp = fTemp;
  } else if (!strcmp(sName, "pressureOut")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32pressureOut = fTemp;
    g_tCtrlVars.tPressureOut_ts = time(NULL);
  } else if (!strcmp(sName, "pressureIn")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32pressureIn = fTemp;
    g_tCtrlVars.tPressureIn_ts = time(NULL);
  } else if (!strcmp(sName, "pressure_offset")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32pressureOffset = fTemp;
  } else if (!strcmp(sName, "ai_defrost_heating")) {
    uint32 u32Temp;
    sscanf(sValue, "%d", &u32Temp);
    g_tDefrostCtrl.u8AiHeating = (u32Temp != 0) ? 1 : 0;
    g_tDefrostCtrl.tAiLastCmd = time(NULL);
  } else if (!strcmp(sName, "ai_defrost_fan_stop")) {
    uint32 u32Temp;
    sscanf(sValue, "%d", &u32Temp);
    g_tDefrostCtrl.u8AiFanStop = (u32Temp != 0) ? 1 : 0;
    g_tDefrostCtrl.tAiLastCmd = time(NULL);
  } else if (!strcmp(sName, "fireplace_on")) {
    uint32 u32MaxTime;
    sscanf(sValue, "%d", &u32MaxTime);
    if (u32MaxTime > 0) {
      g_tFireplace.bActive = true;
      g_tFireplace.tStartTime = time(NULL);
      g_tFireplace.u32MaxTimeSec = u32MaxTime;
      printf("[Fireplace] activated, max time %d seconds\n", u32MaxTime);
    }
  } else if (!strcmp(sName, "fireplace_off")) {
    g_tFireplace.bActive = false;
    printf("[Fireplace] deactivated\n");
  } else if (!strcmp(sName, "defrost_eff_imp_thresh")) {
    real32 fTemp;
    sscanf(sValue, "%f", &fTemp);
    g_tCtrlVars.r32EffImpThresh = fTemp;
  } else if (!strcmp(sName, "defrost_fan_stop_no_imp_time")) {
    int32 i32Temp;
    sscanf(sValue, "%d", &i32Temp);
    if (i32Temp >= 1) {
      g_tCtrlVars.u32FanStopNoImpTime = i32Temp;
    }
  } else if (!strcmp(sName, "defrost_fan_stop_max_dur")) {
    int32 i32Temp;
    sscanf(sValue, "%d", &i32Temp);
    g_tCtrlVars.u32FanStopMaxDur = i32Temp;
  } else if (!strcmp(sName, "defrost_stop_duration")) {
    int32 i32Temp;
    sscanf(sValue, "%d", &i32Temp);
    g_tCtrlVars.u32StopDuration = i32Temp;
  }
  strcpy(str, "{\"status\": true}");
}

void ctrl_json_encode(char *sMesg) {
  char sSubStr1[SUB_STR_MAX_SIZE];
  char sSubStr2[SUB_STR_MAX_SIZE];

  strcpy(sMesg, "{");
  strcpy(sSubStr1, "");
  strcpy(sSubStr2, "");

  // pre_heating_time
  json_encode_real32(sSubStr2, "value",
                     u32_pre_heating_resistor_get_on_time_total());
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "pre_heating_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // post_heating_time
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value",
                     u32_post_heating_counter_get_on_time_total());
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "post_heating_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_time total
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value",
                     u32_defrost_resistor_get_on_time_total());
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_total_duration
  strcpy(sSubStr2, "");
  uint32 total_dur = 0;
  if (g_tDefrostCtrl.tCycleEnd > g_tDefrostCtrl.tCycleStart) {
    total_dur = g_tDefrostCtrl.tCycleEnd - g_tDefrostCtrl.tCycleStart;
  }
  json_encode_real32(sSubStr2, "value", total_dur);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_total_duration", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_time
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", u32_defrost_resistor_get_on_time());
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_on_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_mode
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.u8DefrostMode);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_mode", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // dew_point
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32DewPoint);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "dew_point", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // dew_point
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32MinExhaustTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "min_exhaust_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // in_efficiency
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32InEfficiency);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "in_efficiency", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // fireplace_mode
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tFireplace.bActive ? 1 : 0);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "fireplace_mode", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // out_efficiency
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32OutEfficiency);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "out_efficiency", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // in_efficiency filtered
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.tInEff.r32Value);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "in_efficiency_filtered", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // out_efficiency filtered
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.tOutEff.r32Value);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "out_efficiency_filtered", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_level
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32DefrostStartLevel);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_start_level", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_target_level
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32DefrostTargetLevel);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_target_level", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // pressure out
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32pressureOut);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tCtrlVars.tPressureOut_ts);
  json_encode_object(sSubStr1, "pressure_outdoor", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // pressure in
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32pressureIn);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tCtrlVars.tPressureIn_ts);
  json_encode_object(sSubStr1, "pressure_indoor", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // pressure diff
  real32 r32Diff = g_tCtrlVars.r32pressureIn - g_tCtrlVars.r32pressureOut +
                   g_tCtrlVars.r32pressureOffset;
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", r32Diff);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(
      sSubStr2, "ts",
      min(g_tCtrlVars.tPressureIn_ts, g_tCtrlVars.tPressureOut_ts));
  json_encode_object(sSubStr1, "pressure_diff", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // pressure offset
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32pressureOffset);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "pressure_offset", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_state
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.eState);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_state", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_cycle_count
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.u32CycleCount);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_cycle_count", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_cycle_start
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.tCycleStart);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_cycle_start", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_heating_duration (seconds)
  strcpy(sSubStr2, "");
  json_encode_integer(
      sSubStr2, "value",
      g_tDefrostCtrl.tHeatingEnd > 0
          ? (g_tDefrostCtrl.tHeatingEnd - g_tDefrostCtrl.tCycleStart)
          : 0);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_heating_duration", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_fan_stop_duration (seconds)
  strcpy(sSubStr2, "");
  json_encode_integer(
      sSubStr2, "value",
      (g_tDefrostCtrl.tFanStopStart > 0 && g_tDefrostCtrl.tCycleEnd > 0)
          ? (g_tDefrostCtrl.tCycleEnd - g_tDefrostCtrl.tFanStopStart)
          : 0);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tFanStopStart);
  json_encode_object(sSubStr1, "defrost_fan_stop_duration", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_total_duration (seconds)
  strcpy(sSubStr2, "");
  json_encode_integer(
      sSubStr2, "value",
      (g_tDefrostCtrl.tCycleStart > 0 && g_tDefrostCtrl.tCycleEnd > 0)
          ? (g_tDefrostCtrl.tCycleEnd - g_tDefrostCtrl.tCycleStart)
          : 0);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleEnd);
  json_encode_object(sSubStr1, "defrost_total_duration", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_ineff
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32InEff);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_ineff", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_ineff_filtered
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32InEffFiltered);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_ineff_filtered", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_outside_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32OutsideTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_outside_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_incoming_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32IncomingTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_incoming_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_dew_point
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32DewPoint);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_dew_point", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_ds_outside_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32DsOutsideTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_ds_outside_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_ds_exhaust_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32DsExhaustTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_ds_exhaust_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_ds_incoming_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value",
                     g_tDefrostCtrl.tStart.r32DsIncomingTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_ds_incoming_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_start_rh
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tStart.r32Rh);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tCycleStart);
  json_encode_object(sSubStr1, "defrost_start_rh", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_in_eff
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tEnd.r32InEff);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_in_eff", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_incoming_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tEnd.r32IncomingTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_incoming_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_ds_outside_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tEnd.r32DsOutsideTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_ds_outside_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_ds_exhaust_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tEnd.r32DsExhaustTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_ds_exhaust_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_ds_incoming_temp
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.tEnd.r32DsIncomingTemp);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_ds_incoming_temp", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_end_reason (0=none, 1=eff_recovered, 2=temp_target,
  // 3=fan_stop_resolved, 4=timeout)
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.tEnd.eReason);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tHeatingEnd);
  json_encode_object(sSubStr1, "defrost_end_reason", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // ai_defrost_heating
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.u8AiHeating);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tAiLastCmd);
  json_encode_object(sSubStr1, "ai_defrost_heating", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // ai_defrost_fan_stop
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tDefrostCtrl.u8AiFanStop);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tDefrostCtrl.tAiLastCmd);
  json_encode_object(sSubStr1, "ai_defrost_fan_stop", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_eff_at_phase_start (efficiency when current heating/fanstop phase
  // began)
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tDefrostCtrl.r32EffAtPhaseStart);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_eff_at_phase_start", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_last_improvement_ago (seconds since last efficiency improvement, 0
  // if not in defrost)
  strcpy(sSubStr2, "");
  {
    uint32 u32ImprovAgo = 0;
    if (g_tDefrostCtrl.eState == e_Defrost_Heating ||
        g_tDefrostCtrl.eState == e_Defrost_InputFanStop) {
      u32ImprovAgo = time(NULL) - g_tDefrostCtrl.tLastImprovementTime;
    }
    json_encode_integer(sSubStr2, "value", u32ImprovAgo);
  }
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_last_improvement_ago", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // fireplace_active
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tFireplace.bActive ? 1 : 0);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tFireplace.tStartTime);
  json_encode_object(sSubStr1, "fireplace_active", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // fireplace_max_time (seconds)
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tFireplace.u32MaxTimeSec);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", g_tFireplace.tStartTime);
  json_encode_object(sSubStr1, "fireplace_max_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // fireplace_remaining (seconds, 0 if inactive)
  strcpy(sSubStr2, "");
  {
    uint32 u32Remaining = 0;
    if (g_tFireplace.bActive) {
      time_t tElapsed = time(NULL) - g_tFireplace.tStartTime;
      if (tElapsed < g_tFireplace.u32MaxTimeSec) {
        u32Remaining = g_tFireplace.u32MaxTimeSec - tElapsed;
      }
    }
    json_encode_integer(sSubStr2, "value", u32Remaining);
  }
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "fireplace_remaining", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_eff_imp_thresh
  strcpy(sSubStr2, "");
  json_encode_real32(sSubStr2, "value", g_tCtrlVars.r32EffImpThresh);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_eff_imp_thresh", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_fan_stop_no_imp_time
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tCtrlVars.u32FanStopNoImpTime);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_fan_stop_no_imp_time", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_fan_stop_max_dur
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tCtrlVars.u32FanStopMaxDur);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_fan_stop_max_dur", sSubStr2);
  strncat(sSubStr1, ",", 1);

  // defrost_stop_duration
  strcpy(sSubStr2, "");
  json_encode_integer(sSubStr2, "value", g_tCtrlVars.u32StopDuration);
  strncat(sSubStr2, ",", 1);
  json_encode_integer(sSubStr2, "ts", time(NULL));
  json_encode_object(sSubStr1, "defrost_stop_duration", sSubStr2);

  /* Remove trailing comma if present */
  size_t len = strlen(sSubStr1);
  if (len > 0 && sSubStr1[len - 1] == ',') {
    sSubStr1[len - 1] = '\0';
  }

  json_encode_object(sMesg, CONTROL_VARS, sSubStr1);
  strncat(sMesg, "}", 1);
}

/******************************************************************************
 *  Local function implementation
 ******************************************************************************/

static void ctrl_init() {
  memset(&g_tCtrlVars, 0x0, sizeof(g_tCtrlVars));
  memset(&g_tPreHeating, 0x0, sizeof(g_tPreHeating));
  memset(&g_tDefrostCtrl, 0x0, sizeof(g_tDefrostCtrl));
  memset(&g_tFireplace, 0x0, sizeof(g_tFireplace));

  g_tCtrlVars.r32MinExhaustTemp = DEFROST_EXHAUST_MIN_TEMP;
  g_tCtrlVars.u8DefrostMode = DEFROST_MODE_OFF;

  g_tCtrlVars.r32DefrostStartLevel = DEFROST_TARGET_LEVEL;
  g_tCtrlVars.r32DefrostTargetLevel = DEFROST_TARGET_LEVEL; // default 72%

  g_tCtrlVars.r32EffImpThresh = DEFROST_EFF_IMPROVEMENT_THRESH;
  g_tCtrlVars.u32FanStopNoImpTime = DEFROST_FANSTOP_NO_IMPROV_TIME;
  g_tCtrlVars.u32FanStopMaxDur = DEFROST_FANSTOP_MAX_DURATION;
  g_tCtrlVars.u32StopDuration = DEFROST_STOP_DURATION;

  post_heating_counter_init();
  pre_heating_resistor_init();
  defrost_resistor_init();
  load_cycle_count();
}

static void ctrl_run() {
  post_heating_counter_update();
  pre_heating_resistor_counter_update();
  defrost_resistor_counter_update();

  /* Fireplace auto-disable timer */
  if (g_tFireplace.bActive) {
    time_t tNow = time(NULL);
    if ((tNow - g_tFireplace.tStartTime) >= g_tFireplace.u32MaxTimeSec) {
      g_tFireplace.bActive = false;
      printf("[Fireplace] auto-disabled after %d seconds\n",
             g_tFireplace.u32MaxTimeSec);
    }
  }

  if (digit_vars_ok() && DS18B20_vars_ok()) {
    ctrl_update_vars();

    if (g_tCtrlVars.u32CallCnt % 2) {
      defrost_control();
    }
  }
}

static void ctrl_update_vars() {
  g_tCtrlVars.r32DewPoint = r32_calc_dew_point();
  calc_in_out_effiency(&g_tCtrlVars.r32InEfficiency,
                       &g_tCtrlVars.r32OutEfficiency);
  if (!g_tCtrlVars.u32CallCnt) {
    avf_filter_init(&g_tCtrlVars.tInEff, g_tCtrlVars.r32InEfficiency);
    avf_filter_init(&g_tCtrlVars.tOutEff, g_tCtrlVars.r32OutEfficiency);
  } else {
    real32 in_eff = g_tCtrlVars.r32InEfficiency;
    real32 out_eff = g_tCtrlVars.r32OutEfficiency;

    if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
      out_eff = g_tCtrlVars.tOutEff.r32Value;
    }

    avf_filter_calc(&g_tCtrlVars.tInEff, in_eff, g_tCtrlVars.u32CallCnt);
    avf_filter_calc(&g_tCtrlVars.tOutEff, out_eff, g_tCtrlVars.u32CallCnt);
  }
  g_tCtrlVars.u32CallCnt++;
}

static void input_fan_stop(void) { digit_set_input_fan_stop(14.0f); }

static void input_fan_start(void) { digit_set_input_fan_stop(-6.0f); }

static void load_cycle_count(void) {
  FILE *file = fopen(CYCLE_COUNT_FILE, "r");
  if (file) {
    if (fscanf(file, "%u", &g_tDefrostCtrl.u32CycleCount) != 1) {
      g_tDefrostCtrl.u32CycleCount = 0;
    }
    fclose(file);
  } else {
    g_tDefrostCtrl.u32CycleCount = 0;
  }
}

static void save_cycle_count(void) {
  FILE *file = fopen(CYCLE_COUNT_FILE, "w");
  if (file) {
    fprintf(file, "%u\n", g_tDefrostCtrl.u32CycleCount);
    fclose(file);
  }
}

static void capture_cycle_start_data(void) {
  g_tDefrostCtrl.tStart.r32InEff = g_tCtrlVars.r32InEfficiency;
  g_tDefrostCtrl.tStart.r32InEffFiltered = g_tCtrlVars.tInEff.r32Value;
  g_tDefrostCtrl.tStart.r32OutsideTemp = r32_DS18B20_outside_temp();

  g_tDefrostCtrl.tStart.r32IncomingTemp = r32_digit_incoming_temp();
  g_tDefrostCtrl.tStart.r32Rh = r32_digit_rh1_sensor() / 100.0f;
  g_tDefrostCtrl.tStart.r32DewPoint = g_tCtrlVars.r32DewPoint;
  g_tDefrostCtrl.tStart.r32DsOutsideTemp = r32_DS18B20_outside_temp();
  g_tDefrostCtrl.tStart.r32DsExhaustTemp = r32_DS18B20_exhaust_temp();
  g_tDefrostCtrl.tStart.r32DsIncomingTemp = r32_DS18B20_incoming_temp();
}

static void capture_cycle_end_data(E_DefrostEndReason eReason) {
  g_tDefrostCtrl.tEnd.r32InEff = g_tCtrlVars.r32InEfficiency;

  g_tDefrostCtrl.tEnd.r32IncomingTemp = r32_digit_incoming_temp();
  g_tDefrostCtrl.tEnd.eReason = eReason;
  g_tDefrostCtrl.tEnd.r32DsOutsideTemp = r32_DS18B20_outside_temp();
  g_tDefrostCtrl.tEnd.r32DsExhaustTemp = r32_DS18B20_exhaust_temp();
  g_tDefrostCtrl.tEnd.r32DsIncomingTemp = r32_DS18B20_incoming_temp();
}

static void defrost_control(void) {
  time_t tCurrentTime = time(NULL);
  real32 r32InEffFiltered = g_tCtrlVars.tInEff.r32Value;
  real32 r32InEff = g_tCtrlVars.r32InEfficiency;
  real32 r32ExhaustTemp = r32_DS18B20_exhaust_temp();
  real32 r32CurrentIncomingTemp = r32_digit_incoming_temp();

  /* Always update current values for monitoring */
  /* (Done in ctrl_update_vars mostly, but checked here) */

  if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_OFF) {
    g_tDefrostCtrl.eState = e_Measuring;
    /* Reset controls */
    input_fan_start();
    defrost_resistor_stop();
    return;
  }

  // --- AUTO Mode Implementation (One-Phase) ---
  if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_AUTO) {

    if (g_tDefrostCtrl.eState == e_Measuring) {
      /* Precondition 1: Outside temp < 0C */
      /* Precondition 2: Stop State Cooldown exceeded */
      bool bCooldownComplete = true;
      if (g_tDefrostCtrl.tStopStateStart > 0) {
        if ((tCurrentTime - g_tDefrostCtrl.tStopStateStart) <
            g_tCtrlVars.u32StopDuration) {
          bCooldownComplete = false;
        }
      }

      if (r32_DS18B20_outside_temp() < 0.0f && bCooldownComplete &&
          r32InEffFiltered < g_tCtrlVars.r32DefrostStartLevel &&
          r32InEff < r32InEffFiltered) {

        /* START DEFROST */
        g_tDefrostCtrl.tCheckTime = tCurrentTime;

        if (g_tFireplace.bActive) {
          /* Fireplace Mode: Heating ONLY, Fan Stop FORBIDDEN */
          g_tDefrostCtrl.eState = e_Defrost_Heating;
          defrost_resistor_start();
          input_fan_start(); // Ensure fan runs
          printf("[AUTO] Defrost Started (Fireplace Mode - Heating Only)\n");
        } else {
          /* Normal One-Phase: Input Fan Stop + Heating */
          g_tDefrostCtrl.eState = e_Defrost_InputFanStop;
          defrost_resistor_start(); // Ensure heating ON (One-phase =
                                    // FanStop+Heating)
          /* Fan stop command is sent in state handler below */
        }

        /* Capture start conditions */
        g_tDefrostCtrl.tCycleStart = tCurrentTime;
        if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
          g_tDefrostCtrl.tHeatingOnSince = tCurrentTime;
          g_tDefrostCtrl.tHeatingEnd = 0;
          g_tDefrostCtrl.tFanStopStart = 0;
        } else {
          g_tDefrostCtrl.tHeatingOnSince = 0;
          g_tDefrostCtrl.tHeatingEnd = 0;
          g_tDefrostCtrl.tFanStopStart = tCurrentTime;
        }
        g_tDefrostCtrl.tCycleEnd = 0;
        g_tDefrostCtrl.tEnd.eReason = e_EndReason_None;

        capture_cycle_start_data();
        // Override with local vars if they differed from helper logic
        // (preserving existing logic)

        g_tDefrostCtrl.tStart.r32IncomingTemp = r32CurrentIncomingTemp;
        /* Initialize efficiency improvement tracking */
        g_tDefrostCtrl.r32PrevEffSample = r32InEff;
        g_tDefrostCtrl.tLastEffSampleTime = tCurrentTime;
        g_tDefrostCtrl.tLastImprovementTime = tCurrentTime;
        g_tDefrostCtrl.r32EffAtPhaseStart = r32InEff;

        g_tDefrostCtrl.u32CycleCount++;
        save_cycle_count();

        if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
          input_fan_start();
          printf("Defrost Cycle %d STARTED (Fireplace: Heating)\n",
                 g_tDefrostCtrl.u32CycleCount);
        } else {
          input_fan_stop();
          printf("Defrost Cycle %d STARTED (FanStop)\n",
                 g_tDefrostCtrl.u32CycleCount);
        }
      }
    }

    else if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
      /* Fireplace Mode Heating Logic */
      if (!g_tFireplace.bActive) {
        /* Fireplace turned off? Switch to Fan Stop */
        g_tDefrostCtrl.eState = e_Defrost_InputFanStop;
        defrost_resistor_stop();
        printf("[AUTO] Fireplace OFF -> Switching to Fan Stop\n");
        return; // Loop will handle next state
      }

      /* Stop Condition: Exhaust Temp > Min Limit */
      if (r32ExhaustTemp > g_tCtrlVars.r32MinExhaustTemp) {
        g_tDefrostCtrl.eState = e_Defrost_Stopped;
        g_tDefrostCtrl.tStopStateStart = tCurrentTime;
        g_tDefrostCtrl.tCycleEnd = tCurrentTime;

        defrost_resistor_stop();

        /* Capture End Stats */
        capture_cycle_end_data(e_EndReason_TempTarget);
        // Override with local vars to match previous logic exactly
        g_tDefrostCtrl.tEnd.r32InEff = r32InEff;
        g_tDefrostCtrl.tEnd.r32IncomingTemp = r32CurrentIncomingTemp;

        printf("[Fireplace] Heating Done (Exhaust %.1f > %.1f)\n",
               r32ExhaustTemp, g_tCtrlVars.r32MinExhaustTemp);
      } else {
        /* Keep Heating */
        defrost_resistor_start();
      }

    } else if (g_tDefrostCtrl.eState == e_Defrost_InputFanStop) {
      /* Fireplace Safety Override */
      if (g_tFireplace.bActive) {
        /* Fireplace started -> Switch to Heating immediately */
        g_tDefrostCtrl.eState = e_Defrost_Heating;
        input_fan_start();                             // Fan ON
        defrost_resistor_start();                      // Heater ON
        g_tDefrostCtrl.tHeatingOnSince = tCurrentTime; // Start tracking heating
        printf("[AUTO] Fan Stop aborted -> Switched to Fireplace Heating\n");
        return;
      }
      /* Periodically check for efficiency improvement */
      if ((tCurrentTime - g_tDefrostCtrl.tLastEffSampleTime) >=
          DEFROST_EFF_SAMPLE_INTERVAL) {
        real32 r32Diff = r32InEff - g_tDefrostCtrl.r32PrevEffSample;
        if (r32Diff > g_tCtrlVars.r32EffImpThresh) {
          g_tDefrostCtrl.tLastImprovementTime = tCurrentTime;
        }
        g_tDefrostCtrl.r32PrevEffSample = r32InEff;
        g_tDefrostCtrl.tLastEffSampleTime = tCurrentTime;
      }

      /* End Condition 1: Efficiency recovered > target AND Exh > Min */
      bool bRecovered = (r32InEff > g_tCtrlVars.r32DefrostTargetLevel);
      bool bExhaustOK = (r32ExhaustTemp > g_tCtrlVars.r32MinExhaustTemp);

      /* End Condition 2: No improvement for X minutes (Efficiency Plateau) */
      bool bEffPlateau = (tCurrentTime - g_tDefrostCtrl.tLastImprovementTime) >
                         g_tCtrlVars.u32FanStopNoImpTime;

      /* End Condition 3: Max Duration */
      bool bMaxDuration = (tCurrentTime - g_tDefrostCtrl.tFanStopStart) >
                          g_tCtrlVars.u32FanStopMaxDur;

      /* Greedy Stop: Only stop if Target reached AND Plateaued (OR Safety Max
       * Duration) */
      /* We ignore standalone bEffPlateau to ensure we try to reach target */
      if ((bRecovered && bExhaustOK && bEffPlateau) || bMaxDuration) {
        /* Transition to STOPPED State (Cooldown) instead of Measuring */
        g_tDefrostCtrl.eState = e_Defrost_Stopped;
        g_tDefrostCtrl.tStopStateStart = tCurrentTime;
        g_tDefrostCtrl.tCycleEnd =
            tCurrentTime; // Cycle officially ends here (active part)

        input_fan_start(); // Resume Fan

        if (bRecovered && bExhaustOK) {
          capture_cycle_end_data(e_EndReason_EffRecovered);
        } else if (bEffPlateau) {
          capture_cycle_end_data(e_EndReason_FanStopPlateau);
        } else {
          capture_cycle_end_data(e_EndReason_Timeout);
        }

        /* Capture End Stats Overrides */
        g_tDefrostCtrl.tEnd.r32InEff = r32InEff;
        g_tDefrostCtrl.tEnd.r32IncomingTemp = r32CurrentIncomingTemp;

        printf("Defrost Cycle Ended (Reason: %d, Eff: %.1f)\n",
               g_tDefrostCtrl.tEnd.eReason, r32InEff);
      }
    } else if (g_tDefrostCtrl.eState == e_Defrost_Stopped) {
      /* Stop State (Cooldown) */
      /* Just wait for u32StopDuration. Logic handled in Measuring transition
       * check. */
      /* If we are here, we are just waiting. */
      if ((tCurrentTime - g_tDefrostCtrl.tStopStateStart) >=
          g_tCtrlVars.u32StopDuration) {
        g_tDefrostCtrl.eState = e_Measuring;
        printf("Defrost Stop State Complete. Resume Measuring.\n");
      }
    }

  }
  // --- AI Mode Implementation (Preserved) ---
  else if (g_tCtrlVars.u8DefrostMode == DEFROST_MODE_AI) {
    /* AI defrost mode:
       - Single command: ai_defrost_heating (1=Start/Keep Defrost, 0=Stop)
       - Firmware manages cycle: Heating -> (60s) -> Fan Stop -> Cooldown
       - Fireplace Mode blocks Fan Stop transition */

    time_t tCurrentTime = time(NULL);
    real32 r32InEff = g_tCtrlVars.r32InEfficiency;
    real32 r32CurrentIncomingTemp = r32_digit_incoming_temp();

    // 1. MEASURING -> HEATING
    if (g_tDefrostCtrl.eState == e_Measuring) {
      if (g_tDefrostCtrl.u8AiHeating) {
        printf("[AI] Cycle Started (Heating)\n");
        g_tDefrostCtrl.eState = e_Defrost_Heating;
        g_tDefrostCtrl.tCycleStart = tCurrentTime;
        g_tDefrostCtrl.tCheckTime = tCurrentTime;
        g_tDefrostCtrl.tHeatingOnSince = tCurrentTime;

        // Reset logs
        g_tDefrostCtrl.tHeatingEnd = 0;
        g_tDefrostCtrl.tFanStopStart = 0;
        g_tDefrostCtrl.tCycleEnd = 0;
        g_tDefrostCtrl.tEnd.eReason = e_EndReason_None;

        // Capture initial values
        capture_cycle_start_data();
        g_tDefrostCtrl.tStart.r32IncomingTemp = r32CurrentIncomingTemp;
      }
    }

    // 2. HEATING -> FAN STOP (or STOP)
    else if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
      if (!g_tDefrostCtrl.u8AiHeating) {
        // AI said STOP
        printf("[AI] Cycle Stopped by AI\n");
        g_tDefrostCtrl.eState = e_Defrost_Stopped;
        g_tDefrostCtrl.tHeatingEnd = tCurrentTime;
        g_tDefrostCtrl.tCycleEnd = tCurrentTime;
        g_tDefrostCtrl.tEnd.eReason = e_EndReason_EffRecovered;
        capture_cycle_end_data(e_EndReason_EffRecovered);
        g_tDefrostCtrl.tEnd.r32InEff = r32InEff;
        g_tDefrostCtrl.tEnd.r32IncomingTemp = r32CurrentIncomingTemp;
      } else {
        // AI says Keep Going. Check if we should transition to Fan Stop.
        // Wait 60s of pre-heating before stopping fan.
        if (tCurrentTime - g_tDefrostCtrl.tCheckTime > 60) {
          if (g_tFireplace.bActive) {
            // Fireplace ON: Keep Heating, Don't stop fan.
          } else {
            // Fireplace OFF: Transition to Fan Stop.
            printf("[AI] Transition to Fan Stop\n");
            g_tDefrostCtrl.eState = e_Defrost_InputFanStop;
            g_tDefrostCtrl.tFanStopStart = tCurrentTime;
            input_fan_stop();
          }
        }
      }
    }

    // 3. FAN STOP -> HEATING (or STOP)
    else if (g_tDefrostCtrl.eState == e_Defrost_InputFanStop) {
      if (!g_tDefrostCtrl.u8AiHeating) {
        // AI said STOP
        printf("[AI] Cycle Stopped by AI\n");
        g_tDefrostCtrl.eState = e_Defrost_Stopped;
        g_tDefrostCtrl.tHeatingEnd = tCurrentTime;
        g_tDefrostCtrl.tCycleEnd = tCurrentTime;
        g_tDefrostCtrl.tEnd.eReason = e_EndReason_EffRecovered;
        input_fan_start();
        capture_cycle_end_data(e_EndReason_EffRecovered);
        g_tDefrostCtrl.tEnd.r32InEff = r32InEff;
        g_tDefrostCtrl.tEnd.r32IncomingTemp = r32CurrentIncomingTemp;
      } else if (g_tFireplace.bActive) {
        // Safety: Fireplace turned ON during Fan Stop -> Abort to Heating
        printf("[AI] Fireplace Activated - Abort Fan Stop\n");
        g_tDefrostCtrl.eState = e_Defrost_Heating;
        input_fan_start();
        g_tDefrostCtrl.tCheckTime = tCurrentTime;
      }
    }

    // 4. STOPPED -> MEASURING
    else if (g_tDefrostCtrl.eState == e_Defrost_Stopped) {
      // Small buffer time before returning to Measuring
      if (tCurrentTime - g_tDefrostCtrl.tCycleEnd > 5) {
        g_tDefrostCtrl.u32CycleCount++;
        save_cycle_count();
        g_tDefrostCtrl.eState = e_Measuring;
        g_tDefrostCtrl.tHeatingOnSince = 0;
        printf("[AI] defrost cycle #%d complete. Resume Measuring.\n",
               g_tDefrostCtrl.u32CycleCount);
      }
    }

    // Safety catch-all
    else {
      g_tDefrostCtrl.eState = e_Measuring;
    }
  }

  /* GLOBAL: Ensure resistor is managed for One-Phase (Fan Stop + Heating) */
  /* This applies to BOTH Auto and AI modes to enforce the One-Phase physics */
  if (g_tDefrostCtrl.eState == e_Defrost_InputFanStop) {
    /* Keep heating ON during Fan Stop phase */
    defrost_resistor_start();
    if (g_tDefrostCtrl.tHeatingOnSince == 0) {
      g_tDefrostCtrl.tHeatingOnSince = time(NULL);
    }
  } else if (g_tDefrostCtrl.eState == e_Defrost_Heating) {
    /* Explicit Heating State (Fireplace or AI-Heating) */
    defrost_resistor_start();
    if (g_tDefrostCtrl.tHeatingOnSince == 0) {
      g_tDefrostCtrl.tHeatingOnSince = time(NULL);
    }
  } else {
    /* All other states: STOP HEATING */
    defrost_resistor_stop();
    g_tDefrostCtrl.tHeatingOnSince = 0;
  }
}

static void calc_in_out_effiency(real32 *pr32InEff, real32 *pr32OutEff) {
  /* MODIFIED: Use DIGIT incoming temp for input efficiency calc as requested */
  real32 r32IncomingTemp = r32_digit_incoming_temp();
  real32 r32OutsideTemp = r32_DS18B20_outside_temp();
  real32 r32InsideTemp = r32_digit_inside_temp();
  real32 r32ExhaustTemp = r32_DS18B20_exhaust_temp();

  real32 r32IncomingEff =
      ((r32IncomingTemp - r32OutsideTemp) / (r32InsideTemp - r32OutsideTemp)) *
      100.0f;

  real32 r32OutcomingEff =
      ((r32InsideTemp - r32ExhaustTemp) / (r32InsideTemp - r32OutsideTemp)) *
      100.0f;

  if (r32IncomingEff > 100.0f) {
    r32IncomingEff = 100.0f;
  } else if (r32IncomingEff < 0.0f) {
    r32IncomingEff = 0.0f;
  }

  if (r32OutcomingEff > 100.0f) {
    r32OutcomingEff = 100.0f;
  } else if (r32OutcomingEff < 0.0f) {
    r32OutcomingEff = 0.0f;
  }
  *pr32InEff = r32IncomingEff;
  *pr32OutEff = r32OutcomingEff;
}

static real32 r32_calc_dew_point(void) {
  real32 inside_temp = r32_digit_inside_temp();
  real32 r32Rh = r32_digit_rh1_sensor() / 100.0f;
  real32 tempA = 17.27f;
  real32 tempB = 237.7f;
  real32 tempZ = (((tempA * inside_temp) / (tempB + inside_temp)) + log(r32Rh));
  real32 dew_point = ((tempB * tempZ) / (tempA - tempZ));
  return dew_point;
}

static void avf_filter_init(T_AvfFilter *tFilter, real32 r32Value) {
  tFilter->r64Sum = 0;
  for (int i = 0; i < MOVING_AVERAGE_SIZE; i++) {
    tFilter->ar32Table[i] = r32Value;
    tFilter->r64Sum += r32Value;
  }
  tFilter->r32Value = r32Value;
}

static void avf_filter_calc(T_AvfFilter *tFilter, real32 r32NewValue,
                            uint32 i32index) {
  real32 r32OldValue = tFilter->ar32Table[i32index % MOVING_AVERAGE_SIZE];
  tFilter->r64Sum -= r32OldValue;
  tFilter->ar32Table[i32index % MOVING_AVERAGE_SIZE] = r32NewValue;
  tFilter->r64Sum += r32NewValue;
  tFilter->r32Value = tFilter->r64Sum / MOVING_AVERAGE_SIZE;
}
