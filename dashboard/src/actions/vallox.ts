'use server';
import { getServerSession } from 'next-auth';
import { authOptions } from '@/lib/auth';
import { getStatus, setVariable } from '@/lib/vallox-api';
import {
  DigitVarsResponse,
  ControlVarsResponse,
  DS18B20VarsResponse,
  type DeviceStatus,
  type AiDefrostState,
  type AiConfig,
} from '@/lib/schemas';

import { getFirestore } from '@/lib/firestore';

export async function getDeviceStatus(): Promise<DeviceStatus> {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  const firestore = getFirestore();

  const [rawDigit, rawControl, rawDS18B20, aiStateDoc, aiConfigDoc] = await Promise.all([
    getStatus('digit_vars'),
    getStatus('control_vars'),
    getStatus('ds18b20_vars'),
    firestore.doc('ai_defrost/state').get(),
    firestore.doc('ai_defrost/config').get(),
  ]);

  const digitVars = DigitVarsResponse.parse(rawDigit);
  const controlVars = ControlVarsResponse.parse(rawControl);
  const ds18b20Vars = DS18B20VarsResponse.parse(rawDS18B20);
  
  let aiDefrostState: AiDefrostState | undefined;
  
  if (aiStateDoc.exists) {
    const data = aiStateDoc.data() as any;
    aiDefrostState = {
        defrostScore: data.defrostScore,
        updated: data.updated?.toMillis ? data.updated.toMillis() : Date.now(),
    };
  }

  let aiConfig: AiConfig = {
      guardrailStartLimit: 60, 
      guardrailStopLimit: 80
  };

  if (aiConfigDoc.exists) {
      const data = aiConfigDoc.data();
      aiConfig = {
          guardrailStartLimit: data?.guardrailStartLimit ?? 60,
          guardrailStopLimit: data?.guardrailStopLimit ?? 80,
      };
  }

  const ret = {
    digitVars: digitVars.digit_vars,
    controlVars: controlVars.control_vars,
    ds18b20Vars: ds18b20Vars.ds18b20_vars,
    aiDefrostState,
    aiConfig,
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

export async function saveAiConfig(config: AiConfig) {
    const session = await getServerSession(authOptions);
    if (!session) {
      throw new Error('Unauthorized');
    }
  
    const firestore = getFirestore();
    await firestore.doc('ai_defrost/config').set(config, { merge: true });
    return { success: true };
}
