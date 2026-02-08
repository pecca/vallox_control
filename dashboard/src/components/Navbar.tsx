'use client';
import { signOut, useSession } from 'next-auth/react';
import AppBar from '@mui/material/AppBar';
import Toolbar from '@mui/material/Toolbar';
import Typography from '@mui/material/Typography';
import Button from '@mui/material/Button';
import Box from '@mui/material/Box';
import AirIcon from '@mui/icons-material/Air';
import HistoryIcon from '@mui/icons-material/History';
import DashboardIcon from '@mui/icons-material/Dashboard';
import AcUnitIcon from '@mui/icons-material/AcUnit';
import Link from 'next/link';

export default function Navbar() {
  const { data: session } = useSession();

  return (
    <AppBar position="static" color="transparent" elevation={1}>
        <Toolbar>
          <AirIcon sx={{ mr: 1 }} />
          <Typography variant="h6" sx={{ flexGrow: 0, mr: 4 }}>
            Vallox Control
          </Typography>
          <Button
            component={Link}
            href="/dashboard"
            startIcon={<DashboardIcon />}
            color="inherit"
          >
            Dashboard
          </Button>
          <Button
            component={Link}
            href="/dashboard/history"
            startIcon={<HistoryIcon />}
            color="inherit"
          >
            History
          </Button>
          <Button
            component={Link}
            href="/defrost-cycles"
            startIcon={<AcUnitIcon />}
            color="inherit"
          >
            Defrost Cycles
          </Button>
          <Box sx={{ flexGrow: 1 }} />
          <Typography variant="body2" sx={{ mr: 2 }}>
            {session?.user?.name}
          </Typography>
          <Button color="inherit" onClick={() => signOut({ callbackUrl: '/login' })}>
            Logout
          </Button>
        </Toolbar>
      </AppBar>
  );
}
