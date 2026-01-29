'use server';
import { getServerSession } from 'next-auth';
import { authOptions } from '@/lib/auth';
import { getStatus, setVariable } from '@/lib/vallox-api';

export async function getDeviceStatus() {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  const response = await getStatus('digit_vars');
  return response.digit_vars || response;
}

export async function setDeviceVariable(variable: string, value: number) {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  return setVariable('digit_vars', variable, value);
}
