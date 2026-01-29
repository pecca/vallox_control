'use client';
import { useEffect, useState, useCallback } from 'react';
import Grid from '@mui/material/Grid';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import CircularProgress from '@mui/material/CircularProgress';
import ThermostatIcon from '@mui/icons-material/Thermostat';
import WaterDropIcon from '@mui/icons-material/WaterDrop';
import SensorsIcon from '@mui/icons-material/Sensors';
import StatusCard from '@/components/StatusCard';
import FanControl from '@/components/FanControl';
import LedIndicators from '@/components/LedIndicators';
import ControlPanel from '@/components/ControlPanel';
import { getDeviceStatus } from '@/actions/vallox';
import type { DeviceStatus } from '@/lib/schemas';

export default function DashboardPage() {
  const [data, setData] = useState<DeviceStatus | null>(null);
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

  const dv = data?.digitVars;
  const cv = data?.controlVars;
  const ds = data?.ds18b20Vars;

  return (
    <Box>
      <Typography variant="h5" sx={{ mb: 3 }}>
        Vallox 150 SE MLV
      </Typography>

      <Grid container spacing={3}>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Inside"
            value={dv?.inside_temp.value}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="error" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Outside"
            value={dv?.outside_temp.value}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="info" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Exhaust"
            value={dv?.exhaust_temp.value}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="warning" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Incoming"
            value={dv?.incoming_temp.value}
            unit={'\u00B0C'}
            icon={<ThermostatIcon color="success" />}
          />
        </Grid>

        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Humidity"
            value={dv?.rh1_sensor.value}
            unit="%"
            icon={<WaterDropIcon color="info" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Efficiency (in)"
            value={cv?.in_efficiency_filtered.value}
            unit="%"
            icon={<ThermostatIcon color="primary" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="Efficiency (out)"
            value={cv?.out_efficiency_filtered.value}
            unit="%"
            icon={<ThermostatIcon color="primary" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <FanControl currentSpeed={dv?.cur_fan_speed.value} />
        </Grid>

        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="DS Outside"
            value={ds?.ds_outside_temp.value}
            unit={'\u00B0C'}
            icon={<SensorsIcon color="info" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="DS Exhaust"
            value={ds?.ds_exhaust_temp.value}
            unit={'\u00B0C'}
            icon={<SensorsIcon color="warning" />}
          />
        </Grid>
        <Grid size={{ xs: 12, sm: 6, md: 3 }}>
          <StatusCard
            title="DS Incoming"
            value={ds?.ds_incoming_temp.value}
            unit={'\u00B0C'}
            icon={<SensorsIcon color="success" />}
          />
        </Grid>

        <Grid size={{ xs: 12 }}>
          <LedIndicators leds={dv?.panel_leds.value} />
        </Grid>

        <Grid size={{ xs: 12 }}>
          <ControlPanel digitVars={dv} />
        </Grid>
      </Grid>
    </Box>
  );
}
