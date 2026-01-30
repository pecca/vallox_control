# Vallox Control System - Complete Architecture

## System Overview

```mermaid
graph TB
    subgraph "Vallox HVAC Unit"
        HVAC[Vallox 150 SE MLV]
    end

    subgraph "Raspberry Pi"
        FIRMWARE[C Firmware<br>vallox_ctrl]
        SENSORS[DS18B20 Sensors<br>BMP280 Pressure]

        FIRMWARE <-->|RS485 / DIGIT Protocol| HVAC
        SENSORS -->|1-Wire, I2C| FIRMWARE
    end

    subgraph "Cloud VM (91.157.190.137)"
        API[Node.js API<br>Express :9000]
    end

    subgraph "GCP europe-north1"
        SCHEDULER[Cloud Scheduler<br>Every 60s]
        CF_STATUS[Cloud Function<br>processValloxStatus<br>Pub/Sub trigger]
        CF_PREDICT[Cloud Function<br>predictDefrost<br>HTTP trigger]
        FIRESTORE[(Firestore)]
        PUBSUB[Pub/Sub<br>vallox-status topic]
    end

    subgraph "Client"
        DASHBOARD[Dashboard<br>Next.js 15]
    end

    API <-->|UDP :8056<br>JSON| FIRMWARE
    API -->|Publish every 5 min| PUBSUB
    PUBSUB -->|Trigger| CF_STATUS
    CF_STATUS -->|Write| FIRESTORE

    SCHEDULER -->|HTTP trigger| CF_PREDICT
    CF_PREDICT -->|GET sensor data| API
    CF_PREDICT -->|POST commands| API
    CF_PREDICT -->|Read/Write state| FIRESTORE
    CF_PREDICT -->|Log predictions| FIRESTORE

    DASHBOARD <-->|REST API :9000| API
    DASHBOARD -->|History queries| FIRESTORE
```

## Deployment Map

```mermaid
graph LR
    subgraph "Raspberry Pi<br>pekanraspi.duckdns.org"
        direction TB
        FW[vallox_ctrl<br>C executable<br>6 pthreads]
        HW[GPIO / RS485 / 1-Wire / I2C]
    end

    subgraph "Cloud VM<br>91.157.190.137"
        direction TB
        API2[Node.js API<br>port 9000]
    end

    subgraph "GCP<br>pekan-vallox-control"
        direction TB
        SCH[Cloud Scheduler]
        CF1[processValloxStatus]
        CF2[predictDefrost]
        FS2[(Firestore)]
    end

    subgraph "Browser"
        DASH2[Next.js Dashboard]
    end
```

## C Firmware Threading Model

```mermaid
graph TB
    subgraph "vallox_ctrl - 6 Pthreads"
        T1[DS18B20 Thread<br>Reads temperature sensors<br>1-Wire bus]
        T2[DIGIT Receive Thread<br>RS485 protocol reception<br>from Vallox HVAC]
        T3[DIGIT Update Thread<br>RS485 protocol processing<br>5-second cycle]
        T4[UDP Server Thread<br>Port 8056<br>JSON GET/SET API]
        T5[UDP Server Thread<br>Port 8057<br>JSON GET/SET API]
        T6[Control Logic Thread<br>10-second cycle<br>Defrost / Heating / Relays]
    end

    subgraph "Shared State (mutex protected)"
        DV[digit_vars<br>fan speed, temps, humidity<br>LED states, flags]
        CV[control_vars<br>efficiency, dew point<br>defrost state, AI commands]
        DS[ds18b20_vars<br>5x temperature sensors]
    end

    T1 --> DS
    T2 --> DV
    T3 --> DV
    T4 --> DV & CV & DS
    T5 --> DV & CV & DS
    T6 --> CV
```

## Data Flow - Normal Operation

```mermaid
sequenceDiagram
    participant HVAC as Vallox HVAC
    participant FW as C Firmware
    participant API as Node.js API
    participant PS as Pub/Sub
    participant CF as processValloxStatus
    participant FS as Firestore
    participant DASH as Dashboard

    Note over FW: Continuous sensor reading
    HVAC->>FW: RS485 DIGIT protocol (fan speed, temps)
    FW->>FW: DS18B20 sensors, BMP280, efficiency calc

    loop Every 5 minutes
        API->>FW: UDP get digit_vars + control_vars + ds18b20_vars
        FW-->>API: JSON responses
        API->>PS: Publish combined status
        PS->>CF: Trigger
        CF->>FS: Store to vallox_status
        CF->>CF: Check defrost_cycle_count
        opt New cycle completed
            CF->>FS: Store to defrost_cycles
        end
    end

    loop Dashboard polling (5s)
        DASH->>API: GET /api/vallox/status?type=digit_vars
        API->>FW: UDP {"get":"digit_vars"}
        FW-->>API: JSON
        API-->>DASH: HTTP response
    end
```

