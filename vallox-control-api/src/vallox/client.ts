import dgram from 'dgram';
import { Buffer } from 'buffer';
import { config } from '../config';

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

        try {
            client.bind(config.VALLOX_CONTROL_PORT);
        } catch (err) {
            return reject({ error: "failed to bind socket", details: err });
        }

        client.send(message, config.VALLOX_CONTROL_PORT, config.VALLOX_CONTROL_IP, (err: any) => {
            if (err) {
                try { client.close(); } catch (e) { }
                return reject({ error: "failed to send message", details: err });
            }
        });

        const receiveTimeout = setTimeout(() => {
            try { client.close(); } catch (e) { }
            reject({ error: "device not responding" });
        }, UDP_TIMEOUT);

        client.once('message', (msg: any) => {
            try { client.close(); } catch (e) { }
            clearTimeout(receiveTimeout);
            try {
                resolve(JSON.parse(msg.toString()));
            } catch (err) {
                reject({ error: "failed to parse device response", details: err });
            }
        });

        client.on('error', (err: any) => {
            try { client.close(); } catch (e) { }
            clearTimeout(receiveTimeout);
            reject({ error: "UDP socket error", details: err });
        });
    });
};
