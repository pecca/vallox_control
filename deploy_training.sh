#!/bin/bash
set -e

PROJECT_ID="pekan-vallox-control"
REGION="europe-north1"
FUNCTION_NAME="vallox-ai-trainer"
SOURCE_DIR="cloud-functions-training"

echo "Deploying $FUNCTION_NAME to $PROJECT_ID..."

gcloud functions deploy $FUNCTION_NAME \
  --gen2 \
  --runtime=python310 \
  --region=$REGION \
  --source=$SOURCE_DIR \
  --entry-point=train_defrost_model \
  --trigger-http \
  --allow-unauthenticated \
  --memory=1024MB \
  --timeout=300s \
  --set-env-vars GCP_PROJECT_ID=$PROJECT_ID,VALLOX_AI_BUCKET=vallox-ai-models

echo "Deployment submitted."
