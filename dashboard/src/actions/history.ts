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
    const vars = data.digit_vars || data;
    const ts = data.timestamp?.toDate?.() || new Date(data.timestamp);
    return {
      timestamp: ts.toISOString(),
      inside_temp: vars.inside_temp?.value,
      outside_temp: vars.outside_temp?.value,
      exhaust_temp: vars.exhaust_temp?.value,
      incoming_temp: vars.incoming_temp?.value,
      rh1_sensor: vars.rh1_sensor?.value,
      cur_fan_speed: vars.cur_fan_speed?.value,
    };
  });
}
