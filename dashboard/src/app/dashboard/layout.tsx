'use client';
import Box from '@mui/material/Box';
import Navbar from '@/components/Navbar';

export default function DashboardLayout({ children }: { children: React.ReactNode }) {
  return (
    <Box>
      <Navbar />
      <Box sx={{ p: 3 }}>{children}</Box>
    </Box>
  );
}
