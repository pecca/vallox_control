import express, { Request, Response, NextFunction } from 'express';
import { z } from 'zod';
import { sendReceiveMessage } from '../vallox/client';
import { config } from '../config';

const router = express.Router();

const authMiddleware = (req: Request, res: Response, next: NextFunction) => {
    const token = req.query.token;
    if (token !== config.TOKEN) {
        return res.status(404).json({ error: "not authorized" });
    }
    next();
};

router.use(authMiddleware);

const legacySchema = z.object({
    action: z.enum(['get', 'set']),
    type: z.string().optional(),
    variable: z.string().optional(),
    value: z.string().optional(),
});

router.get('/', async (req: Request, res: Response) => {
    const result = legacySchema.safeParse(req.query);

    if (!result.success) {
        return res.status(500).json({ error: "invalid URL parameters" });
    }

    const { action, type, variable, value } = result.data;

    let message;
    if (action === 'get') {
        if (!type) return res.status(500).json({ error: "invalid URL parameters" });
        message = { id: 0, get: type };
    } else if (action === 'set') {
        if (!type || !variable || !value) return res.status(500).json({ error: "invalid URL parameters" });
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
        return res.status(500).json({ error: "invalid URL parameters" });
    }

    try {
        const response = await sendReceiveMessage(message);
        return res.json(response);
    } catch (error) {
        return res.status(500).json(error);
    }
});

router.get('/status', async (req: Request, res: Response) => {
    const type = (req.query.type as string) || 'digit_vars';
    try {
        const response = await sendReceiveMessage({ id: 0, get: type });
        return res.json(response);
    } catch (error) {
        return res.status(500).json(error);
    }
});

const controlSchema = z.object({
    type: z.string().optional().default('digit_vars'),
    variable: z.string(),
    value: z.number(),
});

router.post('/control', express.json(), async (req: Request, res: Response) => {
    const result = controlSchema.safeParse(req.body);
    if (!result.success) {
        return res.status(400).json({ error: "invalid request body", details: result.error.format() });
    }

    const { type, variable, value } = result.data;
    try {
        const response = await sendReceiveMessage({
            id: 0,
            set: { [type || 'digit_vars']: { [variable]: value } }
        });
        return res.json(response);
    } catch (error) {
        return res.status(500).json(error);
    }
});

export default router;
