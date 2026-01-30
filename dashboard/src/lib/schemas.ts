import { z } from 'zod';

// Base: a timestamped numeric value
const NumericVar = z.object({
  value: z.number(),
  ts: z.number(),
});

// Base: a timestamped string value (flags)
const StringVar = z.object({
  value: z.string(),
  ts: z.number(),
});

// Panel LEDs
const PanelLeds = z.object({
  value: z.object({
    'power-key': z.number(),
    'CO2-key': z.number(),
    '%RH-key': z.number(),
    'post-heating-key': z.number(),
    'filter-check-symbol': z.number(),
    'post-heating-symbol': z.number(),
    'fault-symbol': z.number(),
    'service-symbol': z.number(),
  }),
  ts: z.number(),
});

// IO gates
const IOGate1 = z.object({
  value: z.object({ fan_speed: z.number() }),
  ts: z.number(),
});

const IOGate2 = z.object({
  value: z.object({ 'post-heating': z.number() }),
  ts: z.number(),
});

const IOGate3 = z.object({
  value: z.object({
    'HRC-position': z.number(),
    'fault-relay': z.number(),
    'fan-input': z.number(),
    'pre-heating': z.number(),
    'fan-output': z.number(),
    'booster-switch': z.number(),
  }),
  ts: z.number(),
});

// Digit vars schema
const DigitVarsInner = z.object({
  cur_fan_speed: NumericVar,
  outside_temp: NumericVar,
  exhaust_temp: NumericVar,
  inside_temp: NumericVar,
  incoming_temp: NumericVar,
  post_heating_on_cnt: NumericVar,
  post_heating_off_cnt: NumericVar,
  incoming_target_temp: NumericVar,
  panel_leds: PanelLeds,
  max_fan_speed: NumericVar,
  min_fan_speed: NumericVar,
  hrc_bypass_temp: NumericVar,
  input_fan_stop_temp: NumericVar,
  cell_defrosting_hysteresis: NumericVar,
  dc_fan_input: NumericVar,
  dc_fan_output: NumericVar,
  flags_2: StringVar,
  flags_4: StringVar,
  flags_5: StringVar,
  flags_6: StringVar,
  rh_max: NumericVar,
  rh1_sensor: NumericVar,
  basic_rh_level: NumericVar,
  pre_heating_temp: NumericVar,
  IO_gate_1: IOGate1,
  IO_gate_2: IOGate2,
  IO_gate_3: IOGate3,
});

export const DigitVarsResponse = z.object({
  digit_vars: DigitVarsInner,
});

// Control vars schema
const ControlVarsInner = z.object({
  pre_heating_time: NumericVar,
  post_heating_time: NumericVar,
  defrost_time: NumericVar,
  defrost_on_time: NumericVar,
  defrost_mode: NumericVar,
  dew_point: NumericVar,
  min_exhaust_temp: NumericVar,
  in_efficiency: NumericVar,
  out_efficiency: NumericVar,
  in_efficiency_filtered: NumericVar,
  out_efficiency_filtered: NumericVar,
  defrost_start_level: NumericVar,
  defrost_target_in_eff: NumericVar,
  defrost_target_temp: NumericVar,
  defrost_max_duration: NumericVar,
  pressure_outdoor: NumericVar,
  pressure_indoor: NumericVar,
  pressure_diff: NumericVar,
  pressure_offset: NumericVar,
  defrost_start_duration: NumericVar,
  fireplace_active: NumericVar,
  fireplace_max_time: NumericVar,
  fireplace_remaining: NumericVar,
});

export const ControlVarsResponse = z.object({
  control_vars: ControlVarsInner,
});

// DS18B20 vars schema
const DS18B20VarsInner = z.object({
  ds_outside_temp: NumericVar,
  ds_exhaust_temp: NumericVar,
  ds_incoming_temp: NumericVar,
});

export const DS18B20VarsResponse = z.object({
  ds18b20_vars: DS18B20VarsInner,
});

// Combined device status (after parsing)
export type DigitVars = z.infer<typeof DigitVarsInner>;
export type ControlVars = z.infer<typeof ControlVarsInner>;
export type DS18B20Vars = z.infer<typeof DS18B20VarsInner>;

export interface DeviceStatus {
  digitVars: DigitVars;
  controlVars: ControlVars;
  ds18b20Vars: DS18B20Vars;
}
