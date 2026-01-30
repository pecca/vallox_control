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
    const digit = data.digit_vars || {};
    const control = data.control_vars || {};
    const ts = data.timestamp?.toDate?.() || new Date(data.timestamp);

    // Calc efficiency based on outside temp
    const tin = digit.incoming_temp?.value;
    const tout = digit.outside_temp?.value;
    const tinside = digit.inside_temp?.value;
    const texhaust = digit.exhaust_temp?.value;

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
      inside_temp: digit.inside_temp?.value,
      outside_temp: digit.outside_temp?.value,
      exhaust_temp: digit.exhaust_temp?.value,
      incoming_temp: digit.incoming_temp?.value,
      rh1_sensor: digit.rh1_sensor?.value,
      cur_fan_speed: digit.cur_fan_speed?.value,
      in_efficiency: control.in_efficiency_filtered?.value,
      out_efficiency: control.out_efficiency_filtered?.value,
      in_efficiency_calc: in_eff_calc,
      out_efficiency_calc: out_eff_calc,
    };
  });
}
