import express from 'express';
import { config } from './config.js';
import valloxRoutes from './routes/vallox.js';

const app = express();

app.use((req, res, next) => {
    console.log(`${new Date().toISOString()} ${req.method} ${req.url}`);
    next();
});

app.use('/api/vallox', valloxRoutes);

app.listen(config.PORT, () => {
    console.log(`🚀 Server is running on port ${config.PORT} (Express Mode)`);
});
