'use client';
import { useState, useTransition } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import TextField from '@mui/material/TextField';
import Button from '@mui/material/Button';
import Collapse from '@mui/material/Collapse';
import IconButton from '@mui/material/IconButton';
import ExpandMoreIcon from '@mui/icons-material/ExpandMore';
import SettingsIcon from '@mui/icons-material/Settings';
import { setDeviceVariable } from '@/actions/vallox';
import type { ControlVars } from '@/lib/schemas';

interface DefrostConfigPanelProps {
  controlVars: ControlVars | undefined;
}

interface ConfigField {
  key: keyof ControlVars;
  label: string;
  unit: string;
  min: number;
  max: number;
  step: number;
  description: string;
}

const CONFIG_FIELDS: ConfigField[] = [
  {
    key: 'defrost_start_level',
    label: 'Start Level',
    unit: '%',
    min: 50,
    max: 85,
    step: 1,
    description: 'Efficiency threshold to trigger defrost',
  },
  {
    key: 'defrost_eff_imp_thresh',
    label: 'Improvement Threshold',
    unit: '%',
    min: 0.1,
    max: 2.0,
    step: 0.1,
    description: 'Minimum efficiency delta to count as improvement',
  },
  {
    key: 'min_exhaust_temp',
    label: 'Min Exhaust Temp',
    unit: '°C',
    min: 0,
    max: 10,
    step: 0.5,
    description: 'Safety limit during heating (DS18B20)',
  },
  {
    key: 'defrost_heating_no_imp_time',
    label: 'Heating No-Improvement Time',
    unit: 's',
    min: 60,
    max: 600,
    step: 30,
    description: 'Stop heating after this long without improvement',
  },
  {
    key: 'defrost_fan_stop_no_imp_time',
    label: 'Fan Stop No-Improvement Time',
    unit: 's',
    min: 60,
    max: 600,
    step: 30,
    description: 'Stop fan stop after this long without improvement',
  },
  {
    key: 'defrost_fan_stop_max_dur',
    label: 'Fan Stop Max Duration',
    unit: 's',
    min: 300,
    max: 1800,
    step: 60,
    description: 'Absolute maximum fan stop duration',
  },
  {
    key: 'defrost_stop_duration',
    label: 'Stop Duration (Cooldown)',
    unit: 's',
    min: 300,
    max: 1800,
    step: 60,
    description: 'Cooldown period after defrost before next cycle',
  },
];