## Data Flow - AI Defrost Cycle

The Cloud Function (`predictDefrost`) is fully self-contained. It fetches sensor data from the Node.js API, runs the defrost algorithm, sends commands back through the API, and persists its state in Firestore.

```mermaid
sequenceDiagram
    participant FW as C Firmware
    participant API as Node.js API
    participant SCH as Cloud Scheduler
    participant CF as predictDefrost
    participant FS as Firestore

    Note over CF: AI mode active (defrost_mode=3)
    Note over CF: State in Firestore: MEASURING

    loop Every 60 seconds
        SCH->>CF: HTTP trigger
        CF->>API: GET /api/vallox/status?type=control_vars
        API->>FW: UDP get control_vars
        FW-->>API: JSON
        API-->>CF: control_vars response
        CF->>API: GET /api/vallox/status?type=digit_vars
        API->>FW: UDP get digit_vars
        FW-->>API: JSON
        API-->>CF: digit_vars response
        CF->>FS: Load state (ai_defrost/state)
        CF->>CF: Check: filtered_eff < 73% AND trending down?
        CF-->>SCH: {action: "none", reason: "efficiency_ok"}
    end

    Note over CF: filtered_eff drops below 73%
    SCH->>CF: HTTP trigger
    CF->>API: GET control_vars + digit_vars
    CF->>FS: Load state
    CF->>CF: Rule: start heating
    CF->>API: POST /api/vallox/control ai_defrost_heating=1
    API->>FW: UDP SET ai_defrost_heating=1
    CF->>FS: Save state (HEATING, timestamp)
    CF->>FS: Log prediction
    Note over FW: Defrost resistor ON

    loop Every 60 seconds (heating)
        SCH->>CF: HTTP trigger
        CF->>API: GET control_vars + digit_vars
        CF->>FS: Load state (calculate heating_elapsed)
        CF->>CF: Check: elapsed >= 10-15 min?
        CF-->>SCH: {action: "none", reason: "heating 120s"}
    end

    Note over CF: heating_elapsed >= 600s (10 min)
    SCH->>CF: HTTP trigger
    CF->>CF: Rule: stop heating, start fan stop
    CF->>API: POST ai_defrost_heating=0
    CF->>API: POST ai_defrost_fan_stop=1
    CF->>FS: Save state (FAN_STOP)
    CF->>FS: Log prediction
    Note over FW: Defrost resistor OFF, input fan stopped

    loop Every 60 seconds (fan stop)
        SCH->>CF: HTTP trigger
        CF->>API: GET control_vars
        CF->>FS: Load state
        CF->>CF: Check: filtered_eff > 85%?
        CF-->>SCH: {action: "none", reason: "fan_stop_waiting"}
    end

    Note over CF: filtered_eff > 85%
    SCH->>CF: HTTP trigger
    CF->>CF: Rule: stop fan stop
    CF->>API: POST ai_defrost_fan_stop=0
    CF->>FS: Save state (MEASURING)
    CF->>FS: Log prediction
    Note over FW: Input fan resumed, cycle complete
```

## Defrost Modes

```mermaid
stateDiagram-v2
    [*] --> OFF: defrost_mode=0
    [*] --> ON: defrost_mode=1
    [*] --> AUTO: defrost_mode=2
    [*] --> AI: defrost_mode=3

    state OFF {
        [*] --> HeatingDisabled
    }

    state ON {
        [*] --> HeatingAlwaysOn
    }

    state AUTO {
        [*] --> Measuring_A
        Measuring_A --> Heating_A: filtered_eff < 73%<br>AND raw < filtered
        Heating_A --> FanStop_A: 10-15 min elapsed
        Heating_A --> FanStop_A: eff > 85% (early)
        FanStop_A --> Stopped_A: filtered_eff > 85%
        Stopped_A --> Measuring_A: 10 min cooldown
        note right of AUTO: All logic runs in<br>C firmware locally
    }

    state AI {
        [*] --> Measuring_AI
        Measuring_AI --> Heating_AI: Cloud Function:<br>start_heating
        Heating_AI --> FanStop_AI: Cloud Function:<br>stop_heating_start_fan_stop
        FanStop_AI --> Measuring_AI: Cloud Function:<br>stop_fan_stop
        note right of AI: Same rules as AUTO<br>but runs in cloud.<br>Tunable + logged.
    }
```

## Firestore Collections

