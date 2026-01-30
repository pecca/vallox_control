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

  const ret = {
    digitVars: digitVars.digit_vars,
    controlVars: controlVars.control_vars,
    ds18b20Vars: ds18b20Vars.ds18b20_vars,
  };
  console.log('digitVars.min_fan_speed', ret.digitVars.min_fan_speed);
  return ret;
}

export async function setDeviceVariable(
  type: 'digit_vars' | 'control_vars',
  variable: string,
  value: number
) {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }
  console.log('setDeviceVariable', type, variable, value);
  return setVariable(type, variable, value);
}
