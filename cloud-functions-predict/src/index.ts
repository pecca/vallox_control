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
const DEFROST_HEATING_MIN = 10 * 60;   // Min heating duration (seconds)
const DEFROST_HEATING_MAX = 15 * 60;   // Max heating duration (seconds)

// Prediction mode: 'rules' or 'vertex_ai' (future)
const PREDICTION_MODE = process.env.PREDICTION_MODE || 'rules';

// Defrost states
const STATE_MEASURING = 0;
const STATE_HEATING = 1;
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
    
    // Internal
    defrostMode: number | null;
}

interface AiState {
    defrost_state: number;
    heating_start_time: number;  // Unix timestamp, 0 if not heating
    updated: Date;
}

interface PredictResult {
    action: 'none' | 'start_heating' | 'stop_heating_start_fan_stop' | 'stop_fan_stop';
    reason: string;
    confidence: number;
    heating_duration: number;
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
): SensorData {
    const exhaustTemp = extractValue(digitVars, 'exhaust_temp');
    const dewPoint = extractValue(controlVars, 'dew_point');
    const outsideTemp = extractValue(digitVars, 'outside_temp');
    const pressureDiff = extractValue(controlVars, 'pressure_diff');
    
    // Calculate Dew Point Delta
    let dewPointDelta: number | null = null;
    if (exhaustTemp !== null && dewPoint !== null) {
        dewPointDelta = Number((exhaustTemp - dewPoint).toFixed(2));
    }

    return {
        inEfficiency: extractValue(controlVars, 'in_efficiency'),
        inEfficiencyFiltered: extractValue(controlVars, 'in_efficiency_filtered'),
        outsideTemp,
        exhaustTemp,
        exhaustHumidity: extractValue(digitVars, 'rh1_sensor'),
        supplyTemp: extractValue(digitVars, 'incoming_temp'),
        pressureDiff,
        fanSpeed: extractValue(digitVars, 'cur_fan_speed'),
        dewPointDelta,
        defrostMode: extractValue(controlVars, 'defrost_mode'),
    };
}

// --- Defrost algorithm ---

/**
 * Rule-based defrost decision.
 * Replicates the AUTO mode logic from c/ctrl_logic.c:
 *
 * Cycle: Measuring → Heating (10-15 min) → Fan Stop (until filtered eff > 85%) → Measuring
 */
