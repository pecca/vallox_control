import os
import random
from datetime import datetime, timedelta
from google.cloud import firestore
import numpy as np

# Configuration
PROJECT_ID = os.getenv('GCP_PROJECT_ID', 'pekan-vallox-control')
COLLECTION_NAME = 'defrost_cycles'
NUM_SAMPLES = 100

def generate_cycle(index):
    """Generates a realistic-looking defrost cycle."""
    
    # 1. Randomize standard conditions
    outside_temp = np.random.uniform(-20, 5)  # -20C to +5C
    fan_speed = random.choice([30, 40, 50, 60, 70])
    
    
    # Generate Physics Scenarios
    scenario = np.random.choice(['clean', 'preemptive', 'reactive'])
    
    if scenario == 'clean':
        # Everything is fine. Defrosting now would be a waste.
        start_eff = np.random.uniform(70, 85)
        outside_temp = np.random.uniform(-5, 10)
        # Dew point is SAFE (higher than exhaust or very dry)
        exhaust_temp = 5 + (start_eff - 50) * 0.2
        dew_point = exhaust_temp + np.random.uniform(2, 5) 
        
        # If we ran a cycle here...
        end_reason = 0 # "Failure" in the sense of "Bad decision / Not needed"
        actual_heating = 300 # Short, nothing happened
        end_eff = start_eff # No change
        
        # Missing definitions for clean
        start_exhaust_humidity = np.random.uniform(30, 60) # Dry/Normal
        start_supply_temp = outside_temp + (21 - outside_temp) * (start_eff / 100.0)
        start_pressure_diff = 30 + (fan_speed * 0.8) # Base pressure only
        
    elif scenario == 'preemptive':
        # THE USER'S GOAL: Freezing has started (Dew < Exhaust), but Efficiency is still ok-ish.
        # We want to catch this!
        start_eff = np.random.uniform(60, 72) # Dropping, but not "Low" yet
        outside_temp = np.random.uniform(-15, -2)
        # Dew point is RISK (detectable physics condition)
        exhaust_temp = 5 + (start_eff - 50) * 0.2
        dew_point = exhaust_temp - np.random.uniform(1, 4) # IS COLDER!
        
        
        # New Features Logic
        start_exhaust_humidity = np.random.uniform(40, 80)
        # If Risk (Preemptive/Reactive), humidity is likely higher (cooking/showering)
        if scenario != 'clean':
            start_exhaust_humidity = np.random.uniform(70, 95)
            
        # Supply Temp approx: Outside + (Inside-Outside)*Eff
        # Assume Inside is 21C
        start_supply_temp = outside_temp + (21 - outside_temp) * (start_eff / 100.0)
        
        # Pressure Drop: Base (Fans) + Ice
        # Base approx 50-80 Pa depending on fan speed
        base_pressure = 30 + (fan_speed * 0.8) 
        # Ice effect: Lower efficiency ~ More ice ~ Higher pressure
        ice_pressure = 0
        if start_eff < 60:
            ice_pressure = (60 - start_eff) * 2 # 20% drop -> +40Pa
        start_pressure_diff = base_pressure + ice_pressure
        
        # Valid Defrost
        actual_heating = 400 + np.random.uniform(0, 100) 
        end_reason = 1
        end_eff = np.random.uniform(80, 85) 

    else: # 'reactive'
        # ... (keep existing logic but update variable generation)
        start_eff = np.random.uniform(30, 50)
        outside_temp = np.random.uniform(-20, -5)
        # High moisture
        start_exhaust_humidity = np.random.uniform(85, 99)
        
        exhaust_temp = 2
        dew_point = -5
        
        start_supply_temp = outside_temp + (21 - outside_temp) * (start_eff / 100.0)
        
        # Significant Ice Blockage
        base_pressure = 30 + (50 * 0.8) # Assume 50% fan
        ice_pressure = (60 - start_eff) * 3 # Big pressure wall
        start_pressure_diff = base_pressure + ice_pressure

        # Heavy Valid Defrost
        end_reason = 1
        actual_heating = 1200 + np.random.uniform(0, 600)
        end_eff = np.random.uniform(75, 85)
        
    # Recalculate Dew Point Delta for all
    start_dew_point_delta = exhaust_temp - dew_point
        
    return {
        'cycle_number': 1000 + index,
        'timestamp': datetime.now() - timedelta(hours=NUM_SAMPLES - index),
        'heating_duration': int(actual_heating),
        'fan_stop_duration': int(np.random.uniform(0, 300)),
        'total_duration': int(actual_heating + 300),
        'start_outside_temp': round(outside_temp, 1),
        'start_exhaust_temp': round(exhaust_temp, 1),
        'start_exhaust_humidity': round(start_exhaust_humidity, 1),
        'start_supply_temp': round(start_supply_temp, 1),
        'start_pressure_diff': round(start_pressure_diff, 1),
        'start_fan_speed': fan_speed,
        'start_dew_point_delta': round(start_dew_point_delta, 2),
        'start_in_eff': round(start_eff, 1),
        'start_in_eff_filtered': round(start_eff, 1),
        'end_in_eff': round(end_eff, 1),
        'end_reason': end_reason,
        'scenario': scenario
    }

def main():
    print(f"Connecting to {PROJECT_ID}...")
    db = firestore.Client(project=PROJECT_ID)
    batch = db.batch()
    
    print(f"Generating {NUM_SAMPLES} cycles...")
    for i in range(NUM_SAMPLES):
        data = generate_cycle(i)
        doc_ref = db.collection(COLLECTION_NAME).document(f"sim-{data['cycle_number']}")
        batch.set(doc_ref, data)
        
        # Firestore batch limit is 500, we do 100 so one batch is fine
    
    batch.commit()
    print("Done! Data uploaded.")

if __name__ == "__main__":
    main()
