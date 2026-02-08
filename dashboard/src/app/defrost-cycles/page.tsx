
import React from 'react';
import { getDefrostCycles } from '../actions/getDefrostCycles';
import Navbar from '@/components/Navbar';
import { 
  Table, 
  TableBody, 
  TableCell, 
  TableContainer, 
  TableHead, 
  TableRow, 
  Paper, 
  Typography,
  Chip,
  Box,
  Stack
} from '@mui/material';

export default async function DefrostCyclesPage() {
  const cycles = await getDefrostCycles(100);

  return (
    <Box>
      <Navbar />
      <Box sx={{ p: 3 }}>
        <Typography variant="h4" gutterBottom sx={{ mb: 3, fontWeight: 'bold', color: 'primary.main' }}>
          Defrost Cycles History
        </Typography>
        
        <TableContainer component={Paper} elevation={2} sx={{ borderRadius: 2 }}>
          <Table sx={{ minWidth: 650 }} aria-label="defrost cycles table">
            <TableHead sx={{ bgcolor: 'primary.main' }}>
              <TableRow>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Cycle #</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Time</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Defrost (Heating + Fan Stop)</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Outside °C</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Exhaust °C</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Supply °C</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Humidity %</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Dew Point Δ</TableCell>
                <TableCell sx={{ fontWeight: 'bold', color: 'common.white' }}>Scenario</TableCell>
              </TableRow>
            </TableHead>
            <TableBody>
              {cycles.map((cycle) => (
                <TableRow
                  key={cycle.id}
                  sx={{ '&:last-child td, &:last-child th': { border: 0 }, '&:hover': { bgcolor: 'action.hover' } }}
                >
                  <TableCell component="th" scope="row">
                    {cycle.cycle_number}
                  </TableCell>
                  <TableCell>
                    {new Date(cycle.timestamp).toLocaleString('fi-FI')}
                  </TableCell>
                  <TableCell>
                    <Stack direction="row" spacing={1}>
                      <Chip 
                        label={`H: ${Math.round(cycle.heating_duration / 60)} min`} 
                        size="small" 
                        color={cycle.heating_duration > 900 ? "warning" : "default"}
                        variant="outlined"
                      />
                      {cycle.fan_stop_duration && cycle.fan_stop_duration > 0 ? (
                        <Chip 
                          label={`FS: ${Math.round(cycle.fan_stop_duration / 60)} min`} 
                          size="small" 
                          color="info"
                          variant="outlined"
                        />
                      ) : null}
                    </Stack>
                  </TableCell>
                  <TableCell>{cycle.start_outside_temp}°C</TableCell>
                  <TableCell>{cycle.start_exhaust_temp}°C</TableCell>
                  <TableCell>
                    {cycle.start_supply_temp ?? cycle.start_incoming_temp ?? '-'}°C
                  </TableCell>
                  <TableCell>
                    {cycle.start_exhaust_humidity ? `${cycle.start_exhaust_humidity}%` : '-'}
                  </TableCell>
                  <TableCell>
                     <Chip 
                      label={cycle.start_dew_point_delta ?? '-'} 
                      size="small"
                      color={(cycle.start_dew_point_delta !== undefined && cycle.start_dew_point_delta < 0) ? "error" : "success"}
                    />
                  </TableCell>
                  <TableCell>
                    {cycle.scenario && (
                      <Chip 
                        label={cycle.scenario} 
                        size="small" 
                        color={
                          cycle.scenario === 'preemptive' ? 'primary' : 
                          cycle.scenario === 'reactive' ? 'error' : 'default'
                        } 
                      />
                    )}
                  </TableCell>
                </TableRow>
              ))}
            </TableBody>
          </Table>
        </TableContainer>
      </Box>
    </Box>
  );
}