function ruleBasedPredict(sensors: SensorData, defrostState: number, heatingElapsed: number): PredictResult {
    const { inEfficiency, inEfficiencyFiltered } = sensors;

    if (inEfficiency === null || inEfficiencyFiltered === null) {
        return { action: 'none', reason: 'missing_efficiency_data', confidence: 0, heating_duration: 0 };
    }

    if (defrostState === STATE_MEASURING) {
        const belowThreshold = inEfficiencyFiltered < DEFROST_START_LEVEL;
        const trendingDown = inEfficiency < inEfficiencyFiltered;

        if (!belowThreshold) {
            return { action: 'none', reason: 'efficiency_ok', confidence: 0, heating_duration: 0 };
        }
        if (!trendingDown) {
            return { action: 'none', reason: 'efficiency_low_but_recovering', confidence: 0.3, heating_duration: 0 };
        }

        const effDrop = DEFROST_START_LEVEL - inEfficiencyFiltered;
        const dropFraction = Math.min(effDrop / 30, 1.0);
        const heatingDuration = Math.round(
            DEFROST_HEATING_MIN + (DEFROST_HEATING_MAX - DEFROST_HEATING_MIN) * dropFraction
        );
        const confidence = Math.min(0.5 + effDrop / 20, 1.0);

        return {
            action: 'start_heating',
            reason: `filtered_eff=${inEfficiencyFiltered.toFixed(1)}% raw_eff=${inEfficiency.toFixed(1)}%`,
            confidence,
            heating_duration: heatingDuration,
        };
    }

    if (defrostState === STATE_HEATING) {
        if (heatingElapsed >= DEFROST_HEATING_MAX) {
            return {
                action: 'stop_heating_start_fan_stop',
                reason: `heating_max_reached ${heatingElapsed}s`,
                confidence: 1.0,
                heating_duration: 0,
            };
        }
        if (heatingElapsed >= DEFROST_HEATING_MIN && inEfficiency > DEFROST_TARGET_IN_EFF) {
            return {
                action: 'stop_heating_start_fan_stop',
                reason: `eff_recovered_during_heating eff=${inEfficiency.toFixed(1)}%`,
                confidence: 1.0,
                heating_duration: 0,
            };
        }
        return { action: 'none', reason: `heating ${heatingElapsed}s`, confidence: 0, heating_duration: 0 };
    }

    if (defrostState === STATE_FAN_STOP) {
        if (inEfficiencyFiltered > DEFROST_TARGET_IN_EFF) {
            return {
                action: 'stop_fan_stop',
                reason: `filtered_eff_recovered=${inEfficiencyFiltered.toFixed(1)}%`,
                confidence: 1.0,
                heating_duration: 0,
            };
        }
        return {
            action: 'none',
            reason: `fan_stop_waiting filtered_eff=${inEfficiencyFiltered.toFixed(1)}%`,
            confidence: 0,
            heating_duration: 0,
        };
    }

    return { action: 'none', reason: 'unknown_state', confidence: 0, heating_duration: 0 };
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

/**
 * Cloud Function triggered by Cloud Scheduler (every 60s) or HTTP.
 *
 * 1. Fetches current sensor data from Vallox API
 * 2. Loads AI defrost state from Firestore
 * 3. Runs defrost algorithm
 * 4. Sends commands to Vallox API if needed
 * 5. Saves updated state to Firestore
 */
http('predictDefrost', async (req: Request, res: Response) => {
    try {
        // 1. Fetch current data from Vallox API
        const [controlVars, digitVars] = await Promise.all([
            fetchFromApi('control_vars'),
            fetchFromApi('digit_vars'),
        ]);

        const sensors = extractSensorData(controlVars, digitVars);

        // Check firmware is in AI mode
        if (sensors.defrostMode !== DEFROST_MODE_AI) {
            // Reset state if not in AI mode
            const state = await loadState();
            if (state.defrost_state !== STATE_MEASURING) {
                await saveState({ defrost_state: STATE_MEASURING, heating_start_time: 0, updated: new Date() });
                console.log('Not in AI mode, state reset to measuring');
            }
            res.json({ action: 'none', reason: `defrost_mode=${sensors.defrostMode} (not AI)` });
            return;
        }

        // 2. Load state from Firestore
        const state = await loadState();
        const now = Date.now() / 1000;
        const heatingElapsed = (state.defrost_state === STATE_HEATING && state.heating_start_time > 0)
            ? Math.round(now - state.heating_start_time)
            : 0;

        // 3. Run algorithm
        const result = ruleBasedPredict(sensors, state.defrost_state, heatingElapsed);

        console.log(`[${PREDICTION_MODE}] state=${state.defrost_state} action=${result.action} reason=${result.reason}`);

        // 4. Execute action via Vallox API
        if (result.action === 'start_heating') {
            await sendCommand('ai_defrost_heating', 1);
            state.defrost_state = STATE_HEATING;
            state.heating_start_time = now;
            console.log('Defrost heating started');
        } else if (result.action === 'stop_heating_start_fan_stop') {
            await sendCommand('ai_defrost_heating', 0);
            await sendCommand('ai_defrost_fan_stop', 1);
            state.defrost_state = STATE_FAN_STOP;
            state.heating_start_time = 0;
            console.log('Heating stopped, fan stop started');
        } else if (result.action === 'stop_fan_stop') {
            await sendCommand('ai_defrost_fan_stop', 0);
            state.defrost_state = STATE_MEASURING;
            state.heating_start_time = 0;
            console.log('Fan stop ended, back to measuring');
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
