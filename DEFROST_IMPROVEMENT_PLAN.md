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

### The One-Phase Defrost Process (Fan Stop + Heating)

The defrost process has been simplified to a **one-phase** approach.

#### Phase 1: Defrost (Fan Stop + Heating)

- **Trigger**:
  - Outside temp < 0°C
  - `filtered_efficiency < start_level` (e.g., 72%)
  - Efficiency likely trending down (`raw < filtered`)
- **Action**:
  - **Heating Turned ON**: `defrost_resistor_start()` (Ensures efficient melting)
  - **Input Fan STOPPED**: `digit_set_input_fan_stop(14.0f)`
- **Duration**: Continues until target met AND improvement stalls (Greedy Strategy).
- **Stop Condition** (Defrost Over):
  - **Success**: Efficiency > Target (85%) **AND** Efficiency Plateaued (No improvement for X mins).
  - **Timeout**: Safety max duration reached.
  - **Fireplace Mode**: If active, Input Fan is FORCED ON (Heating Only).

#### Phase 2: Stop State (Cooldown / Refractory Period)

- **Action**:
  - Input Fan Resumed (`digit_set_input_fan_stop(-6.0f)`)
  - **Heating Stopped**: `defrost_resistor_stop()`
  - **New Defrost Forbidden**: The system enters a "Stop State".
- **Duration**: Configurable via `defrost_stop_duration` (default 10 mins).
- **Purpose**: Prevents rapid cycling and allows system to stabilize.

---

## 2. Current C Firmware Architecture

### File Map (What to Modify)

| File              | Role                           | Needs Changes?                  |
| :---------------- | :----------------------------- | :------------------------------ |
| `c/ctrl_logic.c`  | **Main defrost state machine** | **YES - Primary target**        |
| `c/ctrl_logic.h`  | Interface for control logic    | No changes needed               |
| `c/json_codecs.c` | UDP JSON encoding/decoding     | **YES - Add new stop_duration** |

---

## 3. Proposed Improved Defrost Algorithm (One-Phase)

### New State Machine

```
                    +--------------+
                    |  e_Measuring  |
                    +------+-------+
                           | outside_temp < 0C
                           | AND filtered_eff < start_level%
                           v
                    +------------------+
                    | e_Defrost_       | <--- HEATER ON
                    | InputFanStop     | <--- INPUT FAN STOPPED
                    +------+-----------+
                           |
              +------------+----------------+
              |            |                |
     eff > target |                   safety
     AND plateau  |                   timeout
              |            |                |
              v            v                v
        +-------------------------------------+
        |          e_Defrost_Stopped          |
        |      (Resume Fan, Wait Duration)    |
        +------------------+------------------+
                           |
                           | timer > defrost_stop_duration
                           v
                    +--------------+
                    |  e_Measuring  |
                    +--------------+
```

### Detailed Algorithm Specification

#### Phase: Measuring -> Input Fan Stop Trigger

**Precondition**: Outside Temp < 0°C.

**Condition**:

```c
filtered_efficiency < g_tCtrlVars.r32DefrostStartLevel  // Configurable
AND raw_efficiency < filtered_efficiency
```

**Transition**:

- `defrost_resistor_stop()` (Ensure OFF)
- `digit_set_input_fan_stop(14.0f)` (Stop Fan)
- `state = e_Defrost_InputFanStop`
- Initialize improvement tracking (`tLastImprovementTime`, etc.)

#### Phase: Input Fan Stop

**Every cycle (5s)**:

1.  **Monitor Improvement**:
    - Compare current efficiency to last sample.
    - If `eff > prev + threshold`: `tLastImprovementTime = now`.
