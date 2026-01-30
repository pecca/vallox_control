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
        
    elif scenario == 'preemptive':
        # THE USER'S GOAL: Freezing has started (Dew < Exhaust), but Efficiency is still ok-ish.
        # We want to catch this!
        start_eff = np.random.uniform(60, 72) # Dropping, but not "Low" yet
        outside_temp = np.random.uniform(-15, -2)
        # Dew point is RISK (detectable physics condition)
        exhaust_temp = 5 + (start_eff - 50) * 0.2
        dew_point = exhaust_temp - np.random.uniform(1, 4) # IS COLDER!
        
        # If we run a cycle here -> Quick clean!
        end_reason = 1 # Valid Defrost
        actual_heating = 400 + np.random.uniform(0, 100) # Fast (approx 7-8 mins)
        end_eff = np.random.uniform(80, 85) # Back to perfect

    else: # 'reactive' (Old logic)
        # We waited too long. Efficiency tanked.
        start_eff = np.random.uniform(30, 50)
        outside_temp = np.random.uniform(-20, -5)
        exhaust_temp = 2 # Cold
        dew_point = -5 # Freezing
        
        # Heavy ice.
        end_reason = 1 # Valid Defrost (Necessary)
        actual_heating = 1200 + np.random.uniform(0, 600) # Long (20-30 mins)
        end_eff = np.random.uniform(75, 85)

    return {
        'cycle_number': 1000 + index,
        'timestamp': datetime.now() - timedelta(hours=NUM_SAMPLES - index),
        'heating_duration': int(actual_heating),
        'fan_stop_duration': int(np.random.uniform(0, 300)),
        'total_duration': int(actual_heating + 300),
        'start_outside_temp': round(outside_temp, 1),
        'start_in_eff': round(start_eff, 1),
        'start_in_eff_filtered': round(start_eff, 1),
        'start_exhaust_temp': round(exhaust_temp, 1),
        'start_incoming_temp': round(outside_temp + 10, 1),
        'start_dew_point': round(dew_point, 1),
        'end_in_eff': round(end_eff, 1),
        'end_reason': end_reason,
        'fan_speed': 50,
        'humidity': 60.0,
        'scenario': scenario # Debug info
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
