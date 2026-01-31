# Vallox Defrost System - Improvement Plan & AI Development Guide

## Purpose of This Document

This document serves two purposes:
1. **Planning document** for improving the C firmware defrost algorithm
2. **Complete reference** for AI assistants to understand the system and implement changes correctly

---

## 1. Physical Understanding of the Frost Problem

### How Ice Forms in the Heat Exchanger

The Vallox 150 SE MLV uses a counter-flow heat exchanger. In cold weather:

1. **Warm, humid indoor air** flows through the exhaust side of the heat exchanger
2. **Cold outdoor air** flows through the supply (input) side
3. As warm exhaust air cools below its **dew point**, moisture condenses on the heat exchanger surfaces
4. When the surface temperature drops below 0°C, this moisture **freezes into ice**
5. Ice builds up progressively on the **output (exhaust) fan side** of the heat exchanger

### How Ice Manifests in Sensor Data

The key observable symptom is **dropping input air efficiency**:

```
Efficiency = ((IncomingTemp - OutsideTemp) / (InsideTemp - OutsideTemp)) * 100%
```

When ice blocks the heat exchanger:
- Airflow through the exchanger is restricted
- Heat transfer efficiency drops
- The incoming (supply) air temperature drops relative to what it should be
- The efficiency reading falls progressively

### The Two-Phase Defrost Process (From Real-World Experience)

The owner's experience over multiple winters has established that effective defrost requires **two distinct phases**:

