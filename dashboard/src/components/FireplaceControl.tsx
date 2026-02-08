'use client';
import { useState, useEffect, useTransition } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import Slider from '@mui/material/Slider';
import LocalFireDepartmentIcon from '@mui/icons-material/LocalFireDepartment';
import { setDeviceVariable } from '@/actions/vallox';
import type { ControlVars } from '@/lib/schemas';

interface FireplaceControlProps {
  controlVars: ControlVars | undefined;
}

export default function FireplaceControl({ controlVars }: FireplaceControlProps) {
  const [duration, setDuration] = useState<number>(90); // minutes
  const [isPending, startTransition] = useTransition();
  const active = controlVars?.fireplace_active?.value === 1;
  const remaining = controlVars?.fireplace_remaining?.value ?? 0;

  function toggleFireplace() {
    startTransition(async () => {
      if (active) {
        await setDeviceVariable('control_vars', 'fireplace_off', 0);
      } else {
        // Firmware expects seconds
        await setDeviceVariable('control_vars', 'fireplace_on', duration * 60);
      }
    });
  }

  const formatRemaining = (seconds: number) => {
    const mins = Math.floor(seconds / 60);
    const secs = seconds % 60;
    return `${mins}:${secs.toString().padStart(2, '0')}`;
  };

  return (
    <Card sx={{ 
      background: active 
        ? 'linear-gradient(135deg, #2c1a1a 0%, #1a1a1a 100%)' 
        : 'rgba(255, 255, 255, 0.05)',
      border: active ? '1px solid #f44336' : '1px solid rgba(255, 255, 255, 0.1)',
      position: 'relative',
      overflow: 'hidden'
    }}>
      {active && (
        <Box sx={{
          position: 'absolute',
          top: -10,
          right: -10,
          opacity: 0.1,
          transform: 'rotate(15deg)'
        }}>
          <LocalFireDepartmentIcon sx={{ fontSize: 100, color: '#f44336' }} />
        </Box>
      )}
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
          <LocalFireDepartmentIcon color={active ? 'error' : 'disabled'} />
          <Typography variant="h6">Fireplace Mode</Typography>
        </Box>

        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 2 }}>
          {!active ? (
            <>
              <Typography variant="body2" color="text.secondary">
                Duration: {duration} minutes
              </Typography>
              <Slider
                value={duration}
                onChange={(_, val) => setDuration(val as number)}
                min={15}
                max={180}
                step={15}
                marks
                disabled={isPending}
                sx={{ color: '#f44336' }}
              />
            </>
          ) : (
            <Box sx={{ textAlign: 'center', py: 1 }}>
              <Typography variant="h4" color="error" sx={{ fontWeight: 'bold' }}>
                {formatRemaining(remaining)}
              </Typography>
              <Typography variant="caption" color="text.secondary">
                REMAINING
              </Typography>
            </Box>
          )}

          <Button
            fullWidth
            variant={active ? "contained" : "outlined"}
            color={active ? "error" : "inherit"}
            onClick={toggleFireplace}
            disabled={isPending}
            startIcon={active ? null : <LocalFireDepartmentIcon />}
          >
            {isPending ? 'Working...' : active ? 'STOP FIREPLACE MODE' : 'START FIREPLACE MODE'}
          </Button>
        </Box>
      </CardContent>
    </Card>
  );
}
