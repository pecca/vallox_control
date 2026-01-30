'use server';
import { getServerSession } from 'next-auth';
import { authOptions } from '@/lib/auth';
import { getFirestore } from '@/lib/firestore';

export interface HistoryDataPoint {
  timestamp: string;
  inside_temp?: number;
  outside_temp?: number;
  exhaust_temp?: number;
  incoming_temp?: number;
  rh1_sensor?: number;
  cur_fan_speed?: number;
  in_efficiency?: number;
  out_efficiency?: number;
  in_efficiency_calc?: number;
  out_efficiency_calc?: number;
}

export async function getHistoricalData(hours: number): Promise<HistoryDataPoint[]> {
  const session = await getServerSession(authOptions);
  if (!session) {
    throw new Error('Unauthorized');
  }

  const db = getFirestore();
  const since = new Date(Date.now() - hours * 60 * 60 * 1000);

  const snapshot = await db
    .collection('vallox_status')
    .where('timestamp', '>=', since)
    .orderBy('timestamp', 'asc')
    .get();

  return snapshot.docs.map((doc) => {
    const data = doc.data();
    // Support both nested and flat structure
    const digit = data.digit_vars || data;
    const control = data.control_vars || data;
    const ts = data.timestamp?.toDate?.() || new Date(data.timestamp);

    // Safely extract values with support for double-nesting and trailing whitespace/newlines
    const getVal = (obj: any, key: string): number | undefined => {
      if (!obj) return undefined;
      
      // Try exact match and trimmed match
      const findKey = (o: any, k: string) => {
        const keys = Object.keys(o);
        return keys.find(actualKey => actualKey.trim() === k);
      };

      const actualKey = findKey(obj, key);
      let entry = actualKey ? obj[actualKey] : undefined;
      
      // If we found another object with the same name (double-nesting), dive in
      if (entry && typeof entry === 'object' && !entry.value) {
        const nestedKey = findKey(entry, key);
        if (nestedKey) {
          entry = entry[nestedKey];
        }
      }

      if (typeof entry === 'number') return entry;
      if (entry && typeof entry.value === 'number') return entry.value;
      return undefined;
    };

    const tin = getVal(digit, 'incoming_temp');
    const tout = getVal(digit, 'outside_temp');
    const tinside = getVal(digit, 'inside_temp');
    const texhaust = getVal(digit, 'exhaust_temp');

    let in_eff_calc = undefined;
    let out_eff_calc = undefined;
    if (tin !== undefined && tout !== undefined && tinside !== undefined && texhaust !== undefined) {
      const diff = tinside - tout;
      if (Math.abs(diff) > 0.1) {
        in_eff_calc = ((tin - tout) / diff) * 100;
        out_eff_calc = ((tinside - texhaust) / diff) * 100;
      }
    }

    return {
      timestamp: ts.toISOString(),
      inside_temp: tinside,
      outside_temp: tout,
      exhaust_temp: texhaust,
      incoming_temp: tin,
      rh1_sensor: getVal(digit, 'rh1_sensor'),
      cur_fan_speed: getVal(digit, 'cur_fan_speed'),
      in_efficiency: getVal(control, 'in_efficiency_filtered') ?? getVal(control, 'in_efficiency'),
      out_efficiency: getVal(control, 'out_efficiency_filtered') ?? getVal(control, 'out_efficiency'),
      in_efficiency_calc: in_eff_calc,
      out_efficiency_calc: out_eff_calc,
    };
  });
}
