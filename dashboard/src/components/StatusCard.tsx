'use client';
import Card from '@mui/material/Card';
import CardContent from '@mui/material/CardContent';
import Typography from '@mui/material/Typography';
import Box from '@mui/material/Box';

interface StatusCardProps {
  title: string;
  value: number | string | undefined;
  unit: string;
  icon: React.ReactNode;
}

export default function StatusCard({ title, value, unit, icon }: StatusCardProps) {
  return (
    <Card>
      <CardContent>
        <Box sx={{ display: 'flex', alignItems: 'center', gap: 1, mb: 1 }}>
          {icon}
          <Typography variant="body2" color="text.secondary">
            {title}
          </Typography>
        </Box>
        <Typography variant="h4" sx={{ 
          fontSize: { xs: '1.5rem', sm: '2.125rem' },
          whiteSpace: 'nowrap',
          overflow: 'hidden',
          textOverflow: 'ellipsis'
        }}>
          {value !== undefined ? `${value}` : '--'}
          <Typography component="span" variant="h6" color="text.secondary" sx={{ 
            ml: 0.5,
            fontSize: { xs: '0.8rem', sm: '1.25rem' } 
          }}>
            {unit}
          </Typography>
        </Typography>
      </CardContent>
    </Card>
  );
}
