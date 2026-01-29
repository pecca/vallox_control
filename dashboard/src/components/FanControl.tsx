'use client';
import { useState, useTransition } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Slider from '@mui/material/Slider';
import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import SpeedIcon from '@mui/icons-material/Speed';
import { setDeviceVariable } from '@/actions/vallox';

interface FanControlProps {
  currentSpeed: number | undefined;
}

export default function FanControl({ currentSpeed }: FanControlProps) {
  const [value, setValue] = useState<number>(currentSpeed || 1);
  const [isPending, startTransition] = useTransition();

  function handleSet() {
    startTransition(async () => {
      await setDeviceVariable('FAN_SPEED', value);
    });
  }

  return (
    <Card>
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
          <SpeedIcon color="primary" />
          <Typography variant="h6">Fan Speed</Typography>
        </Box>
        <Typography variant="h3" sx={{ textAlign: 'center', mb: 2 }}>
          {currentSpeed !== undefined ? currentSpeed : '--'}
        </Typography>
        <Slider
          value={value}
          onChange={(_, v) => setValue(v as number)}
          min={1}
          max={8}
          step={1}
          marks
          valueLabelDisplay="auto"
        />
        <Button
          variant="contained"
          fullWidth
          onClick={handleSet}
          disabled={isPending}
          sx={{ mt: 1 }}
        >
          {isPending ? 'Setting...' : `Set to ${value}`}
        </Button>
      </CardContent>
    </Card>
  );
}
