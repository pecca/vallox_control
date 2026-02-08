import { http, Request, Response } from '@google-cloud/functions-framework';
import { Firestore } from '@google-cloud/firestore';
import { PredictionServiceClient, helpers } from '@google-cloud/aiplatform';

const firestore = new Firestore();
const predictionClient = new PredictionServiceClient({
    apiEndpoint: 'europe-north1-aiplatform.googleapis.com',
});


// GCP Configuration
const GCP_PROJECT_ID = process.env.GCP_PROJECT_ID || 'pekan-vallox-control';
const GCP_LOCATION = process.env.GCP_LOCATION || 'europe-north1';
const VERTEX_ENDPOINT_ID = process.env.VERTEX_ENDPOINT_ID || ''; // e.g. "1234567890"

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
    defrost_needed: boolean;
    reason: string;
    confidence: number;
}
 
// --- Vallox API communication ---
// (Removed as part of refactor)
 
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
 
// --- Vertex AI Prediction ---
 
async function vertexAiPredict(sensors: SensorData): Promise<PredictResult> {
    if (!VERTEX_ENDPOINT_ID) {
        return { defrost_needed: false, reason: 'vertex_endpoint_not_configured', confidence: 0 };
    }
 
    // Features MUST be in exactly the same order as in ai_config.py's TRAINING_FEATURES
    const featureValues = [
        sensors.outsideTemp || 0,
        sensors.exhaustTemp || 0, // Using ds_exhaust_temp
        sensors.exhaustHumidity || 0,
        sensors.supplyTemp || 0,
        sensors.fanSpeed || 0,
        sensors.dewPointDelta || 0,
        sensors.outsideTemp || 0, // start_ds_outside_temp
        sensors.supplyTemp || 0,  // start_ds_incoming_temp
        sensors.inEfficiency || 0, // start_in_eff
    ];
    
    const instances = [helpers.toValue(featureValues)!];
 
    const endpoint = `projects/${GCP_PROJECT_ID}/locations/${GCP_LOCATION}/endpoints/${VERTEX_ENDPOINT_ID}`;
 
    try {
        const responseArray = await predictionClient.predict({
            endpoint,
            instances,
        }) as any;
        const response = responseArray[0];
 
        if (!response.predictions || response.predictions.length === 0) {
            throw new Error('No predictions returned from Vertex AI');
        }
 
        console.log('Vertex AI Raw Response:', JSON.stringify(response));

        // SKLearn output format can vary. It usually returns the predicted class directly, 
        // OR probabilities if predict_proba was called. 
        // For GradientBoostingClassifier in Vertex AI serving container:
        // It likely returns just the prediction class, or we need to check how to get probabilities.
        
        // Let's first see what we get.
        const prediction = response.predictions[0];
        console.log('Prediction[0]:', prediction);

        let defrostScore = 0;
        
        // Handle different possible formats
        if (Array.isArray(prediction)) {
             // Case: [0.1, 0.9] (probabilities)
             defrostScore = prediction[1]; 
        } else if (typeof prediction === 'number') {
             // Case: 0 or 1 (class label)
             defrostScore = prediction;
        } else if (prediction && prediction.scores) {
             // Case: { scores: [0.1, 0.9], classes: ... }
             defrostScore = prediction.scores[1];
        } else if (prediction && typeof prediction === 'object' && 'numberValue' in prediction) {
             // Case: { numberValue: 1, kind: 'numberValue' } (Protobuf Value)
             defrostScore = prediction.numberValue;
        } else {
             // Unknown structure
             throw new Error(`Unknown prediction format: ${JSON.stringify(prediction)}`);
        }
 
        
        console.log(`Vertex AI Score: ${defrostScore}`);
 
        if (defrostScore > 0.5) {
             return { defrost_needed: true, reason: 'ai_score_high', confidence: defrostScore };
        }
 
        return { defrost_needed: false, reason: 'ai_score_low', confidence: defrostScore };
 
    } catch (err: any) {
        console.error('Vertex AI Prediction failed:', err);
        return { defrost_needed: false, reason: `vertex_ai_error: ${err.message}`, confidence: 0 };
    }
}
 
// --- Defrost algorithm ---
 
function ruleBasedPredict(sensors: SensorData, defrostState: number): PredictResult {
    const { inEfficiency, inEfficiencyFiltered } = sensors;
 
    if (inEfficiency === null || inEfficiencyFiltered === null) {
        return { defrost_needed: false, reason: 'missing_efficiency_data', confidence: 0 };
    }
 
    // 1. MEASURING STATE
    if (defrostState === STATE_MEASURING) {
        const belowThreshold = inEfficiencyFiltered < DEFROST_START_LEVEL;
        const trendingDown = inEfficiency < inEfficiencyFiltered;
 
        if (!belowThreshold) {
            return { defrost_needed: false, reason: 'efficiency_ok', confidence: 0 };
        }
        if (!trendingDown) {
            return { defrost_needed: false, reason: 'efficiency_low_but_recovering', confidence: 0.3 };
        }
 
        const effDrop = DEFROST_START_LEVEL - inEfficiencyFiltered;
        const confidence = Math.min(0.5 + effDrop / 20, 1.0);
 
        return {
            defrost_needed: true,
            reason: `filtered_eff=${inEfficiencyFiltered.toFixed(1)}% raw_eff=${inEfficiency.toFixed(1)}%`,
            confidence,
        };
    }
 
    // 2. DEFROSTING STATE (Any active state: Heating, Fan Stop, etc.)
    // We stay in defrost mode until efficiency recovers.
    if (inEfficiencyFiltered > DEFROST_TARGET_IN_EFF) {
        return {
            defrost_needed: false,
            reason: `filtered_eff_recovered=${inEfficiencyFiltered.toFixed(1)}%`,
            confidence: 1.0,
        };
    }
 
    return {
        defrost_needed: true,
        reason: `defrost_ongoing filtered_eff=${inEfficiencyFiltered.toFixed(1)}%`,
        confidence: 0,
    };
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
        if (req.method !== 'POST' || !req.body || !req.body.sensors) {
             res.status(400).json({ action: 'none', reason: 'invalid_request', error: 'POST with sensor data required' });
             return;
        }

        const sensors: SensorData = req.body.sensors;
        // State is passed by RPi, or default to Measuring if not provided
        const state: AiState = req.body.state || { defrost_state: STATE_MEASURING, heating_start_time: 0, updated: new Date() };



        // 3. Run algorithm
        let result: PredictResult;
        // Vertex AI mode logic will go here once model is ready
        if (PREDICTION_MODE === 'vertex_ai') {
             result = await vertexAiPredict(sensors);
        } else {
             // Fallback to rules if Vertex AI not configured or not in Measuring state
             result = ruleBasedPredict(sensors, state.defrost_state);
        }

        console.log(`[${PREDICTION_MODE}] state=${state.defrost_state} defrost_needed=${result.defrost_needed} reason=${result.reason}`);

        // Log prediction for future training
        if (result.defrost_needed) {
            await logPrediction(sensors, state.defrost_state, result);
        }

        res.json(result);
    } catch (err: any) {
        console.error('predictDefrost error:', err?.message || err);
        res.status(500).json({ defrost_needed: false, reason: 'error', error: err?.message, confidence: 0 });
    }
});
