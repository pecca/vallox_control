import { config } from '../config';
import { sendReceiveMessage } from '../vallox/client';
import { publishMessage } from './pubsub';

const pollAndPublish = async (): Promise<void> => {
    try {
        const [digitRes, controlRes, ds18b20Res] = await Promise.all([
            sendReceiveMessage({ id: 0, get: 'digit_vars' }),
            sendReceiveMessage({ id: 0, get: 'control_vars' }),
            sendReceiveMessage({ id: 0, get: 'ds18b20_vars' }),
        ]);

        // Helper to un-nest and normalize keys (C firmware sometimes includes keys with newlines)
        const normalize = (obj: any, key: string) => {
            if (!obj) return {};
            const keys = Object.keys(obj);
            const foundKey = keys.find(k => k.trim() === key);
            const data = foundKey ? obj[foundKey] : obj;
            // Return the inner data or the obj itself if not nested
            // Most device responses are { "digit_vars": { ... } }
            return (data && typeof data === 'object' && !data.value) ? data : obj;
        };

        const status = { 
            digit_vars: normalize(digitRes, 'digit_vars'), 
            control_vars: normalize(controlRes, 'control_vars'), 
            ds18b20_vars: normalize(ds18b20Res, 'ds18b20_vars') 
        };

        const messageId = await publishMessage(config.PUBSUB_TOPIC, status);
        console.log(`[StatusPublisher] Published message ${messageId} to ${config.PUBSUB_TOPIC}`);
    } catch (err) {
        console.error('[StatusPublisher] Failed to poll/publish:', err);
    }
};

export const startStatusPublisher = (): void => {
    if (!config.GCP_PROJECT_ID) {
        console.log('[StatusPublisher] GCP_PROJECT_ID not set, skipping status publishing');
        return;
    }

    console.log(`[StatusPublisher] Starting, polling every ${config.POLL_INTERVAL_MS}ms, topic: ${config.PUBSUB_TOPIC}`);
    setInterval(pollAndPublish, config.POLL_INTERVAL_MS);
    pollAndPublish();
};
