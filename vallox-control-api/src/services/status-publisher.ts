import { config } from '../config';
import { sendReceiveMessage } from '../vallox/client';
import { publishMessage } from './pubsub';

const pollAndPublish = async (): Promise<void> => {
    try {
        const status = await sendReceiveMessage({ id: 0, get: 'digit_vars' });
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
