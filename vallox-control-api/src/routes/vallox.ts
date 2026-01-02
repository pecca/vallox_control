import { Hono } from 'hono';
import { z } from 'zod';
import { zValidator } from '@hono/zod-validator';
import { sendReceiveMessage } from '../vallox/client.js';
import { config } from '../config.js';

const app = new Hono();

// Auth middleware
app.use('*', async (c, next) => {
    const token = c.req.query('token');
    if (token !== config.TOKEN) {
        return c.json({ error: "not authorized" }, 404);
    }
    await next();
});

// Legacy Endpoint for backwards compatibility
const legacySchema = z.object({
    action: z.enum(['get', 'set']),
    type: z.string().optional(),
    variable: z.string().optional(),
    value: z.string().optional(),
});

app.get('/', zValidator('query', legacySchema, (result, c) => {
    if (!result.success) {
        return c.json({ error: "invalid URL parameters" }, 500);
    }
}), async (c) => {
    const { action, type, variable, value } = c.req.valid('query');

    let message;
    if (action === 'get') {
        if (!type) return c.json({ error: "invalid URL parameters" }, 500);
        message = { id: 0, get: type };
    } else if (action === 'set') {
        if (!type || !variable || !value) return c.json({ error: "invalid URL parameters" }, 500);
        message = {
            id: 0,
            set: {
                [type]: {
                    [variable]: Number(value)
                }
            }
        };
    }

    if (!message) {
        return c.json({ error: "invalid URL parameters" }, 500);
    }

    try {
        const response = await sendReceiveMessage(message);
        return c.json(response);
    } catch (error) {
        return c.json(error, 500);
    }
});

// Modern RESTful-ish Endpoints
app.get('/status', async (c) => {
    const type = c.req.query('type') || 'digit_vars';
    try {
        const response = await sendReceiveMessage({ id: 0, get: type });
        return c.json(response);
    } catch (error) {
        return c.json(error, 500);
    }
});

const controlSchema = z.object({
    type: z.string().default('digit_vars'),
    variable: z.string(),
    value: z.number(),
});

app.post('/control', zValidator('json', controlSchema), async (c) => {
    const { type, variable, value } = c.req.valid('json');
    try {
        const response = await sendReceiveMessage({
            id: 0,
            set: { [type]: { [variable]: value } }
        });
        return c.json(response);
    } catch (error) {
        return c.json(error, 500);
    }
});

export default app;
