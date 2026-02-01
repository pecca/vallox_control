import { http, Request, Response } from '@google-cloud/functions-framework';
import { Firestore } from '@google-cloud/firestore';

const firestore = new Firestore();

// Vallox API configuration
const VALLOX_API_URL = process.env.VALLOX_API_URL || 'http://91.157.190.137:9000';
const VALLOX_API_TOKEN = process.env.VALLOX_API_TOKEN || '';

// Firestore collections
const STATE_DOC = 'ai_defrost/state';
const PREDICTIONS_COLLECTION = 'ai_predictions';

// Defrost algorithm thresholds (matching C firmware)
const DEFROST_START_LEVEL = 73;        // Filtered efficiency must drop below this (%)
const DEFROST_TARGET_IN_EFF = 85;      // Fan stop ends when filtered eff exceeds this (%)

// Prediction mode: 'rules' or 'vertex_ai' (future)
const PREDICTION_MODE = process.env.PREDICTION_MODE || 'rules';

// Defrost states
const STATE_MEASURING = 0;
const STATE_HEATING = 1; 
const STATE_STOPPED = 2; 
const STATE_FAN_STOP = 3;

// Defrost mode in C firmware
const DEFROST_MODE_AI = 3;

interface SensorData {
    // New Features
    inEfficiency: number | null;
    inEfficiencyFiltered: number | null;
    outsideTemp: number | null;
    exhaustTemp: number | null;
    exhaustHumidity: number | null; // rh1_sensor
    supplyTemp: number | null;      // incoming_temp
    pressureDiff: number | null;
    fanSpeed: number | null;
    dewPointDelta: number | null;   // Calculated: exhaust - dew_point
    
    // Internal / Config
    defrostMode: number | null;
    fireplaceMode: number | null;   // 1=Active, 0=Inactive
    minExhaustTemp: number | null;  // Threshold for stopping heating in Fireplace mode
}

interface AiState {
    defrost_state: number;
    heating_start_time: number;  // Unix timestamp, 0 if not heating
    updated: Date;
}

interface PredictResult {
    action: 'none' | 'start_fan_stop' | 'stop_fan_stop' | 'start_heating' | 'stop_heating';
    reason: string;
    confidence: number;
}

// --- Vallox API communication ---

async function fetchFromApi(type: string): Promise<Record<string, any>> {
    const url = `${VALLOX_API_URL}/api/vallox/status?type=${type}&token=${VALLOX_API_TOKEN}`;
    const resp = await fetch(url, { signal: AbortSignal.timeout(10000) });
    if (!resp.ok) {
        throw new Error(`API error fetching ${type}: ${resp.status} ${resp.statusText}`);
    }
    return resp.json() as Promise<Record<string, any>>;
}

async function sendCommand(variable: string, value: number): Promise<void> {
    const url = `${VALLOX_API_URL}/api/vallox/control?token=${VALLOX_API_TOKEN}`;
    const resp = await fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ type: 'control_vars', variable, value }),
        signal: AbortSignal.timeout(10000),
    });
    if (!resp.ok) {
        throw new Error(`API error sending ${variable}=${value}: ${resp.status} ${resp.statusText}`);
    }
    console.log(`Command sent: ${variable}=${value}`);
}

// --- State persistence in Firestore ---

async function loadState(): Promise<AiState> {
    const doc = await firestore.doc(STATE_DOC).get();
    if (doc.exists) {
        return doc.data() as AiState;
    }
    return { defrost_state: STATE_MEASURING, heating_start_time: 0, updated: new Date() };
}

async function saveState(state: AiState): Promise<void> {
    state.updated = new Date();
    await firestore.doc(STATE_DOC).set(state);
}

// --- Sensor data extraction ---

function extractValue(vars: Record<string, any>, key: string): number | null {
    const val = vars?.[key]?.value;
    if (val === undefined || val === null) return null;
    return Number(val);
}

