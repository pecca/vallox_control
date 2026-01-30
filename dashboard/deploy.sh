#!/bin/bash

# Exit on error
set -e

# Load .env.production variables from the dashboard directory
ENV_FILE=".env.production"
if [ -f "$ENV_FILE" ]; then
  export $(grep -v '^#' "$ENV_FILE" | xargs)
  echo "✅ Loaded environment variables from $ENV_FILE"
else
  echo "❌ Environment file $ENV_FILE not found in $(pwd)"
  echo "Please create a $ENV_FILE file for deployment."
  exit 1
fi

# Configuration
SERVICE_NAME="vallox-dashboard"
REGION="europe-north1"
PROJECT_ID=${GCP_PROJECT_ID:-"pekan-vallox-control"}

echo "🚀 Deploying $SERVICE_NAME to $PROJECT_ID in $REGION..."

# Construct the env vars string for gcloud
# We filter for specific vars we want to pass to Cloud Run
ENV_VARS="GCP_PROJECT_ID=$GCP_PROJECT_ID,\
VALLOX_API_URL=$VALLOX_API_URL,\
VALLOX_API_TOKEN=$VALLOX_API_TOKEN,\
NEXTAUTH_SECRET=$NEXTAUTH_SECRET"

# Optional: Add NEXTAUTH_URL if it exists in .env
if [ ! -z "$NEXTAUTH_URL" ]; then
  ENV_VARS="$ENV_VARS,NEXTAUTH_URL=$NEXTAUTH_URL"
fi

gcloud run deploy "$SERVICE_NAME" \
  --source . \
  --region "$REGION" \
  --project "$PROJECT_ID" \
  --allow-unauthenticated \
  --set-env-vars "$ENV_VARS"

echo "✅ Deployment complete!"
