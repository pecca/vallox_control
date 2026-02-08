#!/bin/bash

# Configuration
PROJECT_ID="pekan-vallox-control"
REGION="europe-north1"
FUNCTION_NAME="predictDefrost"
SOURCE_DIR="cloud-functions-predict"
ENTRY_POINT="predictDefrost"

# Prediction Mode: 'rules' or 'vertex_ai'
# Set VERTEX_ENDPOINT_ID if using 'vertex_ai'
PREDICTION_MODE="vertex_ai"
VERTEX_ENDPOINT_ID="4281080464141189120" 

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
    --memory=512Mi \
    --timeout=60s \
    --set-env-vars PROJECT_ID=$PROJECT_ID,PREDICTION_MODE=$PREDICTION_MODE,VERTEX_ENDPOINT_ID=$VERTEX_ENDPOINT_ID,VALLOX_API_TOKEN=huuhaa \
    --project=$PROJECT_ID

echo "Done."
