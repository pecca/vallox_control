import { z } from 'zod';
import 'dotenv/config';

const envSchema = z.object({
    VALLOX_CONTROL_IP: z.string().min(1),
    VALLOX_CONTROL_PORT: z.coerce.number().int().positive(),
    TOKEN: z.string().min(1),
    PORT: z.coerce.number().int().positive().default(3000),
});

const result = envSchema.safeParse(process.env);

if (!result.success) {
    console.error('❌ Invalid environment variables:', result.error.format());
    process.exit(1);
}

export const config = result.data;
