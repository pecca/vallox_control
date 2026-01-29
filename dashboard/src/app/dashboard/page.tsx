'use client';
import { useEffect, useState, useCallback } from 'react';
import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';
import ThermostatIcon from '@mui/icons-material/Thermostat';
import WaterDropIcon from '@mui/icons-material/WaterDrop';
import StatusCard from '@/components/StatusCard';
import FanControl from '@/components/FanControl';
import LedIndicators from '@/components/LedIndicators';
import ControlPanel from '@/components/ControlPanel';
import { getDeviceStatus } from '@/actions/vallox';

function getVal(data: Record<string, any> | null, key: string): number | undefined {
  if (!data) return undefined;
  const entry = data[key];
  if (entry && entry.value !== undefined) return entry.value;
  return undefined;
}

export default function DashboardPage() {
  const [data, setData] = useState<Record<string, any> | null>(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const fetchStatus = useCallback(async () => {
    try {
      const status = await getDeviceStatus();
      setData(status);
      setError(null);
    } catch (err) {
      setError('Failed to fetch device status');
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    fetchStatus();
    const interval = setInterval(fetchStatus, 5000);
    return () => clearInterval(interval);
  }, [fetchStatus]);

  if (loading) {
    return (
      <Box sx={{ display: 'flex', justifyContent: 'center', mt: 8 }}>
        <CircularProgress />
      </Box>
    );
  }

  if (error && !data) {
    return (
      <Typography color="error" sx={{ mt: 4, textAlign: 'center' }}>
        {error}
      </Typography>
    );
  }

  return (
    <Box>
      <Typography variant="h5" sx={{ mb: 3 }}>
        Vallox 150 SE MLV
      </Typography>

      <Grid container spacing={3}>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Inside"
            value={getVal(data, 'inside_temp')}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="error" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Outside"
            value={getVal(data, 'outside_temp')}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="info" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Exhaust"
            value={getVal(data, 'exhaust_temp')}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="warning" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Incoming"
            value={getVal(data, 'incoming_temp')}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="success" />}
          />
        </Grid>

        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Humidity"
            value={getVal(data, 'rh1_sensor')}
            unit="%"
            icon={<WaterDropIcon color="info" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <FanControl currentSpeed={getVal(data, 'cur_fan_speed')} />
        </Grid>

        <Grid size={{ xs: 12 }}>
          <LedIndicators leds={data?.panel_leds?.value} />
        </Grid>

        <Grid size={{ xs: 12 }}>
          <ControlPanel digitVars={data || {}} />
        </Grid>
      </Grid>
    </Box>
  );
}
