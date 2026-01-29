'use client';
import { useEffect, useState } from 'react';
import Box from '@mui/material/Box';
import Typography from '@mui/material/Typography';
import ToggleButton from '@mui/material/ToggleButton';
import ToggleButtonGroup from '@mui/material/ToggleButtonGroup';
import CircularProgress from '@mui/material/CircularProgress';
import TemperatureChart, { HumidityChart } from '@/components/TemperatureChart';
import { getHistoricalData, type HistoryDataPoint } from '@/actions/history';

const TIME_RANGES = [
  { label: '1h', hours: 1 },
  { label: '6h', hours: 6 },
  { label: '24h', hours: 24 },
  { label: '7d', hours: 168 },
];

export default function HistoryPage() {
  const [hours, setHours] = useState(24);
  const [data, setData] = useState<HistoryDataPoint[]>([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    setLoading(true);
    getHistoricalData(hours)
      .then(setData)
      .catch(() => setData([]))
      .finally(() => setLoading(false));
  }, [hours]);

  return (
    <Box>
      <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', mb: 3 }}>
        <Typography variant="h5">Historical Data</Typography>
        <ToggleButtonGroup
          value={hours}
          exclusive
          onChange={(_, val) => val !== null && setHours(val)}
          size="small"
        >
          {TIME_RANGES.map((r) => (
            <ToggleButton key={r.hours} value={r.hours}>
              {r.label}
            </ToggleButton>
          ))}
        </ToggleButtonGroup>
      </Box>

      {loading ? (
        <Box sx={{ display: 'flex', justifyContent: 'center', mt: 8 }}>
          <CircularProgress />
        </Box>
      ) : data.length === 0 ? (
        <Typography color="text.secondary" sx={{ textAlign: 'center', mt: 4 }}>
          No data available for this time range.
        </Typography>
      ) : (
        <Box sx={{ display: 'flex', flexDirection: 'column', gap: 3 }}>
          <TemperatureChart data={data} title="Temperature Trends" />
          <HumidityChart data={data} />
        </Box>
      )}
    </Box>
  );
}
