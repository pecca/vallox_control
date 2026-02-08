const fs = require('fs');
const path = require('path');
const http = require('http');
const https = require('https');
const url = require('url');

// --- Configuration ---
const VALLOX_API_URL = process.env.VALLOX_API_URL || 'http://91.157.190.137:9000';
const VALLOX_API_TOKEN = process.env.VALLOX_API_TOKEN || 'huuhaa';
const CLOUD_AI_URL = process.env.CLOUD_AI_URL || 'https://predictdefrost-2hdewsdtgq-lz.a.run.app';
const INTERVAL = 60000; // 60 seconds

// Defrost modes
const DEFROST_MODE_AI = 3;

/**
 * Generic HTTP Request Helper (Node 10 Compatible)
 */
function request(method, path, body, overrideUrl) {
    return new Promise((resolve, reject) => {
        let fullUrl;
        if (overrideUrl) {
            fullUrl = overrideUrl + path;
        } else {
            fullUrl = `${VALLOX_API_URL}${path}${path.includes('?') ? '&' : '?'}token=${VALLOX_API_TOKEN}`;
        }
        
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
 * Main Loop
 */
async function tick() {
    try {
        console.log(`[${new Date().toISOString()}] Tick...`);

        // 1. Fetch sensor data including current Firmware State
        const [controlRes, digitRes, ds18Res] = await Promise.all([
            api.getStatus('control_vars'),
            api.getStatus('digit_vars'),
            api.getStatus('ds18b20_vars')
        ]);

        const cv = controlRes.control_vars || {};
        const dv = digitRes.digit_vars || {};
        const ds = ds18Res.ds18b20_vars || {};

        const currentDefrostState = getVal(cv, 'defrost_state') || 0; // 0=Measuring, 1=Heating, etc.

        const sensorPayload = {
            defrostMode: getVal(cv, 'defrost_mode'),
            inEfficiency: getVal(cv, 'in_efficiency'),
            inEfficiencyFiltered: getVal(cv, 'in_efficiency_filtered'),
            fireplaceMode: getVal(cv, 'fireplace_mode'),
            minExhaustTemp: getVal(cv, 'min_exhaust_temp'),
            outsideTemp: getVal(ds, 'ds_outside_temp'),
            exhaustTemp: getVal(ds, 'ds_exhaust_temp'),
            exhaustHumidity: getVal(dv, 'rh1_sensor'),
            supplyTemp: getVal(dv, 'incoming_temp'),
            fanSpeed: getVal(dv, 'cur_fan_speed'),
            pressureDiff: getVal(cv, 'pressure_diff'),
            dewPointDelta: 0, // Will be calculated by Cloud
        };

        // 2. Monitoring: Always consult Cloud AI
        let decision;
        try {
            // We pass the Firmware's state to the Cloud so it knows context
            const statePayload = { 
                defrost_state: currentDefrostState,
                updated: new Date()
            };

            decision = await request('POST', '', { 
                sensors: sensorPayload,
                state: statePayload
            }, CLOUD_AI_URL);
            
            console.log(`Cloud AI says: needed=${decision.defrost_needed} (${decision.reason})`);

        } catch (err) {
            console.error("Cloud AI unreachable:", err.message);
            
            // Safety Fallback: Only if we are in AI Control Mode
            if (sensorPayload.defrostMode === DEFROST_MODE_AI) {
                console.log("CRITICAL: Falling back to Firmware AUTO mode for safety!");
                await api.control('defrost_mode', 0); // 0 = Auto
            }
            return;
        }

        // 3. Actuate: Always mirror the AI decision to the firmware variable
        // Even if we are in Auto mode, this variable is logged for monitoring.
        const targetValue = decision.defrost_needed ? 1 : 0;
        
        // Optimization: Check current value to avoid spamming POSTs?
        // But for robust monitoring, sending it every minute is fine.
        await api.control('ai_defrost_heating', targetValue);

        // We do NOT touch ai_defrost_fan_stop anymore. Firmware handles it.

    } catch (err) {
        console.error("Tick failed:", err.message);
    }
}

console.log("Vallox RPi Predictor Started (Monitoring + Single Command Mode)");
setInterval(tick, INTERVAL);
tick();