export default function DefrostConfigPanel({ controlVars }: DefrostConfigPanelProps) {
  const [expanded, setExpanded] = useState(false);
  const [isPending, startTransition] = useTransition();
  const [pendingField, setPendingField] = useState<string | null>(null);

  const handleUpdate = (field: ConfigField, value: number) => {
    setPendingField(field.key);
    startTransition(async () => {
      await setDeviceVariable('control_vars', field.key, value);
      setPendingField(null);
    });
  };

  if (!controlVars) return null;

  // Filter fields to only show those available in the current firmware
  const availableFields = CONFIG_FIELDS.filter(field => controlVars[field.key] !== undefined);
  
  if (availableFields.length === 0) {
    return null; // No config fields available in this firmware version
  }

  const efficiencyFields = availableFields.slice(0, 2);
  const tempFields = availableFields.filter(f => f.key === 'min_exhaust_temp');
  const timeFields = availableFields.filter(f => 
    f.key.includes('time') || f.key.includes('duration') || f.key.includes('dur')
  );

  return (
    <Card sx={{
      background: 'rgba(255, 255, 255, 0.05)',
      border: '1px solid rgba(255, 255, 255, 0.1)',
    }}>
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
          <Box sx={{ display: 'flex', alignItems: 'center', gap: 1 }}>
            <SettingsIcon color="info" />
            <Typography variant="h6">Defrost Configuration</Typography>
          </Box>
          <IconButton
            onClick={() => setExpanded(!expanded)}
            sx={{
              transform: expanded ? 'rotate(180deg)' : 'rotate(0deg)',
              transition: 'transform 0.3s',
            }}
          >
            <ExpandMoreIcon />
          </IconButton>
        </Box>

        <Collapse in={expanded}>
          <Box sx={{ mt: 2 }}>
            {/* Efficiency Thresholds */}
            {efficiencyFields.length > 0 && (
              <>
                <Typography variant="subtitle2" sx={{ mb: 1, color: 'info.main' }}>
                  Efficiency Thresholds
                </Typography>
                <Box sx={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(250px, 1fr))', gap: 2, mb: 3 }}>
                  {efficiencyFields.map((field) => (
                <Box key={field.key}>
                  <TextField
                    fullWidth
                    size="small"
                    type="number"
                    label={field.label}
                    defaultValue={controlVars[field.key]?.value}
                    disabled={isPending && pendingField === field.key}
                    inputProps={{
                      min: field.min,
                      max: field.max,
                      step: field.step,
                    }}
                    InputProps={{
                      endAdornment: <Typography variant="caption" sx={{ ml: 1 }}>{field.unit}</Typography>,
                    }}
                    helperText={field.description}
                    onBlur={(e) => {
                      const value = parseFloat(e.target.value);
                      if (!isNaN(value) && value >= field.min && value <= field.max) {
                        handleUpdate(field, value);
                      }
                    }}
                  />
                </Box>
                  ))}
                </Box>
              </>
            )}

            {/* Temperature Safety */}
            {tempFields.length > 0 && (
              <>
                <Typography variant="subtitle2" sx={{ mb: 1, color: 'warning.main' }}>
                  Temperature Safety
                </Typography>
                <Box sx={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(250px, 1fr))', gap: 2, mb: 3 }}>
                  {tempFields.map((field) => (
                    <Box key={field.key}>
                      <TextField
                        fullWidth
                        size="small"
                        type="number"
                        label={field.label}
                        defaultValue={controlVars[field.key]?.value}
                        disabled={isPending && pendingField === field.key}
                        inputProps={{
                          min: field.min,
                          max: field.max,
                          step: field.step,
                        }}
                        InputProps={{
                          endAdornment: <Typography variant="caption" sx={{ ml: 1 }}>{field.unit}</Typography>,
                        }}
                        helperText={field.description}
                        onBlur={(e) => {
                          const value = parseFloat(e.target.value);
                          if (!isNaN(value) && value >= field.min && value <= field.max) {
                            handleUpdate(field, value);
                          }
                        }}
                      />
                    </Box>
                  ))}
                </Box>
              </>
            )}

            {/* Time Limits */}
            {timeFields.length > 0 && (
              <>
                <Typography variant="subtitle2" sx={{ mb: 1, color: 'success.main' }}>
                  Time Limits
                </Typography>
                <Box sx={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(250px, 1fr))', gap: 2 }}>
                  {timeFields.map((field) => (
                <Box key={field.key}>
                  <TextField
                    fullWidth
                    size="small"
                    type="number"
                    label={field.label}
                    defaultValue={controlVars[field.key]?.value}
                    disabled={isPending && pendingField === field.key}
                    inputProps={{
                      min: field.min,
                      max: field.max,
                      step: field.step,
                    }}
                    InputProps={{
                      endAdornment: <Typography variant="caption" sx={{ ml: 1 }}>{field.unit}</Typography>,
                    }}
                    helperText={field.description}
                    onBlur={(e) => {
                      const value = parseFloat(e.target.value);
                      if (!isNaN(value) && value >= field.min && value <= field.max) {
                        handleUpdate(field, value);
                      }
                    }}
                  />
                </Box>
                   ))}\n                 </Box>
               </>
             )}

            {isPending && (
              <Typography variant="caption" color="info.main" sx={{ display: 'block', mt: 2, textAlign: 'center' }}>
                Updating {pendingField}...
              </Typography>
            )}
          </Box>
        </Collapse>
      </CardContent>
    </Card>
  );
}
