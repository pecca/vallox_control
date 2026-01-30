#!/bin/bash

# Configuration
PROJECT_ID="pekan-vallox-control"
REGION="europe-north1"
FUNCTION_NAME="predictDefrost"
SOURCE_DIR="cloud-functions-predict"
ENTRY_POINT="predictDefrost"

echo "Deploying $FUNCTION_NAME to $PROJECT_ID..."

# Ensure we are in the root directory
cd "$(dirname "$0")"

# 1. Build TypeScript
echo "Building TypeScript..."
cd $SOURCE_DIR
npm install
npm run build
cd ..

# 2. Deploy
gcloud functions deploy $FUNCTION_NAME \
    --gen2 \
    --runtime=nodejs20 \
    --region=$REGION \
    --source=$SOURCE_DIR \
    --entry-point=$ENTRY_POINT \
    --trigger-http \
    --allow-unauthenticated \
    --memory=256Mi \
    --timeout=60s \
    --set-env-vars PROJECT_ID=$PROJECT_ID,PREDICTION_MODE=rules,VALLOX_API_TOKEN=huuhaa \
    --project=$PROJECT_ID

echo "Done."
