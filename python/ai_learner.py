import os
from google.cloud import firestore
import pandas as pd
import numpy as np

# Configuration
PROJECT_ID = os.getenv('GCP_PROJECT_ID', 'pekan-vallox-control')
COLLECTION_NAME = 'defrost_cycles'

def fetch_defrost_data():
    """Fetches defrost cycle data from Firestore and returns a DataFrame."""
    print(f"Connecting to Firestore Project: {PROJECT_ID}")
    db = firestore.Client(project=PROJECT_ID)
    
    cycles_ref = db.collection(COLLECTION_NAME)
    docs = cycles_ref.stream()
    
    data = []
    for doc in docs:
        d = doc.to_dict()
        d['id'] = doc.id
        data.append(d)
    
    if not data:
        print("No defrost cycles found in database.")
        return pd.DataFrame()
    
    df = pd.DataFrame(data)
    print(f"Fetched {len(df)} cycles.")
    return df

def analyze_cycles(df):
    """Performs basic analysis on the defrost cycles."""
    if df.empty:
        return

    # Ensure numeric types
    numeric_cols = ['total_duration', 'heating_duration', 'start_outside_temp', 'start_in_eff', 'end_in_eff']
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    print("\n--- Defrost Cycle Statistics ---")
    
    if 'total_duration' in df.columns:
        print(f"Average Duration: {df['total_duration'].mean():.1f} s")
        print(f"Max Duration:     {df['total_duration'].max():.1f} s")
        
    if 'heating_duration' in df.columns:
        print(f"Avg Heating Date: {df['heating_duration'].mean():.1f} s")

    if 'start_outside_temp' in df.columns:
        print(f"Avg Start Temp:   {df['start_outside_temp'].mean():.1f} C")
        
    if 'start_in_eff' in df.columns:
        print(f"Avg Start Eff:    {df['start_in_eff'].mean():.1f} %")
        
    if 'end_in_eff' in df.columns:
        print(f"Avg End Eff:      {df['end_in_eff'].mean():.1f} %")

    print("\n--- Learning Mode Status ---")
    print("AI is observing execution. No control actions taken.")

if __name__ == "__main__":
    df = fetch_defrost_data()
    analyze_cycles(df)
