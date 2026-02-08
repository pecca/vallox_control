'use client';

import { useState } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import Stack from '@mui/material/Stack';
import Alert from '@mui/material/Alert';
import { saveAiConfig } from '../actions/vallox';
import type { AiConfig } from '../lib/schemas';

interface AiSettingsProps {
    config: AiConfig;
}

export default function AiSettings({ config }: AiSettingsProps) {
    const [startLimit, setStartLimit] = useState(config.guardrailStartLimit);
    const [stopLimit, setStopLimit] = useState(config.guardrailStopLimit);
    const [loading, setLoading] = useState(false);
    const [message, setMessage] = useState<{ type: 'success' | 'error', text: string } | null>(null);

    const handleSave = async () => {
        setLoading(true);
        setMessage(null);
        try {
            await saveAiConfig({
                guardrailStartLimit: Number(startLimit),
                guardrailStopLimit: Number(stopLimit)
            });
            setMessage({ type: 'success', text: 'Settings saved successfully' });
        } catch (err: any) {
            setMessage({ type: 'error', text: 'Failed to save settings: ' + err.message });
        } finally {
            setLoading(false);
        }
    };

    return (
        <Card sx={{
            background: 'rgba(255, 255, 255, 0.05)',
            border: '1px solid rgba(255, 255, 255, 0.1)',
            height: '100%'
        }}>
            <CardContent>
                <Typography variant="h6" gutterBottom>
                    AI Guardrails
                </Typography>
                
                <Stack spacing={3}>
                    <TextField
                        label="Start Guardrail Limit (%)"
                        type="number"
                        value={startLimit}
                        onChange={(e) => setStartLimit(Number(e.target.value))}
                        helperText="Prevent defrost if filtered efficiency > this value (Rules/Measuring)"
                        InputProps={{ inputProps: { min: 0, max: 100 } }}
                        variant="outlined"
                        size="small"
                        fullWidth
                    />

                    <TextField
                        label="Stop Guardrail Limit (%)"
                        type="number"
                        value={stopLimit}
                        onChange={(e) => setStopLimit(Number(e.target.value))}
                        helperText="Stop defrost if raw efficiency > this value (Defrosting)"
                        InputProps={{ inputProps: { min: 0, max: 100 } }}
                        variant="outlined"
                        size="small"
                        fullWidth
                    />

                    {message && (
                        <Alert severity={message.type}>{message.text}</Alert>
                    )}

                    <Box sx={{ display: 'flex', justifyContent: 'flex-end' }}>
                        <Button 
                            variant="contained" 
                            color="primary" 
                            onClick={handleSave}
                            disabled={loading}
                        >
                            {loading ? 'Saving...' : 'Save Settings'}
                        </Button>
                    </Box>
                </Stack>
            </CardContent>
        </Card>
    );
}
