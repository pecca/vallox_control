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
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormControl from '@mui/material/FormControl';
import { setDeviceVariable } from '@/actions/vallox';
import type { DigitVars, ControlVars } from '@/lib/schemas';

interface VarConfig {
  type: 'digit_vars' | 'control_vars';
  handle: string;
  label: string;
  unit: string;
  min: number;
  max: number;
  isRadio?: boolean;
}

const digitWritableVars: VarConfig[] = [
  { type: 'digit_vars', handle: 'min_fan_speed', label: 'Min Fan Speed', unit: '', min: 1, max: 8 },
  { type: 'digit_vars', handle: 'max_fan_speed', label: 'Max Fan Speed', unit: '', min: 1, max: 8 },
  { type: 'digit_vars', handle: 'hrc_bypass_temp', label: 'HRC Bypass Temp', unit: '\u00B0C', min: 14, max: 20 },
  { type: 'digit_vars', handle: 'input_fan_stop_temp', label: 'Input Fan Stop', unit: '\u00B0C', min: -6, max: 14 },
  { type: 'digit_vars', handle: 'pre_heating_temp', label: 'Pre-heating Temp', unit: '\u00B0C', min: -3, max: 10 },
  { type: 'digit_vars', handle: 'cell_defrosting_hysteresis', label: 'Defrost Hysteresis', unit: '\u00B0C', min: 0, max: 3 },
  { type: 'digit_vars', handle: 'dc_fan_input', label: 'DC Fan Input', unit: '%', min: 1, max: 100 },
  { type: 'digit_vars', handle: 'dc_fan_output', label: 'DC Fan Output', unit: '%', min: 1, max: 100 },
];

const controlWritableVars: VarConfig[] = [
  { type: 'control_vars', handle: 'defrost_start_level', label: 'Start level (LTO efficiency incoming air)', unit: '%', min: 0, max: 100 },
  { type: 'control_vars', handle: 'defrost_max_duration', label: 'Max duration of defrost', unit: 'min', min: 0, max: 60 },
  { type: 'control_vars', handle: 'defrost_target_in_eff', label: 'Target incoming efficiency', unit: '%', min: 0, max: 100 },
  { type: 'control_vars', handle: 'defrost_target_temp', label: 'Target incoming temperature', unit: '\u00B0C', min: 0, max: 30 },
];



interface ControlPanelProps {
  digitVars: DigitVars | undefined;
  controlVars: ControlVars | undefined;
}

export default function ControlPanel({ digitVars, controlVars }: ControlPanelProps) {
  const [inputs, setInputs] = useState<Record<string, string>>({});
  const [isPending, startTransition] = useTransition();
  const [pendingVar, setPendingVar] = useState<string | null>(null);

  function handleSet(varConfig: VarConfig) {
    const inputValue = inputs[varConfig.handle];
    const val = inputValue !== undefined && inputValue !== '' 
      ? Number(inputValue) 
      : Number(getCurrentValue(varConfig));

    if (isNaN(val) || val < varConfig.min || val > varConfig.max) return;

    setPendingVar(varConfig.handle);
    startTransition(async () => {
      await setDeviceVariable(varConfig.type, varConfig.handle, val);
      setInputs((prev) => ({ ...prev, [varConfig.handle]: '' }));
      setPendingVar(null);
    });
  }

  function getCurrentValue(varConfig: VarConfig): string {
    const { type, handle } = varConfig;
    if (type === 'digit_vars') {
      if (!digitVars) return '--';
      const key = handle as keyof DigitVars;
      const entry = digitVars[key];
      if (entry && 'value' in entry && typeof entry.value === 'number') return String(entry.value);
    } else {
      if (!controlVars) return '--';
      const key = handle as keyof ControlVars;
      const entry = controlVars[key];
      if (entry && 'value' in entry && typeof entry.value === 'number') return String(entry.value);
    }
    return '--';
  }

  return (
    <Box sx={{ display: 'flex', flexDirection: 'column', gap: 3 }}>
      <Card>
        <CardContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Control Variables (Defrost)
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
                <TableRow>
                  <TableCell>Defrost time</TableCell>
                  <TableCell>{controlVars?.defrost_time.value ?? '--'} s</TableCell>
                  <TableCell />
                  <TableCell />
                </TableRow>
                {controlWritableVars.map((v) => (
                  <TableRow key={v.handle}>
                    <TableCell>{v.label}</TableCell>
                    <TableCell>
                      {getCurrentValue(v)} {v.unit}
                    </TableCell>
                    <TableCell>
                      <Box sx={{ display: 'flex', gap: 1, alignItems: 'center' }}>
                        {v.isRadio ? (
                          <FormControl size="small">
                            <RadioGroup
                              row
                              value={inputs[v.handle] ?? getCurrentValue(v)}
                              onChange={(e) =>
                                setInputs((prev) => ({ ...prev, [v.handle]: e.target.value }))
                              }
                            >
                              <FormControlLabel value="0" control={<Radio size="small" />} label="Off" />
                              <FormControlLabel value="1" control={<Radio size="small" />} label="On" />
                              <FormControlLabel value="2" control={<Radio size="small" />} label="Auto" />
                            </RadioGroup>
                          </FormControl>
                        ) : (
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
                        )}
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

      <Card>
        <CardContent>
          <Typography variant="h6" sx={{ mb: 2 }}>
            Digit Variables
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
                {digitWritableVars.map((v) => (
                  <TableRow key={v.handle}>
                    <TableCell>{v.label}</TableCell>
                    <TableCell>
                      {getCurrentValue(v)} {v.unit}
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
    </Box>
  );
}