2.  **Check Exit Conditions**:
    - **Success**: `eff > g_tCtrlVars.r32DefrostTargetInEff` (e.g., 85%).
    - **Plateau**: `(now - tLastImprovementTime) > defrost_fanstop_no_imp_time`.
    - **Greedy Stop Logic**: Stop ONLY if `(Success AND Plateau) OR MaxDuration`.
      (We keep heating/defrosting even if target is met, as long as it's improving).

#### Phase: Stopped (Cooldown)

**Enter**:

- `digit_set_input_fan_stop(-6.0f)` (Resume Fan)
- `tStopStateStart = now`

**Check**:

- `if (now - tStopStateStart > g_tCtrlVars.u32DefrostStopDuration)`: -> Go to Measuring.

### New Configuration Parameters

| Parameter               | Default        | C Var                    |
| :---------------------- | :------------- | :----------------------- |
| **Stop State Duration** | **600s** (10m) | `u32DefrostStopDuration` |

(Existing params `defrost_eff_improvement_thresh`, `defrost_fanstop_no_improv_time` still apply).

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
   - Add exhaust temperature safety check (DS18B20 sensor > 5C)
   - Keep the fireplace safety check
   - After heating: always transition to InputFanStop (not Stopped)

3. **InputFanStop phase** (line 1024-1068):
   - Replace fixed efficiency threshold with improvement tracking
   - Add maximum duration safety limit
   - Keep fireplace safety check

### Step 3: Modify the AI Mode Similarly

The AI mode (`c/ctrl_logic.c:1095-1234`) receives commands from the cloud. The C firmware should still enforce the exhaust temperature safety check even in AI mode. The improvement-based stopping logic should be implemented in the cloud function (Python), not in C for AI mode, since the cloud function controls the timing.

However, add to AI mode in C:

- Exhaust temperature continuation enforcement: do NOT allow AI to stop heating while DS18B20 exhaust sensor is below 5°C (ice still present on surface)
- 30-minute safety timeout still applies regardless of AI commands
- Log the new end reasons

### Step 4: Update JSON Encoding

In `c/json_codecs.c`, add the new defrost tracking fields to the `control_vars` JSON response so the cloud function and dashboard can monitor:

- Current efficiency trend (improving/stable/declining)
- Time since last improvement
- Exhaust temperatures from both sensors

### Step 5: Update AI/ML Configuration & Cloud Functions

Since the DIGIT exhaust sensor is unreliable, the AI pipeline must be updated to use the DS18B20 sensor.

1.  **Update `python/ai_config.py` and `cloud-functions-training/ai_config.py`**:
    - Change `start_exhaust_temp` source to `ds_exhaust_temp` only.
    - Update `start_dew_point_delta` to depend on the new source.

2.  **Update `cloud-functions-predict/src/index.ts`**:
    - Fetch `ds18b20_vars` from the API (currently only fetches control and digit vars).
    - Map `Sensors.exhaustTemp` to `ds_exhaust_temp`.

3.  **Update `cloud-functions/src/index.ts`** (Persistence):
    - Ensure `start_exhaust_temp` in Firestore relies on the correct sensor (or is clearly labeled). Note: The C firmware sends `defrost_start_exhaust_temp` (DIGIT) and `defrost_start_ds_exhaust_temp` (DS18B20). The ML config determines which one is used for training.

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

| Risk                                                                   | Mitigation                                                                                                  |
| ---------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------- |
| Improvement detection too noisy (false improvements from sensor noise) | Use filtered efficiency and minimum threshold (0.5%)                                                        |
| Improvement detection too slow (keeps heating unnecessarily)           | 3-minute no-improvement timeout is conservative; 30-min safety timeout always applies                       |
| Heavy ice after sauna: heating runs very long                          | Expected behavior. DS18B20 stays near 0C during latent heat phase. 30-min safety timeout is the hard limit. |
| Exhaust 5C threshold too high (heating runs longer than needed)        | Tunable via API. DS18B20 sensor must exceed 5C. Can be lowered if monitoring shows unnecessary heating.     |
| Exhaust 5C threshold too low (heating stops before ice melted)         | DS18B20 near-surface placement gives reliable ice detection. 5C provides margin above 0C melting point.     |
| No improvement detected during fan stop (false negative)               | Maximum fan stop duration (20 min) as safety net                                                            |
| Thread safety with new fields                                          | Follow existing pattern (no mutexes, accept eventual consistency)                                           |

---

## 7. Configuration Parameters (Tunable via API)

All new parameters should be configurable via the existing `ctrl_set_var_by_name()` mechanism so they can be tuned without recompilation:

| Parameter                       | Default  | API Name                         | Description                                       |
| ------------------------------- | -------- | -------------------------------- | ------------------------------------------------- |
| Exhaust min temp                | 5.0C     | `defrost_exhaust_min_temp`       | Safety limit during heating                       |
| Efficiency sample interval      | 30 sec   | `defrost_eff_sample_interval`    | How often to check trend                          |
| Improvement threshold           | 0.5%     | `defrost_eff_improvement_thresh` | Minimum delta to count as improvement             |
| Heating no-improvement timeout  | 180 sec  | `defrost_heating_no_improv_time` | Stop heating after this long without improvement  |
| Fan stop no-improvement timeout | 300 sec  | `defrost_fanstop_no_improv_time` | Stop fan stop after this long without improvement |
| Fan stop max duration           | 1200 sec | `defrost_fanstop_max_duration`   | Absolute max fan stop                             |

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

```
Efficiency = ((DIGIT_incoming - DS18B20_outside) / (DIGIT_inside - DS18B20_outside)) * 100%
```

- **Incoming Temp**: Uses DIGIT protocol sensor (updated decision due to user request).
- **Outside Temp**: Uses DS18B20 sensor (more accurate).
- **Inside Temp**: Uses DIGIT protocol sensor.

- DS18B20 incoming (sensor 3) and outside (sensor 1) update every 5 seconds
- DIGIT inside temp updates every 15 seconds (no DS18B20 sensor for indoor air)
- Moving average filter has 720 samples (1-hour window)

### "I need to understand the sensor layout"

```
                    +---------------------+
  Outside Air -->  |                     | --> Incoming Air (to rooms)
  (DS18B20 s1)     |   HEAT EXCHANGER    |     (DS18B20 s3 - NOT USED for eff)
                   |                     |     (DIGIT incoming - USED for eff)
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

# RAW - DIGIT protocol NTC sensor is less accurate
# 'start_outside_temp': ('digit_vars', 'outside_temp'),
```

**Rationale**: The efficiency calculation itself uses DS18B20 sensors for outside and incoming temperatures. Training the ML model on the same sensor source ensures consistency. The DS18B20 sensors are also more accurate and update more frequently (every 5 seconds vs 15 seconds for DIGIT).

### Training Data: Exhaust Temperature Is Average of Both Sensors

**IMPORTANT**: The `start_exhaust_temp` training feature must use the **DS18B20 exhaust temperature** sensor (`ds_exhaust_temp` from `ds18b20_vars`). The DIGIT exhaust sensor values are unreliable and should be ignored.

```python
'start_exhaust_temp': {
    'description': 'Exhaust temperature from DS18B20 (DIGIT sensor is unreliable)',
    'method': 'copy',
    'source': ('ds18b20_vars', 'ds_exhaust_temp'),
}
```

**Rationale**: The DIGIT exhaust sensor has been found to provide incorrect values. The DS18B20 is mounted close to the heat exchanger surface (sensitive to ice presence) and is the reliable source. During defrost, the DS18B20 sensor near the surface stays near 0°C while ice is present, so using this single reliable source is critical.

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
