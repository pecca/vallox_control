#!/usr/bin/env python3
"""
Vallox Defrost AI Model Training Pipeline

Fetches defrost cycle data from Firestore, trains scikit-learn models,
uploads artifacts to GCS, and optionally deploys to Vertex AI Endpoint.

Usage:
    python train_model.py                    # Train and evaluate only
    python train_model.py --deploy           # Train + deploy to Vertex AI
    python train_model.py --min-cycles 50    # Require minimum training samples
    python train_model.py --export-only      # Export data to GCS without training
"""

import argparse
import sys

from ai_learner import (
    fetch_defrost_data,
    analyze_cycles,
    prepare_training_data,
    train_models,
    export_to_gcs,
    save_model_artifacts,
    upload_and_deploy,
)


def main():
    parser = argparse.ArgumentParser(
        description='Vallox Defrost AI Model Training Pipeline'
    )
    parser.add_argument(
        '--min-cycles', type=int, default=20,
        help='Minimum number of defrost cycles required for training (default: 20)',
    )
    parser.add_argument(
        '--deploy', action='store_true',
        help='Deploy trained model to Vertex AI Endpoint',
    )
    parser.add_argument(
        '--export-only', action='store_true',
        help='Export training data to GCS without training',
    )
    args = parser.parse_args()

    # 1. Fetch data from Firestore
    print("=" * 50)
    print("Step 1: Fetching data from Firestore")
    print("=" * 50)
    df = fetch_defrost_data()

    if df.empty:
        print("No data available. Exiting.")
        sys.exit(1)

    if len(df) < args.min_cycles:
        print(f"Insufficient data: {len(df)} cycles (need {args.min_cycles}). Exiting.")
        sys.exit(1)

    # 2. Analyze
    analyze_cycles(df)

    # 3. Prepare training data
    print("\n" + "=" * 50)
    print("Step 2: Preparing training data")
    print("=" * 50)
    clf_X, clf_y, reg_X, reg_y, scaler, available_features = prepare_training_data(df)

    # 4. Export to GCS
    print("\n" + "=" * 50)
    print("Step 3: Exporting data to GCS")
    print("=" * 50)
    try:
        export_to_gcs(df, available_features)
    except Exception as e:
        print(f"Warning: GCS export failed: {e}")
        print("Continuing with local training...")

    if args.export_only:
        print("\n--export-only flag set. Skipping training.")
        return

    # 5. Train models
    print("\n" + "=" * 50)
    print("Step 4: Training models")
    print("=" * 50)
    classifier, regressor = train_models(clf_X, clf_y, reg_X, reg_y, available_features)

    # 6. Save artifacts to GCS
    print("\n" + "=" * 50)
    print("Step 5: Saving model artifacts to GCS")
    print("=" * 50)
    try:
        artifact_uri = save_model_artifacts(classifier, regressor, scaler, available_features)
    except Exception as e:
        print(f"Warning: GCS artifact upload failed: {e}")
        artifact_uri = None

    # 7. Deploy to Vertex AI
    if args.deploy:
        print("\n" + "=" * 50)
        print("Step 6: Deploying to Vertex AI Endpoint")
        print("=" * 50)
        endpoint = upload_and_deploy(artifact_uri)
        print(f"\nDeployment complete. Endpoint: {endpoint.resource_name}")
    else:
        print("\nSkipping deployment (use --deploy to deploy to Vertex AI).")
        print(f"Model artifacts at: {artifact_uri}")

    print("\nDone.")


if __name__ == "__main__":
    main()
