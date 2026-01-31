
'use server';

import { getFirestore } from '../../lib/firestore';

export interface DefrostCycle {
  id: string;
  cycle_number: number;
  timestamp: string; // ISO string for serialization
  heating_duration: number;
  fan_stop_duration?: number;
  total_duration: number;
  start_outside_temp: number;
  start_exhaust_temp: number;
  start_incoming_temp?: number; // Some cycles might have old name or missing
  start_supply_temp?: number;   // New standard name
  start_exhaust_humidity?: number;
  start_dew_point_delta?: number;
  start_fan_speed?: number;
  start_in_eff: number;
  end_reason: number;
  scenario?: string; // For simulated data
}

export async function getDefrostCycles(limit: number = 50): Promise<DefrostCycle[]> {
  const firestore = getFirestore();
  const snapshot = await firestore
    .collection('defrost_cycles')
    .orderBy('timestamp', 'desc')
    .limit(limit)
    .get();

  const cycles: DefrostCycle[] = snapshot.docs.map((doc) => {
    const data = doc.data();
    
    // Handle timestamp correctly (Firestore Timestamp -> Date -> ISO String)
    let timestampStr = new Date().toISOString();
    if (data.timestamp && typeof data.timestamp.toDate === 'function') {
        timestampStr = data.timestamp.toDate().toISOString();
    } else if (data.timestamp) {
        // Fallback if it's already a date or string (simulated data might vary)
        timestampStr = new Date(data.timestamp).toISOString();
    }

    return {
      id: doc.id,
      cycle_number: data.cycle_number,
      timestamp: timestampStr,
      heating_duration: data.heating_duration,
      fan_stop_duration: data.fan_stop_duration,
      total_duration: data.total_duration,
      start_outside_temp: data.start_outside_temp,
      start_exhaust_temp: data.start_exhaust_temp,
      start_incoming_temp: data.start_incoming_temp,
      start_supply_temp: data.start_supply_temp, // Standardize on this later if needed
      start_exhaust_humidity: data.start_exhaust_humidity,
      start_dew_point_delta: data.start_dew_point_delta,
      start_fan_speed: data.start_fan_speed,
      start_in_eff: data.start_in_eff,
      end_reason: data.end_reason,
      scenario: data.scenario,
    };
  });

  return cycles;
}
