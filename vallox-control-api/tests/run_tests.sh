#!/bin/bash

# Load .env if it exists
if [ -f .env ]; then
  export $(grep -v '^#' .env | xargs)
fi

# Default values if not in .env
API_PORT=${PORT:-3000}
#BASE_URL="http://localhost:$API_PORT"
BASE_URL="http://pekanraspi.duckdns.org:$API_PORT"
API_TOKEN=${TOKEN:-""}

echo "Checking if server is running at $BASE_URL..."
if ! curl -s "$BASE_URL/api/vallox?token=$API_TOKEN&action=get&type=digit_vars" > /dev/null; then
  echo "❌ Error: Server is not running at $BASE_URL"
  echo "Please start the server first with 'npm run dev' or 'npm start'"
  exit 1
fi

echo "Running Hurl tests against $BASE_URL..."

hurl --test tests/api.hurl \
  --variable base_url="$BASE_URL" \
  --variable token="$API_TOKEN" \
  --verbose
