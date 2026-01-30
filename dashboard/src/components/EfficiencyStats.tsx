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
import Divider from '@mui/material/Divider';
import type { DigitVars, ControlVars } from '@/lib/schemas';

interface EfficiencyStatsProps {
  digitVars: DigitVars | undefined;
  controlVars: ControlVars | undefined;
}

export default function EfficiencyStats({ digitVars, controlVars }: EfficiencyStatsProps) {
  if (!digitVars || !controlVars) return null;

  // Calculate "LTO efficiency (outside temp)"
  // formula: 
  // in = (incoming - outside) / (inside - outside)
  // out = (inside - exhaust) / (inside - outside)
  const tin = digitVars.incoming_temp.value;
  const tout = digitVars.outside_temp.value;
  const tinside = digitVars.inside_temp.value;
  const texhaust = digitVars.exhaust_temp.value;

  let outsideEffIn = 0;
  let outsideEffOut = 0;
  if (Math.abs(tinside - tout) > 0.1) {
    outsideEffIn = ((tin - tout) / (tinside - tout)) * 100;
    outsideEffOut = ((tinside - texhaust) / (tinside - tout)) * 100;
  }

  const EfficiencyTable = ({ title, inc, out }: { title: string, inc: number | undefined, out: number | undefined }) => {
    const i = inc ?? 0;
    const o = out ?? 0;
    const avg = (i + o) / 2;

    return (
      <Box sx={{ mb: 2 }}>
        <Typography variant="subtitle2" sx={{ fontWeight: 'bold', mb: 1 }}>
          {title}
        </Typography>
        <TableContainer sx={{ border: '1px solid #eee', borderRadius: 1 }}>
          <Table size="small">
            <TableBody>
              <TableRow>
                <TableCell sx={{ py: 0.5 }}>Incoming air</TableCell>
                <TableCell align="right" sx={{ py: 0.5 }}>{i.toFixed(1)} %</TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ py: 0.5 }}>Outcoming air</TableCell>
                <TableCell align="right" sx={{ py: 0.5 }}>{o.toFixed(1)} %</TableCell>
              </TableRow>
              <TableRow>
                <TableCell sx={{ py: 0.5 }}>Average</TableCell>
                <TableCell align="right" sx={{ py: 0.5 }}>{avg.toFixed(1)} %</TableCell>
              </TableRow>
            </TableBody>
          </Table>
        </TableContainer>
      </Box>
    );
  };

  return (
    <Card>
      <CardContent>
        <Typography variant="h6" sx={{ mb: 2 }}>
          Efficiency & Environment
        </Typography>
        
        <EfficiencyTable 
          title="LTO efficiency (after radiator)" 
          inc={controlVars.in_efficiency.value} 
          out={controlVars.out_efficiency.value} 
        />
        
        <EfficiencyTable 
          title="LTO efficiency filtered (after radiator)" 
          inc={controlVars.in_efficiency_filtered.value} 
          out={controlVars.out_efficiency_filtered.value} 
        />

        <EfficiencyTable 
          title="LTO efficiency (outside temp)" 
          inc={outsideEffIn} 
          out={outsideEffOut} 
        />

        <Divider sx={{ my: 2 }} />

        <Box sx={{ textAlign: 'center' }}>
          <Typography variant="subtitle2" sx={{ color: 'text.secondary' }}>
            Dew point
          </Typography>
          <Typography variant="h5">
            {controlVars.dew_point.value.toFixed(1)} {'\u00B0C'}
          </Typography>
        </Box>
      </CardContent>
    </Card>
  );
}
