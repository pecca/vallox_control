# Vallox Control Dashboard

Next.js dashboard for Vallox 150 SE MLV ventilation unit. Deployed on GCP Cloud Run.

## Prerequisites

- [Google Cloud SDK](https://cloud.google.com/sdk/docs/install) installed and authenticated
- GCP project with Firestore, Pub/Sub, and Cloud Run APIs enabled

## Local Development

```bash
cp .env.example .env
# Edit .env with your values
npm install
npm run dev
```

## GCP Setup (First Time)

### 1. Enable APIs

```bash
gcloud services enable \
  run.googleapis.com \
  firestore.googleapis.com \
  cloudbuild.googleapis.com \
  artifactregistry.googleapis.com
```

### 2. Create Firestore Database (if not existing)

```bash
gcloud firestore databases create --location=europe-north1
```

### 3. Seed a User

```bash
gcloud auth application-default login

GCP_PROJECT_ID=pekan-vallox-control npx ts-node scripts/seed-user.ts <username> <password>
```

### 4. Deploy to Cloud Run

```bash
gcloud run deploy vallox-dashboard \
  --source=. \
  --region=europe-north1 \
  --allow-unauthenticated \
  --set-env-vars="\
VALLOX_API_URL=http://pekanraspi.duckdns.org:9000,\
VALLOX_API_TOKEN=<your-api-token>,\
NEXTAUTH_SECRET=$(openssl rand -base64 32),\
GCP_PROJECT_ID=pekan-vallox-control"
```

### 5. Set NEXTAUTH_URL

After the first deploy, note the service URL from the output and update:

```bash
gcloud run services update vallox-dashboard \
  --region=europe-north1 \
  --update-env-vars="NEXTAUTH_URL=https://<your-service-url>.run.app"
```

## Redeploying (Code Changes)

```bash
gcloud run deploy vallox-dashboard --source=. --region=europe-north1
```

Existing environment variables are preserved.

## Environment Variables

| Variable | Description | Required |
|----------|-------------|----------|
| `VALLOX_API_URL` | Pi API base URL (e.g. `http://pekanraspi.duckdns.org:9000`) | Yes |
| `VALLOX_API_TOKEN` | API auth token | Yes |
| `NEXTAUTH_SECRET` | Random secret for JWT signing | Yes |
| `NEXTAUTH_URL` | Public URL of the dashboard | Yes (production) |
| `GCP_PROJECT_ID` | GCP project ID for Firestore | Yes |

## Architecture

```
Browser -> Cloud Run (Next.js)
              |-- Server Actions -> vallox-control-api (Raspberry Pi)
              |-- Server Components -> Firestore (historical data)
              +-- NextAuth.js -> Firestore (user credentials)
```

- **Live dashboard** polls device status every 5 seconds via Server Actions
- **History page** queries Firestore `vallox_status` collection (populated by Pub/Sub pipeline)
- **Auth** uses NextAuth.js with credentials stored in Firestore `users` collection
