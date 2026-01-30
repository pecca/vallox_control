'use client';
import { useTransition } from 'react';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';
import Radio from '@mui/material/Radio';
import RadioGroup from '@mui/material/RadioGroup';
import FormControlLabel from '@mui/material/FormControlLabel';
import FormControl from '@mui/material/FormControl';
import AcUnitIcon from '@mui/icons-material/AcUnit';
import { setDeviceVariable } from '@/actions/vallox';
import type { ControlVars } from '@/lib/schemas';

interface DefrostModeControlProps {
  controlVars: ControlVars | undefined;
}

export default function DefrostModeControl({ controlVars }: DefrostModeControlProps) {
  const [isPending, startTransition] = useTransition();
  const currentMode = controlVars?.defrost_mode.value ?? 0;

  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const val = parseInt(event.target.value, 10);
    startTransition(async () => {
      await setDeviceVariable('control_vars', 'defrost_mode', val);
    });
  };

  return (
    <Card sx={{ 
      background: 'rgba(255, 255, 255, 0.05)',
      border: '1px solid rgba(255, 255, 255, 0.1)',
      height: '100%'
    }}>
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 2 }}>
          <AcUnitIcon color="info" />
          <Typography variant="h6">Defrost Mode</Typography>
        </Box>

        <FormControl component="fieldset" fullWidth disabled={isPending}>
          <RadioGroup
            row={false}
            value={String(currentMode)}
            onChange={handleChange}
            sx={{
              display: 'grid',
              gridTemplateColumns: 'repeat(4, 1fr)',
              gap: 1,
              '& .MuiFormControlLabel-root': {
                margin: 0,
                padding: '8px',
                borderRadius: '8px',
                border: '1px solid rgba(255, 255, 255, 0.1)',
                justifyContent: 'center',
                '&.Mui-checked': {
                  borderColor: '#0288d1',
                  background: 'rgba(2, 136, 209, 0.1)'
                }
              },
              '& .MuiRadio-root': { display: 'none' } // Hide actual radio button for extra-clean look
            }}
          >
            <FormControlLabel 
              value="0" 
              control={<Radio />} 
              label="OFF" 
              sx={{ border: currentMode === 0 ? '1px solid #0288d1 !important' : 'inherit' }}
            />
            <FormControlLabel 
              value="1" 
              control={<Radio />} 
              label="ON" 
              sx={{ border: currentMode === 1 ? '1px solid #0288d1 !important' : 'inherit' }}
            />
            <FormControlLabel 
              value="2" 
              control={<Radio />} 
              label="AUTO" 
              sx={{ border: currentMode === 2 ? '1px solid #0288d1 !important' : 'inherit' }}
            />
            <FormControlLabel 
              value="3" 
              control={<Radio />} 
              label="AI" 
              sx={{ border: currentMode === 3 ? '1px solid #7c4dff !important' : 'inherit' }}
            />
          </RadioGroup>
        </FormControl>
        {isPending && (
          <Typography variant="caption" color="info.main" sx={{ display: 'block', mt: 1, textAlign: 'center' }}>
            Updating...
          </Typography>
        )}
      </CardContent>
    </Card>
  );
}
