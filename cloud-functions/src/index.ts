import { Firestore } from '@google-cloud/firestore';
import { cloudEvent, CloudEvent } from '@google-cloud/functions-framework';

const firestore = new Firestore();
const STATUS_COLLECTION = 'vallox_status';
const CYCLES_COLLECTION = 'defrost_cycles';
const META_DOC = 'defrost_meta/last_cycle';

interface PubSubData {
    message: {
        data: string;
    };
}

async function checkAndStoreDefrostCycle(
    controlVars: Record<string, any>,
    digitVars: Record<string, any>,
    ds18b20Vars: Record<string, any>,
    timestamp: Date
): Promise<void> {
    const cycleCount = controlVars?.defrost_cycle_count?.value;
    if (cycleCount === undefined || cycleCount === 0) return;

    // Check if this is a new cycle
    const metaRef = firestore.doc(META_DOC);
    const metaSnap = await metaRef.get();
    const lastCycleCount = metaSnap.exists ? metaSnap.data()?.cycle_count || 0 : 0;

    if (cycleCount <= lastCycleCount) return;

    // New cycle detected — store episode data
    const cycle = {
        timestamp,
        cycle_number: cycleCount,
        // Timing
        cycle_start: controlVars.defrost_cycle_start?.value || 0,
        heating_duration: controlVars.defrost_heating_duration?.value || 0,
        fan_stop_duration: controlVars.defrost_fan_stop_duration?.value || 0,
        total_duration: controlVars.defrost_total_duration?.value || 0,
        // Start conditions
        start_outside_temp: controlVars.defrost_start_outside_temp?.value,
        start_exhaust_temp: controlVars.defrost_start_exhaust_temp?.value,
        start_incoming_temp: controlVars.defrost_start_incoming_temp?.value,
        start_dew_point: controlVars.defrost_start_dew_point?.value,
        start_in_eff: controlVars.defrost_start_in_eff?.value,
        start_in_eff_filtered: controlVars.defrost_start_in_eff_filtered?.value,
        // End conditions
        end_in_eff: controlVars.defrost_end_in_eff?.value,
        end_incoming_temp: controlVars.defrost_end_incoming_temp?.value,
        end_exhaust_temp: controlVars.defrost_end_exhaust_temp?.value,
        end_reason: controlVars.defrost_end_reason?.value,
        // Context: ambient conditions at poll time
        outside_temp: digitVars?.outside_temp?.value,
        inside_temp: digitVars?.inside_temp?.value,
        humidity: digitVars?.rh1_sensor?.value,
        fan_speed: digitVars?.cur_fan_speed?.value,
        ds_outside_temp: ds18b20Vars?.ds_outside_temp?.value,
    };

    const docId = `cycle-${cycleCount}`;
    await firestore.collection(CYCLES_COLLECTION).doc(docId).set(cycle);
    await metaRef.set({ cycle_count: cycleCount, updated: timestamp });
    console.log(`Stored defrost cycle #${cycleCount}`);
}

cloudEvent<PubSubData>('processValloxStatus', async (event: CloudEvent<PubSubData>) => {
    const base64data = event.data!.message.data;
    const rawData = Buffer.from(base64data, 'base64').toString('utf8');

    let statusData: Record<string, any>;
    try {
        statusData = JSON.parse(rawData);
    } catch (err) {
        console.error('Failed to parse Pub/Sub message:', rawData);
        return;
    }

    const timestamp = new Date();
    const docId = timestamp.toISOString().replace(/[:.]/g, '-');

    // Store regular status snapshot
    await firestore.collection(STATUS_COLLECTION).doc(docId).set({
        timestamp,
        ...statusData,
    });
    console.log(`Stored status document: ${docId}`);

    // Check for completed defrost cycle
    await checkAndStoreDefrostCycle(
        statusData.control_vars,
        statusData.digit_vars,
        statusData.ds18b20_vars,
        timestamp
    );
});
