'use client';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableRow from '@mui/material/TableRow';
import Box from '@mui/material/Box';
import Chip from '@mui/material/Chip';
import type { ControlVars, DigitVars, DS18B20Vars } from '@/lib/schemas';

const STATE_LABELS: Record<number, string> = {
  0: 'Measuring',
  1: 'Heating',
  2: 'Stopped',
  3: 'Input Fan Stop',
};

const STATE_COLORS: Record<number, 'default' | 'warning' | 'success' | 'info' | 'error'> = {
  0: 'default',
  1: 'error',
  2: 'success',
  3: 'warning',
};

const END_REASON_LABELS: Record<number, string> = {
  0: 'None',
  1: 'Eff Recovered',
  2: 'Temp Target',
  3: 'Fan Stop Resolved',
  4: 'Timeout',
  5: 'Safety Shutoff',
  6: 'Eff Plateau',
  7: 'Fan Stop Plateau',
};

interface DefrostStatusProps {
  controlVars: ControlVars | undefined;
  digitVars: DigitVars | undefined;
  ds18b20Vars: DS18B20Vars | undefined;
}

export default function DefrostStatus({ controlVars, digitVars, ds18b20Vars }: DefrostStatusProps) {
  if (!controlVars) return null;

  const state = controlVars.defrost_state.value;
  const isActive = state === 1 || state === 3; // Heating or InputFanStop

  return (
    <Card sx={{
      background: 'rgba(255, 255, 255, 0.05)',
      border: isActive ? '1px solid rgba(255, 152, 0, 0.5)' : '1px solid rgba(255, 255, 255, 0.1)',
      height: '100%'
    }}>
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
          <Typography variant="h6">Defrost Status</Typography>
          <Chip
            label={STATE_LABELS[state] ?? `Unknown (${state})`}
            color={STATE_COLORS[state] ?? 'default'}
            size="small"
          />
        </Box>

        <TableContainer>
          <Table size="small">
            <TableBody>
              <TableRow>
                <TableCell sx={{ py: 0.5 }}>Cycle count</TableCell>
                <TableCell align="right" sx={{ py: 0.5 }}>
                  {controlVars.defrost_cycle_count.value}
                </TableCell>
              </TableRow>
              {isActive && (
                <>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Eff at phase start</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {controlVars.defrost_eff_at_phase_start.value.toFixed(1)} %
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Current eff</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {controlVars.in_efficiency.value.toFixed(1)} %
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Last improvement</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {controlVars.defrost_last_improvement_ago.value} s ago
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>DS18B20 exhaust</TableCell>
                    <TableCell align="right" sx={{ py: 0.5, color: (ds18b20Vars?.ds_exhaust_temp.value ?? 0) < 5 ? '#ff9800' : 'inherit' }}>
                      {ds18b20Vars?.ds_exhaust_temp.value.toFixed(1) ?? '--'} {'\u00B0C'}
                      {(ds18b20Vars?.ds_exhaust_temp.value ?? 0) < 5 ? ' (ice)' : ''}
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>DIGIT exhaust</TableCell>
                    <TableCell align="right" sx={{ py: 0.5, color: (digitVars?.exhaust_temp.value ?? 0) < 5 ? '#ff9800' : 'inherit' }}>
                      {digitVars?.exhaust_temp.value.toFixed(1) ?? '--'} {'\u00B0C'}
                      {(digitVars?.exhaust_temp.value ?? 0) < 5 ? ' (ice)' : ''}
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Defrost on time</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {controlVars.defrost_on_time.value} s
                    </TableCell>
                  </TableRow>
                </>
              )}
              {!isActive && controlVars.defrost_end_reason.value > 0 && (
                <>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Last end reason</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {END_REASON_LABELS[controlVars.defrost_end_reason.value] ?? controlVars.defrost_end_reason.value}
                    </TableCell>
                  </TableRow>
                  <TableRow>
                    <TableCell sx={{ py: 0.5 }}>Heating duration</TableCell>
                    <TableCell align="right" sx={{ py: 0.5 }}>
                      {controlVars.defrost_heating_duration.value} s
                    </TableCell>
                  </TableRow>
                  {controlVars.defrost_fan_stop_duration.value > 0 && (
                    <TableRow>
                      <TableCell sx={{ py: 0.5 }}>Fan stop duration</TableCell>
                      <TableCell align="right" sx={{ py: 0.5 }}>
                        {controlVars.defrost_fan_stop_duration.value} s
                      </TableCell>
                    </TableRow>
                  )}
                </>
              )}
            </TableBody>
          </Table>
        </TableContainer>
      </CardContent>
    </Card>
  );
}
