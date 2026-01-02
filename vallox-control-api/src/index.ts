import { serve } from '@hono/node-server';
import { Hono } from 'hono';
import { logger } from 'hono/logger';
import { config } from './config.js';
import valloxRoutes from './routes/vallox.js';

const app = new Hono();

app.use('*', logger());

app.route('/api/vallox', valloxRoutes);

console.log(`🚀 Server is running on port ${config.PORT}`);

serve({
    fetch: app.fetch,
    port: config.PORT,
});
