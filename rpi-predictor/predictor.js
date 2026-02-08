const fs = require('fs');
const path = require('path');
const http = require('http');
const https = require('https');
const url = require('url');

// --- Configuration ---
const VALLOX_API_URL = process.env.VALLOX_API_URL || 'http://91.157.190.137:9000';
const VALLOX_API_TOKEN = process.env.VALLOX_API_TOKEN || 'huuhaa';
const INTERVAL = 60000; // 60 seconds
const STATE_FILE = path.join(__dirname, 'defrost_state.json');

// Defrost states (must match C firmware)
const STATE_MEASURING = 0;
const STATE_HEATING = 1;
const STATE_FAN_STOP = 3;
const DEFROST_MODE_AI = 3;

// Thresholds
const DEFROST_START_LEVEL = 73;
const DEFROST_TARGET_IN_EFF = 85;

/**
 * Generic HTTP Request Helper (Node 10 Compatible)
 */
function request(method, path, body) {
    return new Promise((resolve, reject) => {
        const fullUrl = `${VALLOX_API_URL}${path}${path.includes('?') ? '&' : '?'}token=${VALLOX_API_TOKEN}`;
        const parsedUrl = url.parse(fullUrl);
        
        const options = {
            hostname: parsedUrl.hostname,
            port: parsedUrl.port,
            path: parsedUrl.path,
            method: method,
            headers: {
                'Content-Type': 'application/json'
            },
            timeout: 10000
        };

        const lib = parsedUrl.protocol === 'https:' ? https : http;
        const req = lib.request(options, (res) => {
            let data = '';
            res.on('data', (chunk) => data += chunk);
            res.on('end', () => {
                if (res.statusCode >= 200 && res.statusCode < 300) {
                    try { resolve(JSON.parse(data)); } catch (e) { resolve(data); }
                } else {
                    reject(new Error(`API Error: ${res.statusCode} ${data}`));
                }
            });
        });

        req.on('error', reject);
        if (body) req.write(JSON.stringify(body));
        req.end();
    });
}

const api = {
    getStatus: (type) => request('GET', `/api/vallox/status?type=${type}`),
    control: (variable, value) => request('POST', '/api/vallox/control', { type: 'control_vars', variable, value })
};

/**
 * Extracts numeric value from API response
 */
function getVal(obj, key) {
    const v = obj && obj[key];
    if (v === undefined || v === null) return undefined;
    if (typeof v === 'object' && v.value !== undefined) return v.value;
    return v;
}

/**
 * Local Rule-Based Logic
 */
function getRuleDecision(sensors, state) {
    const { in_efficiency, in_efficiency_filtered, fireplace_mode, ds_exhaust_temp, min_exhaust_temp } = sensors;

    if (in_efficiency === undefined || in_efficiency_filtered === undefined) {
        return { action: 'none', reason: 'Missing efficiency data' };
    }

    if (state.defrost_state === STATE_MEASURING) {
        if (in_efficiency_filtered < DEFROST_START_LEVEL && in_efficiency < in_efficiency_filtered) {
            return {
                action: fireplace_mode === 1 ? 'start_heating' : 'start_fan_stop',
                reason: `Efficiency low (${in_efficiency_filtered.toFixed(1)}%)`
            };
        }
        return { action: 'none', reason: 'Efficiency OK' };
    }

    if (state.defrost_state === STATE_FAN_STOP) {
        if (fireplace_mode === 1) return { action: 'start_heating', reason: 'Fireplace activated' };
        if (in_efficiency_filtered > DEFROST_TARGET_IN_EFF) return { action: 'stop_fan_stop', reason: 'Efficiency recovered' };
    }

    if (state.defrost_state === STATE_HEATING) {
        if (fireplace_mode === 0) return { action: 'start_fan_stop', reason: 'Fireplace off' };
        const limit = min_exhaust_temp || 3.0;
        if (ds_exhaust_temp > limit) return { action: 'stop_heating', reason: 'Exhaust target reached' };
    }

    return { action: 'none', reason: 'Waiting' };
}

/**
 * Main Loop
 */
async function tick() {
    try {
        console.log(`[${new Date().toISOString()}] Tick...`);

        // 1. Fetch sensor data
        const [controlRes, digitRes, ds18Res] = await Promise.all([
            api.getStatus('control_vars'),
            api.getStatus('digit_vars'),
            api.getStatus('ds18b20_vars')
        ]);

        const cv = controlRes.control_vars || {};
        const dv = digitRes.digit_vars || {};
        const ds = ds18Res.ds18b20_vars || {};

        const sensors = {
            defrost_mode: getVal(cv, 'defrost_mode'),
            in_efficiency: getVal(cv, 'in_efficiency'),
            in_efficiency_filtered: getVal(cv, 'in_efficiency_filtered'),
            fireplace_mode: getVal(cv, 'fireplace_mode'),
            min_exhaust_temp: getVal(cv, 'min_exhaust_temp'),
            ds_exhaust_temp: getVal(ds, 'ds_exhaust_temp'),
            incoming_temp: getVal(dv, 'incoming_temp')
        };

        if (sensors.defrost_mode !== DEFROST_MODE_AI) {
            console.log("Not in AI mode. Skipping.");
            return;
        }

        // 2. Load State
        let state = { defrost_state: STATE_MEASURING };
        if (fs.existsSync(STATE_FILE)) {
            try { state = JSON.parse(fs.readFileSync(STATE_FILE)); } catch (e) {}
        }

        // 3. Predict
        const decision = getRuleDecision(sensors, state);
        console.log(`State: ${state.defrost_state} | Decision: ${decision.action} (${decision.reason})`);

        // 4. Act
        if (decision.action === 'start_fan_stop') {
            await api.control('ai_defrost_heating', 0);
            await api.control('ai_defrost_fan_stop', 1);
            state.defrost_state = STATE_FAN_STOP;
        } else if (decision.action === 'stop_fan_stop' || decision.action === 'stop_heating') {
            await api.control('ai_defrost_fan_stop', 0);
            await api.control('ai_defrost_heating', 0);
            state.defrost_state = STATE_MEASURING;
        } else if (decision.action === 'start_heating') {
            await api.control('ai_defrost_fan_stop', 0);
            await api.control('ai_defrost_heating', 1);
            state.defrost_state = STATE_HEATING;
        }

        // 5. Save State
        fs.writeFileSync(STATE_FILE, JSON.stringify(state));

    } catch (err) {
        console.error("Tick failed:", err.message);
    }
}

console.log("Vallox RPi Predictor Started (via REST API)");
setInterval(tick, INTERVAL);
tick();