function extractSensorData(
    controlVars: Record<string, any>,
    digitVars: Record<string, any>,
    ds18b20Vars: Record<string, any>,
): SensorData {
    const exhaustTemp = extractValue(ds18b20Vars, 'ds_exhaust_temp');
    const dewPoint = extractValue(controlVars, 'dew_point');
    const dsOutsideTemp = extractValue(ds18b20Vars, 'ds_outside_temp'); 
    
    // Calculate Dew Point Delta
    let dewPointDelta: number | null = null;
    if (exhaustTemp !== null && dewPoint !== null) {
        dewPointDelta = Number((exhaustTemp - dewPoint).toFixed(2));
    }

    return {
        inEfficiency: extractValue(controlVars, 'in_efficiency'),
        inEfficiencyFiltered: extractValue(controlVars, 'in_efficiency_filtered'),
        outsideTemp: dsOutsideTemp, 
        exhaustTemp,                
        exhaustHumidity: extractValue(digitVars, 'rh1_sensor'),
        supplyTemp: extractValue(digitVars, 'incoming_temp'),
        pressureDiff: extractValue(controlVars, 'pressure_diff'),
        fanSpeed: extractValue(digitVars, 'cur_fan_speed'),
        dewPointDelta,
        defrostMode: extractValue(controlVars, 'defrost_mode'),
        fireplaceMode: extractValue(controlVars, 'fireplace_mode'),
        minExhaustTemp: extractValue(controlVars, 'min_exhaust_temp'),
    };
}

// --- Defrost algorithm ---

function ruleBasedPredict(sensors: SensorData, defrostState: number): PredictResult {
    const { inEfficiency, inEfficiencyFiltered, fireplaceMode, exhaustTemp, minExhaustTemp } = sensors;

    if (inEfficiency === null || inEfficiencyFiltered === null) {
        return { action: 'none', reason: 'missing_efficiency_data', confidence: 0 };
    }

    // 1. MEASURING STATE
    if (defrostState === STATE_MEASURING) {
        const belowThreshold = inEfficiencyFiltered < DEFROST_START_LEVEL;
        const trendingDown = inEfficiency < inEfficiencyFiltered;

        if (!belowThreshold) {
            return { action: 'none', reason: 'efficiency_ok', confidence: 0 };
        }
        if (!trendingDown) {
            return { action: 'none', reason: 'efficiency_low_but_recovering', confidence: 0.3 };
        }

        const effDrop = DEFROST_START_LEVEL - inEfficiencyFiltered;
        const confidence = Math.min(0.5 + effDrop / 20, 1.0);

        // FIREPLACE OVERRIDE: Use Heating instead of Fan Stop
        if (fireplaceMode === 1) {
            return {
                action: 'start_heating',
                reason: `fireplace_active eff=${inEfficiencyFiltered.toFixed(1)}%`,
                confidence,
            };
        }

        // Standard One-Phase: Start Fan Stop
        return {
            action: 'start_fan_stop',
            reason: `filtered_eff=${inEfficiencyFiltered.toFixed(1)}% raw_eff=${inEfficiency.toFixed(1)}%`,
            confidence,
        };
    }

    // 2. FAN STOP STATE (Standard)
    if (defrostState === STATE_FAN_STOP) {
        // Fireplace Safety: If activated during fan stop, switch to heating
        if (fireplaceMode === 1) {
             return {
                 action: 'start_heating',
                 reason: 'fireplace_activated_during_fan_stop',
                 confidence: 1.0
             };
        }

        if (inEfficiencyFiltered > DEFROST_TARGET_IN_EFF) {
            return {
                action: 'stop_fan_stop',
                reason: `filtered_eff_recovered=${inEfficiencyFiltered.toFixed(1)}%`,
                confidence: 1.0,
            };
        }
        return {
            action: 'none',
            reason: `fan_stop_waiting filtered_eff=${inEfficiencyFiltered.toFixed(1)}%`,
            confidence: 0,
        };
    }

    // 3. HEATING STATE (Fireplace Mode)
    if (defrostState === STATE_HEATING) {
        if (fireplaceMode === 0) {
            // Fireplace turned off? Fallback to Fan Stop (One-Phase)
             return {
                 action: 'start_fan_stop',
                 reason: 'fireplace_deactivated_switching_to_fan_stop',
                 confidence: 1.0
             };
        }

        // Stop condition: Exhaust Temp > Min Limit
        const limit = minExhaustTemp || 3.0; // Default 3C if missing
        if (exhaustTemp !== null && exhaustTemp > limit) {
             return {
                 action: 'stop_heating',
                 reason: `exhaust_temp_target_reached ${exhaustTemp} > ${limit}`,
                 confidence: 1.0
             };
        }

        return {
             action: 'none',
             reason: `heating_active exhaust=${exhaustTemp}`,
             confidence: 0
        };
    }

    return { action: 'none', reason: 'unknown_state', confidence: 0 };
}