```mermaid
erDiagram
    vallox_status {
        timestamp datetime
        control_vars object
        digit_vars object
        ds18b20_vars object
    }

    defrost_cycles {
        timestamp datetime
        cycle_number int
        heating_duration int
        fan_stop_duration int
        total_duration int
        start_outside_temp float
        start_in_eff float
        start_in_eff_filtered float
        end_in_eff float
        end_reason int
        fan_speed int
        humidity float
    }

    ai_defrost_state {
        defrost_state int
        heating_start_time float
        updated datetime
    }

    ai_predictions {
        timestamp datetime
        mode string
        defrost_state int
        sensors object
        prediction object
    }

    defrost_meta {
        cycle_count int
        updated datetime
    }

    vallox_status ||--o{ defrost_cycles : "new cycle detected"
    ai_predictions ||--o{ defrost_cycles : "future ML training"
    ai_defrost_state ||--|| ai_predictions : "state drives predictions"
```

## Port and Protocol Reference

| Port  | Protocol     | From                        | To          | Purpose                   |
| ----- | ------------ | --------------------------- | ----------- | ------------------------- |
| RS485 | DIGIT serial | Pi GPIO                     | Vallox HVAC | Fan speed, temps, control |
| 8056  | UDP JSON     | Node.js API                 | C Firmware  | GET/SET variables         |
| 8057  | UDP JSON     | (secondary)                 | C Firmware  | GET/SET variables         |
| 9000  | HTTP REST    | Dashboard / Cloud Functions | Node.js API | Status + control          |
| 3005  | WebSocket    | Legacy web app              | WS bridge   | Real-time updates         |

## Safety Mechanisms

```mermaid
graph TB
    subgraph "C Firmware Safety (always enforced)"
        S1[30-min max continuous heating<br>Emergency shutoff + mode reset to OFF]
        S2[Fireplace blocks fan stop<br>Rejects ai_defrost_fan_stop when active]
        S3[AI commands timestamped<br>Staleness detection]
    end

    subgraph "Cloud Function Safety"
        S4[Returns action: none on error<br>Never commands heating on failure]
        S5[Heating capped at 15 min max]
        S6[All actions logged to Firestore]
        S7[State persisted in Firestore<br>Survives cold starts]
        S8[Not in AI mode = reset state<br>Switching away cleans up]
    end
```

## AI Evolution Path

```mermaid
graph LR
    subgraph "Phase 1 - Now"
        R[Rule-Based<br>Same as C AUTO mode<br>Cloud Function]
        R --> L[Log all predictions<br>+ sensor context<br>to Firestore]
    end

    subgraph "Phase 2 - Future"
        L --> T[Train ML model<br>on logged data<br>python train_model.py]
        T --> V[Deploy to<br>Vertex AI<br>Endpoint]
        V --> P[Set PREDICTION_MODE<br>=vertex_ai<br>on Cloud Function]
    end

    subgraph "Phase 3 - Mature"
        P --> O[Model learns<br>optimal timing<br>per conditions]
        O --> E[Better efficiency<br>Less energy waste<br>Adaptive thresholds]
    end
```

## Setup Steps

### 1. Deploy the Prediction Cloud Function

```bash
cd cloud-functions-predict/
npm install

# Set your API token
export VALLOX_API_TOKEN=<your-token>
npm run deploy
```

### 2. Set Up Cloud Scheduler

Create a Cloud Scheduler job to trigger the Cloud Function every 60 seconds:

```bash
gcloud scheduler jobs create http predictDefrost-trigger \
    --location=europe-north1 \
    --schedule="* * * * *" \
    --uri="https://europe-north1-pekan-vallox-control.cloudfunctions.net/predictDefrost" \
    --http-method=POST \
    --attempt-deadline=30s \
    --time-zone="Europe/Helsinki"
```

### 3. Enable AI Mode

Via Dashboard: Set defrost mode to "AI" in DefrostModeControl.

Or via API:

```bash
curl "http://<api-ip>:9000/api/vallox/control?token=<token>" \
    -X POST -H "Content-Type: application/json" \
    -d '{"type":"control_vars","variable":"defrost_mode","value":3}'
```

### What Happens When You Click "Defrost Mode AI"?

1.  **Firmware Switch**: The C firmware running on the device switches `defrost_mode` to `3` (AI).
2.  **Passive Control**: The internal automatic defrost logic (AUTO mode) is **disabled**. The firmware stops making its own decisions about when to run the defrost resistors or stop fans.

### What about Vertex Generative AI Studio?

You asked if you can use **Generative AI Studio**.

- **Current Project (Predictive AI)**: This project uses **Predictive AI** (Scikit-Learn). It takes numbers (temperature, efficiency) and predicts numbers (probability of frost). This is the standard industrial approach for control systems.
- **Generative AI (GenAI)**: Vertex AI Studio is for _Generative_ models (like Gemini) that create text, images, or code.

