import os
import tempfile
import logging
from datetime import datetime

from google.cloud import firestore
from google.cloud import storage
import pandas as pd
import numpy as np
from sklearn.ensemble import GradientBoostingClassifier, GradientBoostingRegressor
from sklearn.model_selection import cross_val_score
from sklearn.preprocessing import StandardScaler
import joblib

from ai_config import (
    PROJECT_ID, REGION, GCS_BUCKET, FIRESTORE_COLLECTION,
    TRAINING_FEATURES, SUCCESSFUL_END_REASONS,
    CLASSIFIER_DISPLAY_NAME, REGRESSOR_DISPLAY_NAME,
    SERVING_CONTAINER, ENDPOINT_DISPLAY_NAME,
)

logger = logging.getLogger(__name__)


def fetch_defrost_data():
    """Fetches defrost cycle data from Firestore and returns a DataFrame."""
    print(f"Connecting to Firestore Project: {PROJECT_ID}")
    db = firestore.Client(project=PROJECT_ID)

    cycles_ref = db.collection(FIRESTORE_COLLECTION)
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

    numeric_cols = ['total_duration', 'heating_duration', 'start_outside_temp',
                    'start_in_eff', 'end_in_eff']
    for col in numeric_cols:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    print("\n--- Defrost Cycle Statistics ---")

    if 'total_duration' in df.columns:
        print(f"Average Duration: {df['total_duration'].mean():.1f} s")
        print(f"Max Duration:     {df['total_duration'].max():.1f} s")

    if 'heating_duration' in df.columns:
        print(f"Avg Heating Dur:  {df['heating_duration'].mean():.1f} s")

    if 'start_outside_temp' in df.columns:
        print(f"Avg Start Temp:   {df['start_outside_temp'].mean():.1f} C")

    if 'start_in_eff' in df.columns:
        print(f"Avg Start Eff:    {df['start_in_eff'].mean():.1f} %")

    if 'end_in_eff' in df.columns:
        print(f"Avg End Eff:      {df['end_in_eff'].mean():.1f} %")

    if 'end_reason' in df.columns:
        print("\n--- End Reason Distribution ---")
        print(df['end_reason'].value_counts().to_string())


def prepare_training_data(df):
    """Prepare features and targets for model training.

    Returns:
        clf_X, clf_y: Features and binary target for classification
        reg_X, reg_y: Features and duration target for regression (successful cycles only)
        scaler: Fitted StandardScaler
    """
    if df.empty:
        raise ValueError("No data to prepare")

    # Ensure numeric types for all features
    for col in TRAINING_FEATURES + ['heating_duration', 'end_reason']:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')

    # Drop rows missing critical features
    available_features = [f for f in TRAINING_FEATURES if f in df.columns]

    # Recalculate start_in_eff if missing but components are present
    if 'start_in_eff' in TRAINING_FEATURES:
        missing_eff = df['start_in_eff'].isna() if 'start_in_eff' in df.columns else pd.Series([True] * len(df))
        if missing_eff.any():
            print(f"Attempting to calculate missing start_in_eff for {missing_eff.sum()} samples...")
            # Required: start_incoming_temp, start_ds_outside_temp (prefer), inside_temp
            # Fallback to start_outside_temp if start_ds_outside_temp is missing
            t_out_col = 'start_ds_outside_temp' if 'start_ds_outside_temp' in df.columns else 'start_outside_temp'
            
            can_calc = (
                df['start_incoming_temp'].notna() & 
                df[t_out_col].notna() & 
                df['inside_temp'].notna()
            )
            idx = missing_eff & can_calc
            if idx.any():
                # Formula: (T_supply - T_outdoor) / (T_extract - T_outdoor) * 100
                t_out = df.loc[idx, t_out_col]
                t_sup = df.loc[idx, 'start_incoming_temp']
                t_ext = df.loc[idx, 'inside_temp']
                
                # Avoid division by zero
                denom = t_ext - t_out
                denom = denom.replace(0, np.nan)
                
                calculated_eff = ((t_sup - t_out) / denom) * 100
                df.loc[idx, 'start_in_eff'] = calculated_eff.clip(0, 100)
                print(f"Successfully calculated start_in_eff for {idx.sum()} samples.")
                
                if 'start_in_eff' not in available_features:
                    available_features.append('start_in_eff')

    if not available_features:
        raise ValueError(f"None of the training features found. Available columns: {list(df.columns)}")

    missing_features = set(TRAINING_FEATURES) - set(available_features)
    if missing_features:
        print(f"Warning: Missing features (will be excluded): {missing_features}")

    df_clean = df.dropna(subset=available_features)
    print(f"Samples after dropping NaN: {len(df_clean)} (dropped {len(df) - len(df_clean)})")

    if len(df_clean) < 10:
        raise ValueError(f"Insufficient data: {len(df_clean)} samples (need at least 10)")

    # Fit scaler on all data
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(df_clean[available_features])

    # Classification: Predict if defrost will be successful (1) or fail (0)
    # Target: 1 if end_reason is in SUCCESSFUL_END_REASONS, else 0
    clf_X = X_scaled
    if 'end_reason' in df_clean.columns:
        clf_y = df_clean['end_reason'].isin(SUCCESSFUL_END_REASONS).astype(int).values
    else:
        # Fallback if no end_reason (shouldn't happen with valid data)
        clf_y = np.ones(len(df_clean))

    # Regression: predict heating duration for successful cycles only
    if 'end_reason' in df_clean.columns and 'heating_duration' in df_clean.columns:
        successful = df_clean['end_reason'].isin(SUCCESSFUL_END_REASONS)
        reg_df = df_clean[successful].dropna(subset=['heating_duration'])
        if len(reg_df) >= 5:
            reg_X = scaler.transform(reg_df[available_features])
            reg_y = reg_df['heating_duration'].values
            print(f"Regression samples (successful cycles): {len(reg_df)}")
        else:
            print(f"Warning: Only {len(reg_df)} successful cycles, skipping regression")
            reg_X, reg_y = None, None
    else:
        print("Warning: Missing end_reason or heating_duration, skipping regression")
        reg_X, reg_y = None, None

    return clf_X, clf_y, reg_X, reg_y, scaler, available_features


