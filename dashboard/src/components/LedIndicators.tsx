'use client';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';

interface LedIndicatorsProps {
  leds: Record<string, number> | undefined;
}

export default function LedIndicators({ leds }: LedIndicatorsProps) {
  if (!leds) return null;

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          Panel LEDs
        </Typography>
        <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 2 }}>
          {Object.entries(leds).map(([name, on]) => (
            <Box key={name} sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
              <Box
                sx={{
                  width: 14,
                  height: 14,
                  borderRadius: '50%',
                  bgcolor: on ? '#4caf50' : '#616161',
                  boxShadow: on ? '0 0 8px #4caf50' : 'none',
                }}
              />
              <Typography variant="body2">{name}</Typography>
            </Box>
          ))}
        </Box>
      </CardContent>
    </Card>
  );
}
