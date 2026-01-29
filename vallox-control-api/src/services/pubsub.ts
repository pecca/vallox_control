import crypto from 'crypto';
import https from 'https';
import fs from 'fs';
import { config } from '../config';

interface ServiceAccountKey {
    client_email: string;
    private_key: string;
}

let cachedToken: { token: string; expiresAt: number } | null = null;
let serviceAccountKey: ServiceAccountKey | null = null;

function getServiceAccountKey(): ServiceAccountKey {
    if (!serviceAccountKey) {
        const keyPath = config.GCP_SERVICE_ACCOUNT_KEY_PATH;
        const raw = fs.readFileSync(keyPath, 'utf8');
        serviceAccountKey = JSON.parse(raw);
    }
    return serviceAccountKey!;
}

function base64url(input: string | Buffer): string {
    const b64 = (typeof input === 'string' ? Buffer.from(input) : input).toString('base64');
    return b64.replace(/=/g, '').replace(/\+/g, '-').replace(/\//g, '_');
}

function createJwt(email: string, privateKey: string): string {
    const now = Math.floor(Date.now() / 1000);
    const header = base64url(JSON.stringify({ alg: 'RS256', typ: 'JWT' }));
    const payload = base64url(JSON.stringify({
        iss: email,
        scope: 'https://www.googleapis.com/auth/pubsub',
        aud: 'https://oauth2.googleapis.com/token',
        iat: now,
        exp: now + 3600,
    }));
    const signInput = `${header}.${payload}`;
    const sign = crypto.createSign('RSA-SHA256');
    sign.update(signInput);
    const signature = base64url(sign.sign(privateKey));
    return `${signInput}.${signature}`;
}

async function getAccessToken(): Promise<string> {
    if (cachedToken && Date.now() < cachedToken.expiresAt - 60000) {
        return cachedToken.token;
    }

    const key = getServiceAccountKey();
    const jwt = createJwt(key.client_email, key.private_key);

    const body = `grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=${jwt}`;

    const token = await new Promise<string>((resolve, reject) => {
        const req = https.request({
            hostname: 'oauth2.googleapis.com',
            path: '/token',
            method: 'POST',
            headers: {
                'Content-Type': 'application/x-www-form-urlencoded',
                'Content-Length': Buffer.byteLength(body),
            },
        }, (res) => {
            let data = '';
            res.on('data', (chunk: string) => data += chunk);
            res.on('end', () => {
                try {
                    const parsed = JSON.parse(data);
                    if (parsed.access_token) {
                        resolve(parsed.access_token);
                    } else {
                        reject(new Error(`Token error: ${data}`));
                    }
                } catch (e) {
                    reject(new Error(`Failed to parse token response: ${data}`));
                }
            });
        });
        req.on('error', reject);
        req.write(body);
        req.end();
    });

    cachedToken = { token, expiresAt: Date.now() + 3600 * 1000 };
    return token;
}

export const publishMessage = async (topicName: string, data: object): Promise<string> => {
    const accessToken = await getAccessToken();
    const messageData = Buffer.from(JSON.stringify(data)).toString('base64');

    const postBody = JSON.stringify({
        messages: [{ data: messageData }],
    });

    const result = await new Promise<string>((resolve, reject) => {
        const req = https.request({
            hostname: 'pubsub.googleapis.com',
            path: `/v1/projects/${config.GCP_PROJECT_ID}/topics/${topicName}:publish`,
            method: 'POST',
            headers: {
                'Authorization': `Bearer ${accessToken}`,
                'Content-Type': 'application/json',
                'Content-Length': Buffer.byteLength(postBody),
            },
        }, (res) => {
            let data = '';
            res.on('data', (chunk: string) => data += chunk);
            res.on('end', () => {
                if (res.statusCode === 200) {
                    try {
                        const parsed = JSON.parse(data);
                        resolve((parsed.messageIds && parsed.messageIds[0]) || 'unknown');
                    } catch (e) {
                        reject(new Error(`Failed to parse publish response: ${data}`));
                    }
                } else {
                    reject(new Error(`Pub/Sub publish failed (${res.statusCode}): ${data}`));
                }
            });
        });
        req.on('error', reject);
        req.write(postBody);
        req.end();
    });

    return result;
};
