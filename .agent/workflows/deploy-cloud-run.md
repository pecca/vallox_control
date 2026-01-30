---
description: Steps to deploy the Dashboard to GCP Cloud Run
---

### Prerequisites

1. Ensure you have the [Google Cloud SDK](https://cloud.google.com/sdk/docs/install) installed and authenticated.
2. Ensure your `.env.production` file in the `dashboard/` directory is up to date. This file is used specifically for deployment, while `.env` remains for local development.

### Deployment Steps

1. **Run the deployment script**
   Navigate to the `dashboard/` directory and run the `deploy.sh` script. It will automatically load your `.env.production` values and handle the Cloud Build/Cloud Run process.

   ```bash
   cd dashboard
   ./deploy.sh
   ```

2. **Verify Deployment**
   Once finished, the script will output the Service URL. Visit that URL to verify the dashboard is running.

### Technical Details

- **Script**: `dashboard/deploy.sh` loads variables from `.env.production`.
- **Environment Separation**: Keeps development credentials (`.env`) separate from production configuration.
- **Dockerfile**: The deployment uses the `dashboard/Dockerfile`.
- **Optimization**: `output: 'standalone'` in `next.config.ts` minimizes the container size.
- **Port**: Cloud Run defaults to port `8080`, which is correctly configured.