def train_models(clf_X, clf_y, reg_X, reg_y, available_features):
    """Train classification and regression models.

    Returns:
        classifier, regressor (regressor may be None)
    """
    print("\n--- Training Classification Model ---")
    classifier = GradientBoostingClassifier(
        n_estimators=100,
        learning_rate=0.1,
        max_depth=5,
        min_samples_split=10,
        random_state=42,
    )

    if len(clf_X) >= 20:
        cv_folds = min(5, len(clf_X) // 4)
        scores = cross_val_score(classifier, clf_X, clf_y, cv=cv_folds, scoring='f1')
        print(f"Cross-validation F1: {scores.mean():.3f} (+/- {scores.std():.3f})")

    classifier.fit(clf_X, clf_y)
    print(f"Training accuracy: {classifier.score(clf_X, clf_y):.3f}")

    # Feature Importance
    if hasattr(classifier, 'feature_importances_'):
        print("\nTop 3 Predictive Features (Classifier):")
        importances = classifier.feature_importances_
        indices = np.argsort(importances)[::-1]
        for i in range(min(3, len(indices))):
            idx = indices[i]
            if idx < len(available_features):
                print(f"  {i+1}. {available_features[idx]}: {importances[idx]:.3f}")


    regressor = None
    if reg_X is not None and reg_y is not None:
        print("\n--- Training Regression Model ---")
        regressor = GradientBoostingRegressor(
            n_estimators=100,
            learning_rate=0.1,
            max_depth=5,
            min_samples_split=10,
            loss='squared_error',
            random_state=42,
        )

        if len(reg_X) >= 20:
            cv_folds = min(5, len(reg_X) // 4)
            mae_scores = cross_val_score(regressor, reg_X, reg_y, cv=cv_folds,
                                         scoring='neg_mean_absolute_error')
            print(f"Cross-validation MAE: {-mae_scores.mean():.1f} s (+/- {mae_scores.std():.1f})")

        regressor.fit(reg_X, reg_y)
        predictions = regressor.predict(reg_X)
        rmse = np.sqrt(np.mean((predictions - reg_y) ** 2))
        print(f"Training RMSE: {rmse:.1f} s")

    return classifier, regressor


def export_to_gcs(df, available_features):
    """Export training data as CSV to GCS bucket."""
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    gcs_path = f"training-data/defrost_cycles_{timestamp}.csv"

    client = storage.Client(project=PROJECT_ID)
    bucket = client.bucket(GCS_BUCKET)

    cols = available_features + ['heating_duration', 'end_reason', 'total_duration']
    cols = [c for c in cols if c in df.columns]
    csv_data = df[cols].to_csv(index=False)

    blob = bucket.blob(gcs_path)
    blob.upload_from_string(csv_data, content_type='text/csv')
    gcs_uri = f"gs://{GCS_BUCKET}/{gcs_path}"
    print(f"Training data exported to {gcs_uri}")
    return gcs_uri


def save_model_artifacts(classifier, regressor, scaler, available_features):
    """Save model artifacts locally and upload to GCS.

    Returns:
        GCS URI of the model artifacts directory.
    """
    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    gcs_base_dir = f"models/v{timestamp}"
    
    # 1. Save standard artifacts (all components)
    with tempfile.TemporaryDirectory() as tmpdir:
        # Save artifacts locally
        joblib.dump(classifier, os.path.join(tmpdir, 'classifier.joblib'))
        joblib.dump(scaler, os.path.join(tmpdir, 'scaler.joblib'))
        if regressor is not None:
            joblib.dump(regressor, os.path.join(tmpdir, 'regressor.joblib'))

        # Save feature config
        import json
        metadata = {
            'features': available_features,
            'timestamp': timestamp,
            'has_regressor': regressor is not None,
        }
        with open(os.path.join(tmpdir, 'metadata.json'), 'w') as f:
            json.dump(metadata, f, indent=2)

        # Upload ALL artifacts to main GCS directory
        client = storage.Client(project=PROJECT_ID)
        bucket = client.bucket(GCS_BUCKET)

        for filename in os.listdir(tmpdir):
            blob = bucket.blob(f"{gcs_base_dir}/{filename}")
            blob.upload_from_filename(os.path.join(tmpdir, filename))
            
    # 2. Prepare deployment package (Only allowed files for Vertex AI)
    # The pre-built container expects exactly one 'model.joblib' or 'model.pkl'
    # We bundle the scaler and classifier into a Pipeline.
    from sklearn.pipeline import Pipeline
    deployment_pipeline = Pipeline([
        ('scaler', scaler),
        ('classifier', classifier)
    ])
    
    gcs_deploy_dir = f"{gcs_base_dir}/deployment"
    
    with tempfile.TemporaryDirectory() as deploy_tmp:
        joblib.dump(deployment_pipeline, os.path.join(deploy_tmp, 'model.joblib'))
        
        # Upload model.joblib to the deployment subdirectory
        blob = bucket.blob(f"{gcs_deploy_dir}/model.joblib")
        blob.upload_from_filename(os.path.join(deploy_tmp, 'model.joblib'))

    gcs_uri = f"gs://{GCS_BUCKET}/{gcs_deploy_dir}"
    print(f"Model artifacts uploaded to {gcs_uri}")
    return gcs_uri


def upload_and_deploy(artifact_uri):
    """Upload model to Vertex AI Model Registry and deploy to endpoint."""
    from google.cloud import aiplatform

    aiplatform.init(project=PROJECT_ID, location=REGION)

    # Upload classifier model
    print("\n--- Uploading model to Vertex AI ---")
    model = aiplatform.Model.upload(
        display_name=CLASSIFIER_DISPLAY_NAME,
        artifact_uri=artifact_uri,
        serving_container_image_uri=SERVING_CONTAINER,
    )
    print(f"Model uploaded: {model.resource_name}")

    # Get or create endpoint
    endpoints = aiplatform.Endpoint.list(
        filter=f'display_name="{ENDPOINT_DISPLAY_NAME}"',
    )

    if endpoints:
        endpoint = endpoints[0]
        print(f"Using existing endpoint: {endpoint.resource_name}")
    else:
        endpoint = aiplatform.Endpoint.create(
            display_name=ENDPOINT_DISPLAY_NAME,
        )
        print(f"Created endpoint: {endpoint.resource_name}")

    # Deploy model to endpoint
    print("Deploying model to endpoint...")
    model.deploy(
        endpoint=endpoint,
        machine_type='n1-standard-2',
        min_replica_count=1,
        max_replica_count=3,
        traffic_percentage=100,
    )
    print(f"Model deployed to endpoint: {endpoint.resource_name}")
    return endpoint


if __name__ == "__main__":
    df = fetch_defrost_data()
    analyze_cycles(df)
