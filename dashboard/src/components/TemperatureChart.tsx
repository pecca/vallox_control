'use client';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import type { HistoryDataPoint } from '@/actions/history';

interface TemperatureChartProps {
  data: HistoryDataPoint[];
  title: string;
}

const TEMP_LINES = [
  { key: 'inside_temp', color: '#ff7043', name: 'Inside' },
  { key: 'outside_temp', color: '#42a5f5', name: 'Outside' },
  { key: 'exhaust_temp', color: '#ab47bc', name: 'Exhaust' },
  { key: 'incoming_temp', color: '#66bb6a', name: 'Incoming' },
];

function formatTime(ts: string): string {
  const d = new Date(ts);
  return `${d.getHours().toString().padStart(2, '0')}:${d.getMinutes().toString().padStart(2, '0')}`;
}

export default function TemperatureChart({ data, title }: TemperatureChartProps) {
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          {title}
        </Typography>
        <ResponsiveContainer width="100%" height={350}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis dataKey="timestamp" tickFormatter={formatTime} stroke="#999" />
            <YAxis stroke="#999" unit={'\u00B0C'} />
            <Tooltip
              labelFormatter={(label) => new Date(label).toLocaleString()}
              contentStyle={{ backgroundColor: '#132f4c', border: 'none' }}
            />
            <Legend />
            {TEMP_LINES.map((line) => (
              <Line
                key={line.key}
                type="monotone"
                dataKey={line.key}
                stroke={line.color}
                name={line.name}
                dot={false}
                strokeWidth={2}
              />
            ))}
          </LineChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  );
}

interface HumidityChartProps {
  data: HistoryDataPoint[];
}

export function HumidityChart({ data }: HumidityChartProps) {
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          Humidity
        </Typography>
        <ResponsiveContainer width="100%" height={250}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis dataKey="timestamp" tickFormatter={formatTime} stroke="#999" />
            <YAxis stroke="#999" unit="%" />
            <Tooltip
              labelFormatter={(label) => new Date(label).toLocaleString()}
              contentStyle={{ backgroundColor: '#132f4c', border: 'none' }}
            />
            <Line
              type="monotone"
              dataKey="rh1_sensor"
              stroke="#ffa726"
              name="Humidity"
              dot={false}
              strokeWidth={2}
            />
          </LineChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  );
}

interface EfficiencyChartProps {
  data: HistoryDataPoint[];
}

export function EfficiencyChart({ data }: EfficiencyChartProps) {
  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          LTO Efficiency
        </Typography>
        <ResponsiveContainer width="100%" height={300}>
          <LineChart data={data}>
            <CartesianGrid strokeDasharray="3 3" stroke="#333" />
            <XAxis dataKey="timestamp" tickFormatter={formatTime} stroke="#999" />
            <YAxis stroke="#999" unit="%" domain={[0, 110]} />
            <Tooltip
              labelFormatter={(label) => new Date(label).toLocaleString()}
              contentStyle={{ backgroundColor: '#132f4c', border: 'none' }}
            />
            <Legend />
            <Line
              type="monotone"
              dataKey="in_efficiency"
              stroke="#4fc3f7"
              name="In Efficiency (measured)"
              dot={false}
              strokeWidth={2}
            />
            <Line
              type="monotone"
              dataKey="in_efficiency_calc"
              stroke="#ba68c8"
              name="In Efficiency (calculated)"
              dot={false}
              strokeWidth={1}
              strokeDasharray="5 5"
            />
          </LineChart>
        </ResponsiveContainer>
      </CardContent>
    </Card>
  );
}
