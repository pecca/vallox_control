#!/bin/bash

# Load .env if it exists
if [ -f .env ]; then
  export $(grep -v '^#' .env | xargs)
fi

# Default values if not in .env
API_PORT=${PORT:-3000}
BASE_URL="http://localhost:$API_PORT"
API_TOKEN=${TOKEN:-""}

echo "Running Hurl tests against $BASE_URL..."

hurl --test tests/api.hurl \
  --variable base_url="$BASE_URL" \
  --variable token="$API_TOKEN" \
  --verbose