// --- Logging ---

async function logPrediction(
    sensors: SensorData,
    defrostState: number,
    result: PredictResult,
): Promise<void> {
    try {
        await firestore.collection(PREDICTIONS_COLLECTION).add({
            timestamp: new Date(),
            mode: PREDICTION_MODE,
            defrost_state: defrostState,
            sensors,
            prediction: result,
        });
    } catch (err) {
        console.error('Failed to log prediction:', err);
    }
}

// --- Main entry point ---

http('predictDefrost', async (req: Request, res: Response) => {
    try {
        // 1. Fetch current data
        const [controlVars, digitVars, ds18b20Vars] = await Promise.all([
            fetchFromApi('control_vars'),
            fetchFromApi('digit_vars'),
            fetchFromApi('ds18b20_vars'),
        ]);

        const sensors = extractSensorData(controlVars, digitVars, ds18b20Vars);

        if (sensors.defrostMode !== DEFROST_MODE_AI) {
            const state = await loadState();
            if (state.defrost_state !== STATE_MEASURING) {
                await saveState({ defrost_state: STATE_MEASURING, heating_start_time: 0, updated: new Date() });
                console.log('Not in AI mode, state reset to measuring');
            }
            res.json({ action: 'none', reason: `defrost_mode=${sensors.defrostMode} (not AI)` });
            return;
        }

        // 2. Load state
        const state = await loadState();

        // 3. Run algorithm
        const result = ruleBasedPredict(sensors, state.defrost_state);

        console.log(`[${PREDICTION_MODE}] state=${state.defrost_state} action=${result.action} reason=${result.reason}`);

        // 4. Execute action
        if (result.action === 'start_fan_stop') {
            await sendCommand('ai_defrost_heating', 0);
            await sendCommand('ai_defrost_fan_stop', 1);
            state.defrost_state = STATE_FAN_STOP;
            state.heating_start_time = 0;
            console.log('Defrost Fan Stop started');
        } 
        else if (result.action === 'stop_fan_stop') {
            await sendCommand('ai_defrost_fan_stop', 0);
            state.defrost_state = STATE_MEASURING;
            state.heating_start_time = 0;
            console.log('Fan stop ended');
        }
        else if (result.action === 'start_heating') {
            await sendCommand('ai_defrost_fan_stop', 0); // Ensure fan stop is OFF
            await sendCommand('ai_defrost_heating', 1);
            state.defrost_state = STATE_HEATING;
            state.heating_start_time = Date.now() / 1000;
            console.log('Defrost Heating started (Fireplace Mode)');
        }
        else if (result.action === 'stop_heating') {
            await sendCommand('ai_defrost_heating', 0);
            state.defrost_state = STATE_MEASURING; // Or Stopped (Cooldown)?
            // For now, go to Measuring. The C firmware handles cooldown if needed.
            if (sensors.fireplaceMode === 1) {
                // In fireplace mode, we might want to respect cooldown?
                // The algorithm doesn't track cooldown, the C firmware does.
                // Cloud just sends commands.
            }
            state.heating_start_time = 0;
            console.log('Heating stopped');
        }

        // 5. Save state + log
        await saveState(state);
        if (result.action !== 'none') {
            await logPrediction(sensors, state.defrost_state, result);
        }

        res.json(result);
    } catch (err: any) {
        console.error('predictDefrost error:', err?.message || err);
        res.status(500).json({ action: 'none', reason: 'error', error: err?.message });
    }
});