**How could GenAI help here?**
While GenAI won't directly control the fans (it's too slow and hallucination-prone for sub-second safety decisions), it _can_ be used for:

1.  **Explaining Events**: You could feed the logs to Gemini to get a summary: _"The unit defrosted 5 times last night because humidity spiked at 3 AM."_
2.  **Dashboard Chat**: A chatbot in your dashboard to ask _"Is the unit running efficiently?"_
3.  **Code Generation**: Writing these scripts! (As we are doing now).

For the core defrost control logic, **AutoML Tabular** or custom **Predictive Models** (what we are building) are the correct tools.

### How do I know the AI Model is "Ready"?

The system is currently in **Phase 1 (Learning)**. To move to **Phase 2 (Active AI Control)**, look for these signs in the training output:

1.  **Data Volume**: You have at least **50-100 real cycles**. (Currently we are using synthetic data).
2.  **Accuracy**:
    - **Classification**: F1 Score > **0.80**. (Means it reliably predicts success vs failure).
    - **Regression**: RMSE represents the error in seconds. Lower is better (e.g., < 60s error).
3.  **Feature Sense**:
    - Look at the "Top 3 Predictive Features" printed by the script.
    - **Good**: `in_efficiency_filtered`, `outside_temp`, `humidity` (Physical causes).
    - **Bad**: Random noise or irrelevant variables.

Once these criteria are met, we can update the `predictDefrost` cloud function to load the trained model instead of using the hardcoded rules. 3. **Command Listening**: The firmware enters a "listening" state, waiting for explicit commands: - `ai_defrost_heating` (1 = ON, 0 = OFF) - `ai_defrost_fan_stop` (1 = ON, 0 = OFF) 4. **Cloud Control Loop**: - The `predictDefrost` Cloud Function runs every 60 seconds. - It checks the sensor data (efficiency, temperatures). - **If needed**, it sends API requests to toggle the variables above.
_ **Phase 1 (Current)**: It uses the same logic as AUTO mode but runs in the cloud (Rule-Based). This verifies the data pipeline.
_ **Phase 2 (ML)**: Once trained, it will use the Python-trained model to predict optimal defrost cycles. \* **Phase 3 (Physics-Aware)**: The model learns complex relationships, such as the "Dew Point vs Exhaust Temp" rule (if dew point < exhaust temp -> severe icing risk), ensuring deeper understanding than simple threshold logic.

> **Note**: If the Cloud Scheduler or Cloud Function stops running while in AI Mode, the unit will **NOT** defrost automatically. The C firmware has safety timeouts (e.g., max heating duration) but will not _start_ a cycle on its own.

### 4. Monitor

Cloud Function logs:

```bash
gcloud functions logs read predictDefrost --region=europe-north1
```

Firestore: check `ai_predictions` and `ai_defrost/state` collections.

## Configuring Thresholds

Edit `cloud-functions-predict/src/index.ts`:

```typescript
const DEFROST_START_LEVEL = 73; // Start heating below this filtered eff (%)
const DEFROST_TARGET_IN_EFF = 85; // Fan stop ends above this filtered eff (%)
const DEFROST_HEATING_MIN = 10 * 60; // Min heating (seconds)
const DEFROST_HEATING_MAX = 15 * 60; // Max heating (seconds)
```

Redeploy: `cd cloud-functions-predict && npm run deploy`

## File Reference

| File                                                  | Runs on  | Purpose                                          |
| ----------------------------------------------------- | -------- | ------------------------------------------------ |
| `c/main.c`                                            | Pi       | Firmware entry point, 6 threads                  |
| `c/ctrl_logic.c`                                      | Pi       | Defrost state machine (AUTO + AI modes)          |
| `c/digit_protocol.c`                                  | Pi       | RS485 DIGIT communication                        |
| `c/udp-server.c`                                      | Pi       | UDP JSON API (ports 8056/8057)                   |
| `vallox-control-api/src/routes/vallox.ts`             | Cloud VM | REST API endpoints                               |
| `vallox-control-api/src/services/status-publisher.ts` | Cloud VM | Pub/Sub publisher (5 min interval)               |
| `cloud-functions/src/index.ts`                        | GCP      | Pub/Sub processor, Firestore writer              |
| `cloud-functions-predict/src/index.ts`                | GCP      | AI defrost controller (rules + future Vertex AI) |
| `dashboard/`                                          | Hosted   | Next.js 15 dashboard UI                          |
| `python/train_model.py`                               | Dev      | ML training pipeline (Phase 2)                   |
| `python/ai_learner.py`                                | Dev      | Training data prep + model training              |
| `python/ai_config.py`                                 | Dev      | Shared ML config (Phase 2)                       |
