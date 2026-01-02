import dgram from 'node:dgram';
import { Buffer } from 'node:buffer';
import { config } from '../config.js';

const UDP_TIMEOUT = 10000;

export interface ValloxMessage {
    id: number;
    get?: string;
    set?: Record<string, Record<string, number>>;
}

export const sendReceiveMessage = (messageSend: ValloxMessage): Promise<any> => {
    return new Promise((resolve, reject) => {
        const message = Buffer.from(JSON.stringify(messageSend));
        const client = dgram.createSocket('udp4');

        // Bind to the port mentioned in the original code, 
        // though usually UDP clients use random ephemeral ports.
        // Keeping it as is for compatibility.
        try {
            client.bind(config.VALLOX_CONTROL_PORT);
        } catch (err) {
            return reject({ error: "failed to bind socket", details: err });
        }

        client.send(message, config.VALLOX_CONTROL_PORT, config.VALLOX_CONTROL_IP, (err) => {
            if (err) {
                client.close();
                return reject({ error: "failed to send message", details: err });
            }
        });

        const receiveTimeout = setTimeout(() => {
            client.close();
            reject({ error: "device not responding" });
        }, UDP_TIMEOUT);

        client.once('message', (msg) => {
            client.close();
            clearTimeout(receiveTimeout);
            try {
                resolve(JSON.parse(msg.toString()));
            } catch (err) {
                reject({ error: "failed to parse device response", details: err });
            }
        });

        client.on('error', (err) => {
            client.close();
            clearTimeout(receiveTimeout);
            reject({ error: "UDP socket error", details: err });
        });
    });
};
