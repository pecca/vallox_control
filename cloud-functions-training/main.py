import functions_framework
import logging
from ai_learner import (
    fetch_defrost_data,
    analyze_cycles,
    prepare_training_data,
    train_models,
    export_to_gcs,
    save_model_artifacts,
    upload_and_deploy
)

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

@functions_framework.http
def train_defrost_model(request):
    """HTTP Cloud Function to trigger AI model training."""
    try:
        logger.info("Starting training job...")
        
        # 1. Fetch data
        df = fetch_defrost_data()
        if df.empty:
            return "No data available.", 200
        
        if len(df) < 10:
            return f"Insufficient data: {len(df)} cycles.", 200

        # 2. Analyze
        analyze_cycles(df)

        # 3. Prepare
        clf_X, clf_y, reg_X, reg_y, scaler, available_features = prepare_training_data(df)

        # 4. Export Data (Optional handling)
        try:
            export_to_gcs(df, available_features)
        except Exception as e:
            logger.warning(f"GCS export failed: {e}")

        # 5. Train
        classifier, regressor = train_models(clf_X, clf_y, reg_X, reg_y, available_features)

        # 6. Save Artifacts (Optional handling)
        artifact_uri = None
        try:
            artifact_uri = save_model_artifacts(classifier, regressor, scaler, available_features)
        except Exception as e:
            logger.warning(f"GCS artifact upload failed: {e}")

        # 7. Deploy to Vertex AI
        if artifact_uri:
            try:
                upload_and_deploy(artifact_uri)
                logger.info("Successfully deployed new model to Vertex AI")
            except Exception as e:
                logger.error(f"Vertex AI deployment failed: {e}")

        return f"Training complete. Artifacts: {artifact_uri}. Model deployed to Vertex AI.", 200

    except Exception as e:
        logger.error(f"Training job failed: {e}")
        return f"Error: {e}", 500
