"""Shared configuration for Vallox AI defrost prediction system."""

import os

# GCP Configuration
PROJECT_ID = os.getenv('GCP_PROJECT_ID', 'pekan-vallox-control')
REGION = 'europe-north1'
GCS_BUCKET = os.getenv('VALLOX_AI_BUCKET', 'vallox-ai-models')
ENDPOINT_DISPLAY_NAME = 'vallox-defrost-predictor'
FIRESTORE_COLLECTION = 'defrost_cycles'

# Prediction rate limiting
PREDICTION_INTERVAL_SEC = 60       # Min seconds between predictions
PREDICTION_TIMEOUT_SEC = 5         # Vertex AI call timeout
MIN_HEAT_INTERVAL_SEC = 600        # Min 10 minutes between heat-start commands
MAX_HEATING_DURATION_SEC = 1800    # Cap predicted duration at 30 min

# Model configuration
CLASSIFIER_DISPLAY_NAME = 'defrost-classifier'
REGRESSOR_DISPLAY_NAME = 'defrost-regressor'
SERVING_CONTAINER = 'europe-docker.pkg.dev/vertex-ai/prediction/sklearn-cpu.1-3:latest'

# Training features (must match Firestore field names from cloud-functions)
TRAINING_FEATURES = [
    'start_outside_temp',
    'start_in_eff',
    'start_in_eff_filtered',
    'start_exhaust_temp',
    'start_incoming_temp',
    'start_dew_point',
    'humidity',
    'fan_speed',
]

# Mapping from live UDP variable names to training feature names.
# Format: feature_name -> (var_group, json_key)
# The var_group is one of: control_vars, digit_vars, ds18b20_vars
# The json_key is the flat key in the JSON response, e.g. {"in_efficiency": {"value": 75.3, "ts": ...}}
LIVE_FEATURE_MAPPING = {
    'start_outside_temp': ('digit_vars', 'outside_temp'),
    'start_in_eff': ('control_vars', 'in_efficiency'),
    'start_in_eff_filtered': ('control_vars', 'in_efficiency_filtered'),
    'start_exhaust_temp': ('digit_vars', 'exhaust_temp'),
    'start_incoming_temp': ('digit_vars', 'incoming_temp'),
    'start_dew_point': ('control_vars', 'dew_point'),
    'humidity': ('digit_vars', 'rh1_sensor'),
    'fan_speed': ('digit_vars', 'cur_fan_speed'),
}

# Successful defrost end reasons (from C firmware ctrl_logic.h)
SUCCESSFUL_END_REASONS = [1, 2]  # e_EndReason_EffRecovered, e_EndReason_TempTarget

# Defrost mode and state constants (from C firmware)
DEFROST_MODE_AI = 3
DEFROST_STATE_MEASURING = 0