#### Phase 1: Infrared Heating (Melt ice from the outside surface)
- **Defrost is NEVER started if outdoor temperature is above 0°C** - ice cannot form when outside air is above freezing
- An infrared heating resistor (GPIO pin 22) heats the **exhaust side** of the heat exchanger
- This melts ice from the **surface** of the output (exhaust) fan area
- Heating should continue **as long as efficiency is improving** (not a fixed duration)
- **Continuation condition**: Heating must continue until **both** exhaust temperature sensors read above 5°C:
  - DIGIT protocol exhaust temp (Vallox's own NTC sensor)
  - DS18B20 exhaust temp (external sensor `28-000004b0aa24`)
- The two sensors behave differently during defrost because of their physical placement:
  - **DS18B20 exhaust sensor is mounted closer to the heat exchanger surface**. While ice is present on the surface, this sensor stays near 0°C due to the latent heat of melting ice (ice absorbs heat without temperature rise until fully melted). This makes the DS18B20 sensor a reliable **ice presence indicator**.
  - **DIGIT exhaust sensor** (Vallox NTC) is further from the surface and responds differently.
- **The 5°C rule means**: heating is NOT done until surface ice is melted. As long as DS18B20 exhaust stays near 0°C, ice is still melting and heating must continue. Once both sensors rise above 5°C, the surface ice has melted and heating can stop.
- **Sauna scenario**: After sauna use, indoor humidity is extremely high, causing heavy ice buildup on the heat exchanger. In this case the heating phase may need to run significantly longer than normal because there is much more ice to melt. The DS18B20 sensor will stay near 0°C for an extended period. The algorithm must be patient and keep heating as long as the 5°C threshold is not yet reached on both sensors, even if efficiency improvement is temporarily slow (the ice mass is absorbing the heat energy). The absolute maximum heating duration is 30 minutes (safety timeout), heating must never exceed this.

#### Phase 2: Input Fan Stop (Melt ice from the inside)
- After heating phase completes, the **input (supply) fan is stopped**
- With no cold incoming air, the warm exhaust air flowing through the exchanger melts ice from the **inside** of the exchanger core
- Fan stop should continue **as long as efficiency is still improving**
- Once efficiency stops improving (plateaus or starts dropping), the fan stop should end

### The Sauna + Extreme Cold Edge Case (Future Improvement)

The most challenging defrost scenario is **sauna use during very cold weather (below -15°C)**. The behavior is counter-intuitive:

1. When sauna starts, high humidity indoor air enters the heat exchanger
2. **Efficiency temporarily INCREASES** - the initial ice formation on the exchanger surfaces actually improves heat transfer briefly (thin ice layer increases effective surface area and turbulence)
3. Ice continues to build rapidly due to the extreme temperature differential and high moisture content
4. **Efficiency only starts DECREASING once there is already a large amount of ice** - by this point ice has accumulated not just on the surface but also **inside the ventilation unit itself**
5. The current trigger (efficiency dropping below 72%) fires too late - by then there is far more ice to deal with, requiring longer heating and fan stop phases

**Ideal future behavior**: The system should detect the conditions that will lead to heavy icing (sauna active + outside temp < -15°C + high indoor humidity) and **start defrost preemptively, before efficiency begins to drop**. This could be implemented as:
- A predictive trigger: if outside temp < -15°C AND indoor humidity is rising rapidly (sauna detected), start a preemptive defrost cycle after a configurable delay
- An AI/ML model trained on historical sauna-frost cycles that learns the optimal preemptive timing
- Monitoring the efficiency *increase* as an early warning (unusual efficiency rise during cold weather = ice forming)

**For now**: This is documented for future implementation. The current algorithm will still handle the sauna case, but it will react later than optimal, requiring longer heating phases. The 30-minute heating limit and the exhaust temperature > 5°C continuation rule help ensure ice is properly melted even when the cycle starts late.

### Why This Is Better Than the Current Algorithm

The current AUTO mode uses **fixed thresholds**:
- Start heating when filtered efficiency < 72%
- Stop heating when efficiency > 85% OR incoming temp > 18°C OR timeout (15 min)
- Fan stop is only activated after a 15-minute heating timeout
- Fixed 10-minute cooldown after each phase

The problem: these fixed thresholds don't adapt to conditions. Sometimes heating for 5 minutes is enough, sometimes 20 minutes is needed. The ice amount and distribution vary with temperature, humidity, and wind conditions.

---

## 2. Current C Firmware Architecture

### File Map (What to Modify)

| File | Role | Needs Changes? |
|------|------|---------------|
| `c/ctrl_logic.c` | **Main defrost state machine** | **YES - Primary target** |
| `c/ctrl_logic.h` | Interface for control logic | Minor if new API needed |
| `c/defrost_resistor.c` | GPIO relay control for heater | No changes needed |
| `c/defrost_resistor.h` | Heater interface | No changes needed |
| `c/digit_protocol.c` | RS485 communication, fan stop | No changes needed |
| `c/digit_protocol.h` | DIGIT protocol interface | No changes needed |
| `c/DS18B20.c` | Temperature sensor reading | No changes needed |
| `c/DS18B20.h` | Sensor interface | No changes needed |
| `c/main.c` | Thread creation | No changes needed |
| `c/json_codecs.c` | UDP JSON encoding/decoding | May need new fields |
| `c/json_codecs.h` | JSON codec interface | May need new fields |
| `c/relay_control.c` | Low-level GPIO | No changes needed |

### Threading Model

Six pthreads run concurrently (defined in `c/main.c`):

1. **DS18B20 thread** - Reads 3 temperature sensors every 5 seconds
2. **DIGIT receive thread** - Listens for RS485 messages from Vallox unit
3. **DIGIT update thread** - Sends RS485 GET/SET requests every 2 seconds
4. **UDP server thread (port 8056)** - JSON API for external communication
5. **UDP server thread (port 8057)** - Secondary JSON API
6. **Control logic thread** - Runs `ctrl_run()` every 5 seconds (the defrost logic lives here)

**Thread safety warning**: There are no mutexes protecting shared state. Global variables (`g_tCtrlVars`, `g_tDefrostCtrl`, DS18B20 globals) are read/written by multiple threads. Any new shared state must follow the same pattern (or ideally add proper synchronization).

### Control Loop Timing

The control logic thread (`ctrl_logic_thread` in `c/ctrl_logic.c:175`) runs every 5 seconds:
```c
while(1) {
    ctrl_run();
    sleep(CTRL_LOGIC_TIMELEVEL);  // CTRL_LOGIC_TIMELEVEL = 5
}
```

Each `ctrl_run()` call:
1. Calls `ctrl_update_vars()` - calculates efficiency, dew point, updates moving average
2. Calls `defrost_control()` - the defrost state machine
3. Updates pre-heating and post-heating logic

### Current Defrost State Machine (`c/ctrl_logic.c:860-1235`)

```
                    +--------------+
                    |  e_Measuring  |
                    +------+-------+
                           | filtered_eff < 72% AND
                           | raw_eff < filtered_eff
                           v
                    +------------------+
                    | e_Defrost_Heating |
                    +------+-----------+
                           |
              +------------+----------------+
              |            |                |
    eff > 85% |   temp > 18C  |    timeout 15 min |
              |            |                |
              v            v                v
        +-------------+  +-------------+  +-------------------+
        |e_Defrost_    |  |e_Defrost_   |  |e_Defrost_         |
        |Stopped       |  |Stopped      |  |InputFanStop       |
        |(10 min cool) |  |(10 min cool)|  |(until eff > 85%   |
        +------+-------+  +------+------+  | or temp > 18C)    |
               |                 |         +--------+----------+
               |                 |                  |
               +-----------+-----+         +--------+
                           |               |
                           v               v
                      +--------------+
                      |  e_Measuring  |
                      +--------------+
```

### Key Constants (`c/ctrl_logic.c:24-37`)

```c
#define DEFROST_MODE_OFF                (0)
#define DEFROST_MODE_ON                 (1)
#define DEFROST_MODE_AUTO               (2)
#define DEFROST_MODE_AI                 (3)
#define MOVING_AVERAGE_SIZE             (720)  // 1 hour at 5-sec intervals
#define DEFROST_MAX_DURATION            (15)   // minutes
#define DEFROST_START_DURATION          (10)   // seconds
#define DEFROST_TARGET_LEVEL            (72)   // % efficiency trigger
#define DEFROST_TARGET_IN_EFF           (85)   // % efficiency target
#define DEFROST_TARGET_TEMP             (18)   // C incoming temp target
#define DEFROST_STOP_TIME               (10 * 60)   // 10 min cooldown
#define DEFROST_HEATING_SAFETY_TIMEOUT  (30 * 60)   // 30 min emergency shutoff
```

### Key Data Structures (`c/ctrl_logic.c:45-147`)

```c
// Defrost states
typedef enum {
    e_Measuring,
    e_Defrost_Heating,
    e_Defrost_Stopped,
    e_Defrost_InputFanStop
} E_DefrostState;

// End reasons for logging
typedef enum {
    e_EndReason_None,
    e_EndReason_EffRecovered,
    e_EndReason_TempTarget,
    e_EndReason_FanStopResolved,
    e_EndReason_Timeout,
    e_EndReason_SafetyShutoff
} E_DefrostEndReason;

// Moving average filter (1-hour window)
typedef struct {
    real32 ar32Table[MOVING_AVERAGE_SIZE];
    real64 r64Sum;
    real32 r32Value;
} T_AvfFilter;

// Main control variables
typedef struct {
    uint32 u32CallCnt;
    real32 r32InEfficiency;        // Current raw efficiency
    real32 r32OutEfficiency;
    T_AvfFilter tInEff;            // Filtered efficiency (1-hour moving average)
    T_AvfFilter tOutEff;
    byte u8DefrostMode;            // 0=OFF, 1=ON, 2=AUTO, 3=AI
    uint32 u32DefrostMaxDuration;  // Configurable max heating (minutes)
    real32 r32DefrostStartLevel;   // Configurable start threshold (%)
    real32 r32DefrostTargetInEff;  // Configurable target efficiency (%)
    real32 r32DefrostTargetTemp;   // Configurable target temp (C)
    real32 r32DewPoint;
    // ... pressure, etc.
} T_CtrlVars;

// Defrost cycle tracking
typedef struct {
    E_DefrostState eState;
    time_t tCheckTime;
    time_t tCycleStart, tHeatingEnd, tFanStopStart, tCycleEnd;
    uint32 u32CycleCount;
    // Captured conditions at start and end
    real32 r32StartInEff, r32StartInEffFiltered;
    real32 r32StartOutsideTemp, r32StartExhaustTemp, r32StartIncomingTemp;
    real32 r32StartDewPoint;
    real32 r32StartDsOutsideTemp, r32StartDsExhaustTemp, r32StartDsIncomingTemp;
    real32 r32EndInEff, r32EndIncomingTemp, r32EndExhaustTemp;
    real32 r32EndDsOutsideTemp, r32EndDsExhaustTemp, r32EndDsIncomingTemp;
    E_DefrostEndReason eEndReason;
    // AI mode
    byte u8AiHeating, u8AiFanStop;
    time_t tAiLastCmd;
    time_t tHeatingOnSince;
} T_Defrost;
```

### Available Sensor Readings

| Variable | Source | Access Function | Update Rate |
|----------|--------|----------------|-------------|
| Outside temp | DS18B20 sensor 1 | `r32_DS18B20_outside_temp()` | 5 sec |
| Exhaust temp (RH1) | DS18B20 sensor 2 | `r32_DS18B20_exhaust_temp()` | 5 sec |
| Incoming temp | DS18B20 sensor 3 | `r32_DS18B20_incoming_temp()` | 5 sec |
| Inside temp | DIGIT protocol | `r32_digit_inside_temp()` | 15 sec |
| Exhaust temp | DIGIT protocol | `r32_digit_exhaust_temp()` | 15 sec |
| Incoming temp | DIGIT protocol | `r32_digit_incoming_temp()` | 15 sec |
| Outside temp | DIGIT protocol | `r32_digit_outside_temp()` | 15 sec |
| RH humidity | DIGIT protocol | `r32_digit_rh1_sensor()` | 20 sec |
| Fan speed | DIGIT protocol | `u16_digit_cur_fan_speed()` | 120 sec |
| Raw efficiency | Calculated | `g_tCtrlVars.r32InEfficiency` | 5 sec |
| Filtered efficiency | Moving avg | `g_tCtrlVars.tInEff.r32Value` | 5 sec |
| Dew point | Calculated | `g_tCtrlVars.r32DewPoint` | 5 sec |

### Efficiency Calculation (`c/ctrl_logic.c:1237-1269`)

**Input air efficiency is calculated using DS18B20 sensors** for incoming and outside temperatures. The inside temperature comes from the DIGIT protocol (Vallox's own NTC sensor) because there is no DS18B20 sensor measuring indoor air.

```c
IncomingEff = ((IncomingTemp - OutsideTemp) / (InsideTemp - OutsideTemp)) * 100%
// IncomingTemp = DS18B20 sensor 3 (28-0000054bdcd4) - measures supply air after heat exchanger
// OutsideTemp  = DS18B20 sensor 1 (28-000004afcbb3) - measures outdoor air before heat exchanger
// InsideTemp   = DIGIT protocol (Vallox NTC) - indoor air temperature
// Clamped to 0-100%
```

The DS18B20 sensors provide more accurate and frequent readings (every 5 seconds) than the DIGIT protocol NTC sensors (every 15 seconds), which is why they are used for the two most critical measurements in the efficiency formula.

### Key Control Functions

```c
// Heater control
defrost_resistor_start();   // Turns ON GPIO pin 22 relay
defrost_resistor_stop();    // Turns OFF GPIO pin 22 relay

// Fan stop control via Vallox DIGIT protocol
digit_set_input_fan_stop(14.0f);  // Sets input fan stop temp to 14C (stops fan)
digit_set_input_fan_stop(-6.0f);  // Sets input fan stop temp to -6C (resumes fan)
```

### Fireplace Safety

The `g_tFireplace` structure tracks whether the fireplace mode is active. When active:
- Fan stop commands are **blocked** (prevents negative pressure from pulling smoke indoors)
- If fan stop is already active and fireplace is turned on, fan stop is immediately cancelled

---

## 3. Proposed Improved Defrost Algorithm

### New State Machine

The heating phase has **two conditions that must BOTH be met** before transitioning to fan stop:
1. Efficiency has stopped improving (no improvement for N minutes)
2. Both exhaust sensors read above 5°C (surface ice has melted)

If exhaust sensors are still below 5°C, heating continues even if efficiency improvement has temporarily stalled (ice mass is absorbing heat energy - latent heat phase).

```
                    +--------------+
                    |  e_Measuring  |
                    +------+-------+
                           | outside_temp < 0C (DS18B20)
                           | AND filtered_eff < start_level%
                           | AND raw_eff < filtered_eff (trending down)
                           v
                    +------------------+
                    | e_Defrost_Heating |
                    |                  |
                    | CONTINUE while:  |
                    | - eff improving  |
                    |   OR             |
                    | - either exhaust |
                    |   sensor < 5C   |
                    +------+-----------+
                           |
              +------------+---------------------------+
              |                                        |
   eff stopped improving                  safety timeout 30 min
   AND both exhaust > 5C                              |
              |                                        |
              v                                        v
  +--------------------+                   +--------------+
  |e_Defrost_          |                   |EMERGENCY     |
  |InputFanStop        |                   |STOP -> OFF   |
  |(keep while eff     |                   +--------------+
  | still improving)   |
  +--------+-----------+
           |
           | eff stops improving
           | (plateau or dropping)
           v
    +--------------+
    |e_Defrost_    |
    |Stopped       |
    |(10 min cool) |
    +------+-------+
           |
           v
    +--------------+
    |  e_Measuring  |
    +--------------+
```

### Detailed Algorithm Specification

#### Phase: Measuring -> Heating Trigger

**Precondition** (hard rule):
```
Outside temperature (DS18B20 sensor 1) must be BELOW 0°C
```
Defrost is NEVER started if outdoor temperature is above zero. Ice cannot form on the heat exchanger when outdoor air is above freezing. This check must be enforced regardless of what efficiency readings show (efficiency drops from other causes should not trigger defrost in warm weather).

**Condition** (same as current, only evaluated when outside temp < 0°C):
```
filtered_efficiency < defrost_start_level (default 72%)
AND raw_efficiency < filtered_efficiency (efficiency is dropping)
```

**On trigger**:
- Capture all start conditions (same as current)
- Record the current efficiency as `r32EffAtPhaseStart`
- Initialize `r32PrevEffSample` for trend tracking
- Set `tLastImprovementTime` = now
- Transition to `e_Defrost_Heating`

#### Phase: Heating

**Core principle**: Heating continues as long as it is doing useful work. There are two independent reasons to keep heating:
- Efficiency is still improving (ice is melting and airflow is recovering)
- Either exhaust sensor is below 5°C (surface ice is still present, absorbing heat via latent heat)

Heating stops ONLY when **both** conditions are satisfied:
- Efficiency has stopped improving (no improvement for N minutes)
- AND both exhaust sensors read above 5°C

**Every 5-second cycle, check:**

1. **Exhaust temperature as ice indicator** (NEW - critical):
   ```c
   real32 r32DsExhaust = r32_DS18B20_exhaust_temp();
   real32 r32DigitExhaust = r32_digit_exhaust_temp();
   bool bExhaustAbove5C = (r32DsExhaust > 5.0f) && (r32DigitExhaust > 5.0f);
   ```
   **Physics**: The DS18B20 exhaust sensor is mounted close to the heat exchanger surface. While ice is present, this sensor stays near 0°C because the latent heat of fusion holds the temperature constant as ice melts. The sensor will only rise above 0°C once the surrounding ice has melted. Requiring both sensors above 5°C provides a reliable signal that surface ice is gone.

   **Sauna scenario**: After sauna use, indoor humidity is very high (often 60-80%+), causing heavy ice buildup. In this case the DS18B20 exhaust sensor may stay near 0°C for a long time. The algorithm must NOT stop heating just because efficiency improvement has stalled - the ice mass is absorbing all the heat energy (latent heat phase). Keep heating until both exhaust sensors confirm ice is gone (> 5°C).

2. **Efficiency improvement tracking** (NEW):
   - Sample efficiency every N seconds (e.g., every 30 seconds to avoid noise)
   - Compare current efficiency to `r32PrevEffSample`
   - If efficiency improved (even slightly): update `tLastImprovementTime` = now
   - Track whether efficiency has stopped improving for a configurable period (e.g., 3-5 minutes)

   ```c
   // Pseudocode for improvement detection
   if (time_since_last_sample >= DEFROST_EFF_SAMPLE_INTERVAL)  // e.g., 30 sec
   {
       if (r32InEff > r32PrevEffSample + DEFROST_EFF_IMPROVEMENT_THRESHOLD)  // e.g., 0.5%
       {
           tLastImprovementTime = now;
       }
       r32PrevEffSample = r32InEff;
   }

   bool bEffStoppedImproving =
       (now - tLastImprovementTime > DEFROST_HEATING_NO_IMPROV_TIME);  // e.g., 3 min
   ```

3. **Combined stop condition** (NEW):
   ```c
   // BOTH conditions must be true to stop heating
   if (bEffStoppedImproving && bExhaustAbove5C)
   {
       // Heating is done: efficiency plateaued AND surface ice is melted
       transition_to(e_Defrost_InputFanStop);
   }
   // If exhaust is still < 5C, keep heating even if efficiency stalled
   // (latent heat phase - ice is absorbing energy without temp/eff change)
   ```

4. **Safety timeout** (keep existing, hard limit):
   - **30-minute absolute maximum heating** -> emergency shutoff and defrost mode set to OFF
   - This is the only condition that can stop heating regardless of exhaust temp or efficiency
   - Heating must NEVER exceed 30 minutes under any circumstances

5. **Fireplace safety** (keep existing):
   - If fireplace activates during heating, skip fan stop phase

**During heating**: `defrost_resistor_start()` keeps heater ON

#### Phase: Input Fan Stop

**Purpose**: After heating melts ice from the surface, stopping the input fan allows warm exhaust air to melt ice from inside the exchanger core.

**Activation**: `digit_set_input_fan_stop(14.0f)` (same as current)

**Every 5-second cycle, check:**

1. **Efficiency improvement tracking** (same logic as heating phase):
   - Sample efficiency periodically
   - Track whether it's still improving
   - When efficiency **stops improving** (no improvement for configurable period):
     - Fan stop is no longer effective -> end fan stop
     - Transition to `e_Defrost_Stopped`

   ```c
   if (now - tLastImprovementTime > DEFROST_FANSTOP_NO_IMPROVEMENT_TIMEOUT)  // e.g., 5 min
   {
       digit_set_input_fan_stop(-6.0f);  // Resume input fan
       transition_to(e_Defrost_Stopped);
   }
   ```

2. **Maximum fan stop duration** (safety):
   - Cap at configurable maximum (e.g., 20 minutes)
   - Prevents indefinite fan stop

3. **Fireplace safety** (keep existing):
   - Immediately resume fan if fireplace activates

**During fan stop**: Heater is OFF, input fan is stopped

#### Phase: Stopped (Cooldown)

Same as current: 10-minute cooldown before returning to Measuring state. This prevents rapid cycling.

### New Constants to Add

```c
#define DEFROST_EXHAUST_MIN_TEMP        (5.0f)  // C - minimum exhaust temp during heating
#define DEFROST_EFF_SAMPLE_INTERVAL     (30)    // seconds between efficiency samples
#define DEFROST_EFF_IMPROVEMENT_THRESH  (0.5f)  // % minimum improvement to count
#define DEFROST_HEATING_NO_IMPROV_TIME  (3 * 60)  // 3 min - stop heating if no improvement
#define DEFROST_FANSTOP_NO_IMPROV_TIME  (5 * 60)  // 5 min - stop fan stop if no improvement
#define DEFROST_FANSTOP_MAX_DURATION    (20 * 60) // 20 min - max fan stop
```

### New Fields in T_Defrost Structure

```c
typedef struct {
    // ... existing fields ...

    // Efficiency improvement tracking (NEW)
    real32 r32PrevEffSample;       // Last sampled efficiency for trend detection
    time_t tLastEffSampleTime;     // When we last sampled efficiency
    time_t tLastImprovementTime;   // When efficiency last improved
    real32 r32EffAtPhaseStart;     // Efficiency when current phase began
} T_Defrost;
```

### New End Reasons to Add

```c
typedef enum {
    e_EndReason_None,
    e_EndReason_EffRecovered,       // Efficiency reached target (legacy)
    e_EndReason_TempTarget,         // Incoming temp reached target (legacy)
    e_EndReason_FanStopResolved,    // Fan stop achieved recovery
    e_EndReason_Timeout,            // Duration timeout
    e_EndReason_SafetyShutoff,      // 30-min emergency
    e_EndReason_EffPlateau,         // NEW: Efficiency plateaued AND exhaust > 5C (normal completion)
    e_EndReason_FanStopPlateau      // NEW: Fan stop efficiency plateaued (normal completion)
} E_DefrostEndReason;
```

---

## 4. Implementation Steps

### Step 1: Add New Constants and Structure Fields

In `c/ctrl_logic.c`:
- Add the new `#define` constants after the existing ones (around line 37)
- Add new fields to `T_Defrost` struct (around line 140)
- Add new enum values to `E_DefrostEndReason` (around line 61)

### Step 2: Modify the AUTO Mode State Machine

The defrost AUTO mode logic starts at `c/ctrl_logic.c:933`. The changes are:

1. **Measuring -> Heating transition** (line 941-966):
   - Add initialization of new tracking fields when entering heating phase

2. **Heating phase checks** (line 968-1022):
   - Replace the fixed efficiency threshold check with improvement tracking
   - Add exhaust temperature safety check (both sensors > 5C)
   - Keep the fireplace safety check
   - After heating: always transition to InputFanStop (not Stopped)

3. **InputFanStop phase** (line 1024-1068):
   - Replace fixed efficiency threshold with improvement tracking
   - Add maximum duration safety limit
   - Keep fireplace safety check

### Step 3: Modify the AI Mode Similarly

The AI mode (`c/ctrl_logic.c:1095-1234`) receives commands from the cloud. The C firmware should still enforce the exhaust temperature safety check even in AI mode. The improvement-based stopping logic should be implemented in the cloud function (Python), not in C for AI mode, since the cloud function controls the timing.

However, add to AI mode in C:
- Exhaust temperature continuation enforcement: do NOT allow AI to stop heating while either exhaust sensor is below 5°C (ice still present on surface)
- 30-minute safety timeout still applies regardless of AI commands
- Log the new end reasons

### Step 4: Update JSON Encoding

In `c/json_codecs.c`, add the new defrost tracking fields to the `control_vars` JSON response so the cloud function and dashboard can monitor:
- Current efficiency trend (improving/stable/declining)
- Time since last improvement
- Exhaust temperatures from both sensors

### Step 5: Update Cloud Function (Future)

The `cloud-functions-predict/src/index.ts` should be updated to use improvement-based logic instead of fixed thresholds when controlling defrost in AI mode.

---

## 5. Testing Strategy

### Simulation Testing (Before Deployment)

1. Compile with `make` in `c/` directory
2. Test without hardware by mocking sensor values via UDP SET commands
3. Verify state transitions by monitoring printf output
4. Verify that exhaust temp safety cutoff works

### Staged Deployment

1. **Phase 1**: Deploy improved AUTO mode, monitor via dashboard
2. **Phase 2**: Collect defrost cycle data with new fields
3. **Phase 3**: Update AI/ML model with new features (efficiency improvement rate, phase durations)

### Key Metrics to Validate

- Does the new algorithm use less heating energy per cycle? (shorter average heating duration)
- Does efficiency recover to a higher level after each cycle?
- Are there fewer repeated cycles in quick succession?
- Do the safety limits (exhaust > 5C) ever trigger?

---

## 6. Risk Assessment

| Risk | Mitigation |
|------|-----------|
| Improvement detection too noisy (false improvements from sensor noise) | Use filtered efficiency and minimum threshold (0.5%) |
| Improvement detection too slow (keeps heating unnecessarily) | 3-minute no-improvement timeout is conservative; 30-min safety timeout always applies |
| Heavy ice after sauna: heating runs very long | Expected behavior. DS18B20 stays near 0C during latent heat phase. 30-min safety timeout is the hard limit. |
| Exhaust 5C threshold too high (heating runs longer than needed) | Tunable via API. Both sensors must exceed 5C. Can be lowered if monitoring shows unnecessary heating. |
| Exhaust 5C threshold too low (heating stops before ice melted) | DS18B20 near-surface placement gives reliable ice detection. 5C provides margin above 0C melting point. |
| No improvement detected during fan stop (false negative) | Maximum fan stop duration (20 min) as safety net |
| Thread safety with new fields | Follow existing pattern (no mutexes, accept eventual consistency) |

---

## 7. Configuration Parameters (Tunable via API)

All new parameters should be configurable via the existing `ctrl_set_var_by_name()` mechanism so they can be tuned without recompilation:

| Parameter | Default | API Name | Description |
|-----------|---------|----------|-------------|
| Exhaust min temp | 5.0C | `defrost_exhaust_min_temp` | Safety limit during heating |
| Efficiency sample interval | 30 sec | `defrost_eff_sample_interval` | How often to check trend |
| Improvement threshold | 0.5% | `defrost_eff_improvement_thresh` | Minimum delta to count as improvement |
| Heating no-improvement timeout | 180 sec | `defrost_heating_no_improv_time` | Stop heating after this long without improvement |
| Fan stop no-improvement timeout | 300 sec | `defrost_fanstop_no_improv_time` | Stop fan stop after this long without improvement |
| Fan stop max duration | 1200 sec | `defrost_fanstop_max_duration` | Absolute max fan stop |

---

## 8. Quick Reference for AI Assistants

### "I need to modify the defrost algorithm"

1. Open `c/ctrl_logic.c`
2. The `defrost_control()` function starts around line 860
3. AUTO mode is at line 933, AI mode is at line 1095
4. State machine uses `g_tDefrostCtrl.eState` to track current phase
5. Heater ON: `defrost_resistor_start()`, heater OFF: `defrost_resistor_stop()`
6. Fan stop ON: `digit_set_input_fan_stop(14.0f)`, fan resume: `digit_set_input_fan_stop(-6.0f)`

### "I need to add a new configurable parameter"

1. Add `#define` in `c/ctrl_logic.c` constants section
2. Add field to `T_CtrlVars` struct
3. Initialize in `ctrl_init()`
4. Add `else if` case in `ctrl_set_var_by_name()` (line 186+)
5. Add to `ctrl_json_encode()` for JSON output

### "I need to add new data to the JSON API"

1. Open `c/json_codecs.c`
2. Find the encode function for the relevant variable group
3. Add `sprintf()` for the new field following existing pattern

### "I need to build and test"

```bash
cd c/
make    # Requires bcm2835 library on Raspberry Pi
```

### "I need to understand the efficiency calculation"

Input air efficiency uses **DS18B20 sensors** for incoming and outside temps:
```
Efficiency = ((DS18B20_incoming - DS18B20_outside) / (DIGIT_inside - DS18B20_outside)) * 100%
```
- DS18B20 incoming (sensor 3) and outside (sensor 1) update every 5 seconds
- DIGIT inside temp updates every 15 seconds (no DS18B20 sensor for indoor air)
- Moving average filter has 720 samples (1-hour window)

### "I need to understand the sensor layout"

```
                    +---------------------+
  Outside Air -->  |                     | --> Incoming Air (to rooms)
  (DS18B20 s1)     |   HEAT EXCHANGER    |     (DS18B20 s3)
                    |                     |
  Exhaust Air <--  |                     | <-- Inside Air (from rooms)
  (DS18B20 s2)     |   (ice forms here)  |     (DIGIT sensor)
  (DIGIT exhaust)  +---------------------+
                          ^
                    Infrared Heater
                    (GPIO pin 22)
```

---

## 9. Compatibility with AI/ML Pipeline

The existing ML pipeline (`python/ai_learner.py`) trains on defrost cycle data stored in Firestore. When implementing improvements:

### Training Data: Use DS18B20 Sensors for Outside Temperature

**IMPORTANT**: The `start_outside_temp` training feature must use the **DS18B20 outside temperature** sensor (`ds_outside_temp` from `ds18b20_vars`), NOT the DIGIT protocol NTC sensor (`outside_temp` from `digit_vars`). This has been corrected in both `python/ai_config.py` and `cloud-functions-training/ai_config.py`:

```python
# CORRECT - uses DS18B20 sensor (more accurate, same as efficiency calculation)
'start_outside_temp': ('ds18b20_vars', 'ds_outside_temp'),

# WRONG - was using DIGIT protocol NTC sensor
# 'start_outside_temp': ('digit_vars', 'outside_temp'),
```

**Rationale**: The efficiency calculation itself uses DS18B20 sensors for outside and incoming temperatures. Training the ML model on the same sensor source ensures consistency. The DS18B20 sensors are also more accurate and update more frequently (every 5 seconds vs 15 seconds for DIGIT).

### Training Data: Exhaust Temperature Is Average of Both Sensors

**IMPORTANT**: The `start_exhaust_temp` training feature must be the **average of the DIGIT protocol and DS18B20 exhaust temperature** readings, NOT just one sensor. This is a calculated feature defined in `CALCULATED_FEATURES` in both config files:

```python
'start_exhaust_temp': {
    'description': 'Average of DIGIT and DS18B20 exhaust temperatures',
    'method': 'average',  # (sources[0] + sources[1]) / 2
    'sources': [
        ('digit_vars', 'exhaust_temp'),       # Vallox NTC sensor (further from surface)
        ('ds18b20_vars', 'ds_exhaust_temp'),   # DS18B20 sensor (close to surface)
    ],
}
```

**Rationale**: The two exhaust sensors have different physical placements. The DS18B20 is mounted close to the heat exchanger surface (sensitive to ice presence), while the DIGIT NTC is further away. Averaging both gives a more representative exhaust air temperature for ML training. During defrost, the DS18B20 sensor near the surface stays near 0°C while ice is present, so the average captures both the air temperature and the surface condition influence.

**Implementation note**: The `CALCULATED_FEATURES` dict in `ai_config.py` defines features that require values from multiple sources. Code that builds feature vectors must:
1. Process `CALCULATED_FEATURES` entries **in dependency order** - check `depends_on` field
2. `start_exhaust_temp` (average) must be calculated **first** because `start_dew_point_delta` depends on it
3. `start_dew_point_delta` = averaged `start_exhaust_temp` minus dew point (its first source is the string `'start_exhaust_temp'` referencing the already-calculated feature, not a raw sensor tuple)
4. For any feature not in `LIVE_FEATURE_MAPPING`, check `CALCULATED_FEATURES`, fetch the required sources, and apply the specified method (`average`, `subtract`, etc.)

### Other Guidelines

1. **Keep the cycle data capture** - the `T_Defrost` fields that record start/end conditions are used for ML training
2. **Add new features** to the cycle data:
   - `heating_stopped_reason` (new end reasons: EffPlateau, FanStopPlateau, SafetyShutoff)
   - `peak_eff_during_heating` - highest efficiency reached during heating
   - `eff_improvement_rate` - average efficiency improvement per minute
   - `total_eff_gained` - efficiency at end minus efficiency at start
3. **Update `python/ai_config.py`** training features list if new fields are added
4. **Update Firestore schema** in `cloud-functions/src/index.ts` to store new fields

The ML model can eventually learn:
- Optimal heating duration given current conditions (instead of waiting for plateau)
- Whether fan stop is needed based on how much improvement heating achieved
- Predictive defrost triggers (start before efficiency drops significantly)
