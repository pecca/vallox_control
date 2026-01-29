import { Firestore } from '@google-cloud/firestore';
import { cloudEvent, CloudEvent } from '@google-cloud/functions-framework';

const firestore = new Firestore();
const COLLECTION = 'vallox_status';

interface PubSubData {
    message: {
        data: string;
    };
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

    const document = {
        timestamp,
        ...statusData,
    };

    await firestore.collection(COLLECTION).doc(docId).set(document);
    console.log(`Stored status document: ${docId}`);
});
