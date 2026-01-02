require('dotenv').config();

function getEnv(key: string, required = true, defaultValue?: any): any {
    const value = process.env[key];
    if (required && (value === undefined || value === '')) {
        console.error(`❌ Missing required environment variable: ${key}`);
        process.exit(1);
    }
    return value !== undefined ? value : defaultValue;
}

export const config = {
    VALLOX_CONTROL_IP: getEnv('VALLOX_CONTROL_IP'),
    VALLOX_CONTROL_PORT: Number(getEnv('VALLOX_CONTROL_PORT')),
    TOKEN: getEnv('TOKEN'),
    PORT: Number(getEnv('PORT', false, 3000)),
};

// Simple validation
if (isNaN(config.VALLOX_CONTROL_PORT) || config.VALLOX_CONTROL_PORT <= 0) {
    console.error('❌ VALLOX_CONTROL_PORT must be a positive number');
    process.exit(1);
}
if (isNaN(config.PORT) || config.PORT <= 0) {
    console.error('❌ PORT must be a positive number');
    process.exit(1);
}
