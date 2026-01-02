import express, { Request, Response, NextFunction } from 'express';
import { sendReceiveMessage } from '../vallox/client';
import { config } from '../config';

const router = express.Router();

// Auth middleware
const authMiddleware = (req: Request, res: Response, next: NextFunction) => {
    const token = req.query.token;
    if (token !== config.TOKEN) {
        return res.status(404).json({ error: "not authorized" });
    }
    next();
};

router.use(authMiddleware);

// Legacy Endpoint for backwards compatibility
router.get('/', async (req: Request, res: Response) => {
    const { action, type, variable, value } = req.query as any;

    if (!action || (action !== 'get' && action !== 'set')) {
        return res.status(500).json({ error: "invalid URL parameters" });
    }

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

// Modern RESTful-ish Endpoints
router.get('/status', async (req: Request, res: Response) => {
    const type = (req.query.type as string) || 'digit_vars';
    try {
        const response = await sendReceiveMessage({ id: 0, get: type });
        return res.json(response);
    } catch (error) {
        return res.status(500).json(error);
    }
});

router.post('/control', express.json(), async (req: Request, res: Response) => {
    const { type, variable, value } = req.body;

    if (!variable || value === undefined) {
        return res.status(400).json({ error: "invalid request body", details: "variable and value are required" });
    }

    const finalType = type || 'digit_vars';
    try {
        const response = await sendReceiveMessage({
            id: 0,
            set: { [finalType]: { [variable]: Number(value) } }
        });
        return res.json(response);
    } catch (error) {
        return res.status(500).json(error);
    }
});

export default router;
