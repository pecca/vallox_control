'use client';
import { useState, useTransition } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import TableRow from '@mui/material/TableRow';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import { setDeviceVariable } from '@/actions/vallox';

interface VarConfig {
  handle: string;
  label: string;
  unit: string;
  min: number;
  max: number;
}

const WRITABLE_VARS: VarConfig[] = [
  { handle: 'MIN_FAN_SPEED', label: 'Min Fan Speed', unit: '', min: 1, max: 8 },
  { handle: 'MAX_FAN_SPEED', label: 'Max Fan Speed', unit: '', min: 1, max: 8 },
  { handle: 'HRC_BYPASS', label: 'HRC Bypass Temp', unit: '\u00B0C', min: 14, max: 20 },
  { handle: 'INPUT_FAN_STOP', label: 'Input Fan Stop', unit: '\u00B0C', min: -3, max: 10 },
  { handle: 'PRE_HEATING_TEMP', label: 'Pre-heating Temp', unit: '\u00B0C', min: -3, max: 10 },
  { handle: 'CELL_DEFROST_HYSTERESIS', label: 'Defrost Hysteresis', unit: '\u00B0C', min: 0, max: 3 },
  { handle: 'DC_FAN_INPUT', label: 'DC Fan Input', unit: '%', min: 1, max: 100 },
  { handle: 'DC_FAN_OUTPUT', label: 'DC Fan Output', unit: '%', min: 1, max: 100 },
];

import type { DigitVars } from '@/lib/schemas';

interface ControlPanelProps {
  digitVars: DigitVars | undefined;
}

export default function ControlPanel({ digitVars }: ControlPanelProps) {
  const [inputs, setInputs] = useState<Record<string, string>>({});
  const [isPending, startTransition] = useTransition();
  const [pendingVar, setPendingVar] = useState<string | null>(null);

  function handleSet(varConfig: VarConfig) {
    const val = Number(inputs[varConfig.handle]);
    if (isNaN(val) || val < varConfig.min || val > varConfig.max) return;

    setPendingVar(varConfig.handle);
    startTransition(async () => {
      await setDeviceVariable(varConfig.handle, val);
      setInputs((prev) => ({ ...prev, [varConfig.handle]: '' }));
      setPendingVar(null);
    });
  }

  function getCurrentValue(handle: string): string {
    if (!digitVars) return '--';
    const key = handle.toLowerCase() as keyof DigitVars;
    const entry = digitVars[key];
    if (entry && 'value' in entry && typeof entry.value === 'number') return String(entry.value);
    return '--';
  }

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          Control Variables
        </Typography>
        <TableContainer>
          <Table size="small">
            <TableHead>
              <TableRow>
                <TableCell>Variable</TableCell>
                <TableCell>Current</TableCell>
                <TableCell>Set Value</TableCell>
                <TableCell />
              </TableRow>
            </TableHead>
            <TableBody>
              {WRITABLE_VARS.map((v) => (
                <TableRow key={v.handle}>
                  <TableCell>{v.label}</TableCell>
                  <TableCell>
                    {getCurrentValue(v.handle)} {v.unit}
                  </TableCell>
                  <TableCell>
                    <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
                      <TextField
                        size="small"
                        type="number"
                        placeholder={`${v.min}-${v.max}`}
                        value={inputs[v.handle] || ''}
                        onChange={(e) =>
                          setInputs((prev) => ({ ...prev, [v.handle]: e.target.value }))
                        }
                        inputProps={{ min: v.min, max: v.max }}
                        sx={{ width: 100 }}
                      />
                    </Box>
                  </TableCell>
                  <TableCell>
                    <Button
                      size="small"
                      variant="outlined"
                      onClick={() => handleSet(v)}
                      disabled={isPending && pendingVar === v.handle}
                    >
                      {isPending && pendingVar === v.handle ? '...' : 'Set'}
                    </Button>
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      </CardContent>
    </Card>
  );
}
