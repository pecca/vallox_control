'use server';
import { getServerSession } from 'next-auth';
import { authOptions } from '@/lib/auth';
import { getStatus, setVariable } from '@/lib/vallox-api';
import {
  DigitVarsResponse,
  ControlVarsResponse,
  DS18B20VarsResponse,
  type DeviceStatus,
} from '@/lib/schemas';

export async function getDeviceStatus(): Promise<DeviceStatus> {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  const [rawDigit, rawControl, rawDS18B20] = await Promise.all([
    getStatus('digit_vars'),
    getStatus('control_vars'),
    getStatus('ds18b20_vars'),
  ]);

  const digitVars = DigitVarsResponse.parse(rawDigit);
  const controlVars = ControlVarsResponse.parse(rawControl);
  const ds18b20Vars = DS18B20VarsResponse.parse(rawDS18B20);

  return {
    digitVars: digitVars.digit_vars,
    controlVars: controlVars.control_vars,
    ds18b20Vars: ds18b20Vars.ds18b20_vars,
  };
}

export async function setDeviceVariable(variable: string, value: number) {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  return setVariable('digit_vars', variable, value);
}
