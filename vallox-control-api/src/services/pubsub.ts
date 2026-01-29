import { PubSub } from '@google-cloud/pubsub';
import { config } from '../config';

let pubsubClient: PubSub | null = null;

function getClient(): PubSub {
    if (!pubsubClient) {
        pubsubClient = new PubSub({ projectId: config.GCP_PROJECT_ID });
    }
    return pubsubClient;
}

export const publishMessage = async (topicName: string, data: object): Promise<string> => {
    const client = getClient();
    const topic = client.topic(topicName);
    const dataBuffer = Buffer.from(JSON.stringify(data));
    const messageId = await topic.publish(dataBuffer);
    return messageId;
};
