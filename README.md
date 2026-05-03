# Vallox Control — IoT Home Automation with AI-Powered Defrost

> Full-stack IoT system for monitoring and controlling a **Vallox 150 SE MLV** heat-recovery ventilation unit.  
> Combines **embedded C firmware**, a **Node.js gateway API**, **GCP Cloud Functions**, a **Next.js dashboard**, and a **Python ML pipeline** — all working together to replace the manufacturer's defrost logic with an AI-driven alternative.

---

## Why This Project Exists

Finnish winters routinely drop below −20 °C. At these temperatures, moisture in the exhaust air freezes inside the counter-flow heat exchanger, blocking airflow and destroying efficiency. The factory defrost algorithm is crude — fixed timers and simple thresholds — wasting energy and sometimes failing entirely during extreme cold or sauna use.

This project replaces that logic with a physics-aware, data-driven system that:
- **Monitors** the unit in real-time via custom sensors (DS18B20, BMP280) and the proprietary DIGIT protocol
- **Controls** defrost heating and fan-stop cycles through a multi-phase state machine
- **Learns** optimal defrost timing from accumulated cycle data using Gradient Boosting models on Vertex AI
- **Visualizes** everything through a responsive Next.js dashboard deployed on Cloud Run

---

## Tech Stack

| Layer | Technology | Purpose |
|---|---|---|
| **Embedded Firmware** | C (POSIX pthreads, bcm2835) | 6-thread real-time control on Raspberry Pi |
| **Hardware Sensors** | DS18B20 (1-Wire), BMP280 (I²C), RS485 | Temperature, pressure, humidity, DIGIT protocol |
| **Gateway API** | Node.js / Express / TypeScript | REST API + UDP bridge + GCP Pub/Sub publisher |
| **Cloud Functions** | TypeScript (GCP Gen2) | Event-driven status logging & AI defrost controller |
| **ML Pipeline** | Python / scikit-learn / Vertex AI | Gradient Boosting defrost prediction models |
| **Dashboard** | Next.js 16 / React 19 / MUI 7 / Recharts | Real-time monitoring UI on Cloud Run |
| **Infrastructure** | GCP (Pub/Sub, Firestore, Cloud Run, Vertex AI) | Serverless event processing & model serving |
| **Data Validation** | Zod 4 | Runtime schema validation across API boundaries |
| **Auth** | NextAuth.js + bcrypt | Dashboard access control |

---

## System Architecture

```mermaid
graph TB
    subgraph "Raspberry Pi (On-Premise)"
        VALLOX["Vallox 150 SE MLV<br/>Heat Exchanger"]
        FW["C Firmware<br/><i>6 pthreads</i>"]
        DS["DS18B20 Sensors<br/><i>1-Wire × 4</i>"]
        BMP["BMP280<br/><i>I²C Pressure</i>"]
        RESISTOR["Heat Resistor<br/><i>Defrost heater<br/>inside heat exchanger</i>"]
        API["Node.js API<br/><i>Express :9000</i>"]
        PRED["RPi Predictor<br/><i>60s polling loop</i>"]

        VALLOX <-->|"RS485 / DIGIT"| FW
        DS -->|"1-Wire"| FW
        BMP -->|"I²C"| FW
        FW -->|"GPIO Relay"| RESISTOR
        RESISTOR ---|"Melts ice"| VALLOX
        FW <-->|"UDP :8056<br/>JSON"| API
        PRED -->|"HTTP"| API
    end

    subgraph "Google Cloud Platform"
        PUBSUB["Pub/Sub<br/><i>vallox-status topic</i>"]
        CF_STATUS["Cloud Function<br/><i>processValloxStatus</i>"]
        CF_PREDICT["Cloud Function<br/><i>predictDefrost</i>"]
        FS["Firestore"]
        VERTEX["Vertex AI<br/><i>Endpoint</i>"]
        CR["Cloud Run<br/><i>Dashboard</i>"]
    end

    API -->|"Publish every 5 min"| PUBSUB
    PUBSUB -->|"Trigger"| CF_STATUS
    CF_STATUS -->|"Write"| FS
    PRED -->|"HTTP POST"| CF_PREDICT
    CF_PREDICT -->|"Predict"| VERTEX
    CF_PREDICT -->|"Log"| FS
    CF_PREDICT -->|"HTTP"| API
    CR -->|"Read"| FS
```

---

## Data Flow — Defrost Cycle

The complete lifecycle of an AI-controlled defrost cycle, from sensor reading to actuator command:

```mermaid
sequenceDiagram
    participant FW as C Firmware
    participant API as Node.js API
    participant PS as Pub/Sub
    participant CF1 as processValloxStatus
    participant FS as Firestore
    participant PRED as RPi Predictor
    participant CF2 as predictDefrost
    participant VAI as Vertex AI

    Note over FW: Every 5s: read sensors,<br/>calculate efficiency,<br/>update state machine

    API->>PS: Publish status (every 5 min)
    PS->>CF1: Trigger
    CF1->>FS: Write vallox_status
    CF1->>FS: Store defrost_cycle (on cycle end)

    loop Every 60 seconds
        PRED->>API: GET /api/vallox/status
        PRED->>CF2: POST {sensor_data, defrost_state}
        
        alt Rule-Based Mode
            CF2->>CF2: Evaluate thresholds
        else Vertex AI Mode
            CF2->>VAI: Predict(features)
            VAI->>CF2: {defrost_needed, confidence}
        end
        
        CF2->>FS: Log prediction
        CF2->>PRED: {action: start_defrost}
        PRED->>API: POST /api/vallox/control
        API->>FW: UDP SET ai_defrost_heating=1
    end
```

---

## Embedded Firmware (C)

The firmware runs on a Raspberry Pi as a multi-threaded POSIX application:

### Threading Model

```mermaid
graph LR
    subgraph "6 Concurrent pthreads"
        T1["DS18B20 Thread<br/><i>1-Wire temperature polling</i>"]
        T2["DIGIT RX Thread<br/><i>RS485 reception</i>"]
        T3["DIGIT Update Thread<br/><i>Protocol processing (5s)</i>"]
        T4["UDP Server :8056<br/><i>JSON command interface</i>"]
        T5["UDP Server :8057<br/><i>Secondary listener</i>"]
        T6["Control Logic Thread<br/><i>Defrost state machine</i>"]
    end

    T1 -->|"Shared memory<br/>(mutex)"| T6
    T3 -->|"DIGIT vars<br/>(mutex)"| T6
    T4 -->|"Commands"| T6
    T6 -->|"Relay GPIO"| HW["Hardware"]
```

### Defrost State Machine

The firmware implements a 4-state defrost controller with improvement-based stopping logic:

```mermaid
stateDiagram-v2
    [*] --> Measuring

    Measuring --> Heating : efficiency < start_level<br/>OR AI command
    
    Heating --> InputFanStop : exhaust > 3°C AND<br/>(target reached OR<br/>efficiency plateaued 3min)
    Heating --> Measuring : safety timeout (30 min)
    
    InputFanStop --> Stopped : efficiency recovered<br/>OR plateaued (5 min)<br/>OR max duration (45 min)
    
    Stopped --> Measuring : cooldown complete (30 min)

    note right of Heating
        Resistor ON
        30s efficiency sampling
        Plateau = no 0.5% improvement
    end note
    
    note right of InputFanStop
        Input fan OFF
        Lets residual heat melt ice
        One-phase optimization
    end note
    
    note right of Stopped
        System cooldown
        Prevents rapid re-triggering
    end note
```

### Key Design Decisions

- **Safety-first**: Local 30-minute heating timeout operates independently of cloud — the device never depends on network connectivity for safety
- **Improvement-based stopping**: Instead of fixed thresholds, the firmware samples efficiency every 30 seconds and detects plateaus (< 0.5% improvement over configurable windows)
- **Dual sensor validation**: Both DIGIT protocol NTC and DS18B20 sensors must agree before state transitions
- **Persistent cycle counter**: Stored to filesystem, survives reboots

### Source Structure

```
c/
├── main.c                    # Entry point, pthread creation
├── ctrl_logic.c/h            # Defrost state machine (1258 lines)
├── digit_protocol.c/h        # Vallox DIGIT protocol (RS485)
├── rs485.c/h                 # Serial communication layer
├── DS18B20.c/h               # 1-Wire temperature sensors
├── defrost_resistor.c/h      # Heating element control
├── pre_heating_resistor.c/h  # Pre-heating management
├── post_heating_counter.c/h  # Post-heating logic
├── relay_control.c/h         # GPIO relay switching (bcm2835)
├── udp-server.c/h            # UDP JSON command interface
├── json_codecs.c/h           # JSON encoding/decoding
├── jsmn.c/h                  # Minimal JSON parser (tokenizer)
├── temperature_conversion.c/h # NTC thermistor conversion
└── Makefile                  # Builds with -lpthread -lbcm2835
```

---

## Node.js Gateway API

TypeScript Express server bridging the firmware with GCP cloud services.

### Responsibilities
- **UDP ↔ REST bridge**: Translates HTTP requests to UDP JSON messages for the firmware
- **Pub/Sub publisher**: Aggregates sensor data and publishes to GCP every 5 minutes
- **Control endpoint**: Accepts commands from the RPi Predictor and forwards to firmware

### API Endpoints

```
GET  /api/vallox/status?type={digit_vars|control_vars|ds18b20_vars}
POST /api/vallox/control  { type, variable, value }
```

### Key Files

```
vallox-control-api/
├── src/
│   ├── index.ts              # Express server + publisher init
│   ├── config.ts             # Environment config (PORT, UDP, Pub/Sub)
│   ├── routes/               # REST route handlers
│   ├── services/
│   │   ├── pubsub.ts         # GCP Pub/Sub client
│   │   └── status-publisher.ts  # 5-min aggregation loop
│   └── vallox/               # UDP communication layer
└── tsconfig.json
```

---

## Cloud Functions (GCP Gen2)

### `processValloxStatus` — Event-Driven Data Pipeline

Triggered by Pub/Sub messages. Performs two tasks:
1. **Status archival** → Writes sensor snapshots to `vallox_status` collection
2. **Cycle detection** → Compares `defrost_cycle_count` against stored metadata to detect completed cycles and archives full cycle data to `defrost_cycles` collection

### `predictDefrost` — AI Defrost Controller

HTTP-triggered function called every 60 seconds by the RPi Predictor. Supports two modes:

| Mode | Config | Description |
|---|---|---|
| **Rule-Based** | `PREDICTION_MODE=rules` | Physics-based thresholds matching firmware logic |
| **Vertex AI** | `PREDICTION_MODE=vertex_ai` | ML model inference via Vertex AI endpoint |

**Safety guardrails** are enforced regardless of prediction mode:
- Minimum 30-minute cooldown between cycles
- Efficiency floor checks before triggering
- Fireplace mode override (prevents defrost during fireplace operation)

---

## ML Pipeline (Python)

Gradient Boosting models trained on physics-based features from real defrost cycles:

### Feature Engineering

```mermaid
graph LR
    subgraph "Raw Sensor Data"
        S1["outside_temp"]
        S2["ds_exhaust_temp"]
        S3["humidity (RH)"]
        S4["incoming_temp"]
        S5["fan_speed"]
    end

    subgraph "Calculated Features"
        C1["dew_point<br/><i>Magnus formula</i>"]
        C2["dew_point_delta<br/><i>exhaust − dew_point</i>"]
        C3["efficiency<br/><i>(incoming − outside) /<br/>(inside − outside)</i>"]
    end

    S2 --> C2
    S3 --> C1
    C1 --> C2

    subgraph "Model Input (8 features)"
        F["start_outside_temp<br/>start_ds_exhaust_temp<br/>start_humidity<br/>start_incoming_temp<br/>start_fan_speed<br/>start_dew_point_delta<br/>start_ds_outside_temp<br/>start_ds_incoming_temp"]
    end
```

The **dew point delta** is the single most important predictor — when `exhaust_temp − dew_point < 0`, water vapor is guaranteed to condense on heat exchanger surfaces.

### Models

| Model | Algorithm | Target | Readiness |
|---|---|---|---|
| **Classifier** | GradientBoostingClassifier (100 trees, depth 5) | Defrost success (binary) | F1 > 0.80 on real data |
| **Regressor** | GradientBoostingRegressor (100 trees, depth 5) | Optimal duration (seconds) | RMSE < 60s on real data |

### Pipeline Files

```
python/
├── ai_learner.py       # Training pipeline → Vertex AI deployment
├── train_model.py      # CLI training entry point
├── simulate_data.py    # Synthetic data generator (3 scenarios)
├── ai_config.py        # Feature definitions, GCP config
├── BMP280.py           # Pressure sensor interface
└── requirements.txt    # scikit-learn, google-cloud-aiplatform
```

### AI Evolution

```mermaid
graph LR
    P1["Phase 1<br/><b>Rule-Based</b><br/><i>Active</i>"]
    P2["Phase 2<br/><b>Vertex AI</b><br/><i>Ready</i>"]
    P3["Phase 3<br/><b>Adaptive</b><br/><i>Future</i>"]

    P1 -->|"50–100 real cycles<br/>F1 > 0.80"| P2
    P2 -->|"Accumulated data<br/>physics-aware model"| P3

    style P1 fill:#4caf50,color:#fff
    style P2 fill:#ff9800,color:#fff
    style P3 fill:#9e9e9e,color:#fff
```

---

## Dashboard (Next.js 16)

Real-time monitoring UI deployed on **Cloud Run**.

### Tech
- **Next.js 16** with App Router and Server Components
- **React 19** + **MUI 7** component library
- **Recharts** for temperature/efficiency charts
- **Zod 4** for runtime data validation
- **NextAuth.js** for session management

### Components

| Component | Purpose |
|---|---|
| `ControlPanel` | Main dashboard with live sensor readings |
| `DefrostStatus` | Current defrost state, AI metrics, cycle history |
| `DefrostConfigPanel` | Tunable parameters (thresholds, durations) |
| `TemperatureChart` | Real-time temperature trend visualization |
| `EfficiencyStats` | Heat recovery efficiency monitoring |
| `FireplaceControl` | Fireplace mode toggle with timer |
| `FanControl` | Fan speed adjustment |
| `AiSettings` | AI mode configuration |

### Data Flow

```
Firestore (vallox_status, defrost_cycles)
    ↓ Server Components (direct Firestore reads)
Dashboard UI
    ↓ Server Actions
Node.js API → C Firmware (control commands)
```

---

## Firestore Schema

```mermaid
erDiagram
    vallox_status {
        timestamp created_at
        object digit_vars
        object control_vars
        object ds18b20_vars
    }
    
    defrost_cycles {
        int cycle_number
        timestamp timestamp
        int total_duration
        int heating_duration
        int fan_stop_duration
        float start_outside_temp
        float start_ds_exhaust_temp
        float start_humidity
        float start_dew_point_delta
        float end_in_eff
        int end_reason
    }
    
    ai_predictions {
        timestamp timestamp
        boolean defrost_needed
        string reason
        float confidence
        object sensor_snapshot
    }
    
    ai_defrost_state {
        int defrost_state
        int heating_start_time
        timestamp updated
    }

    vallox_status ||--o{ defrost_cycles : "cycle detection"
    defrost_cycles ||--o{ ai_predictions : "training data"
```

---

## Deployment

### Infrastructure

| Service | Platform | Trigger |
|---|---|---|
| C Firmware | Raspberry Pi (systemd) | Boot |
| Node.js API | Raspberry Pi (pm2) | Boot |
| RPi Predictor | Raspberry Pi (pm2) | Boot |
| processValloxStatus | Cloud Functions Gen2 | Pub/Sub |
| predictDefrost | Cloud Functions Gen2 | HTTP |
| Dashboard | Cloud Run | HTTPS |
| ML Models | Vertex AI Endpoint | HTTP |

### Quick Start

```bash
# Build C firmware
cd c/ && make

# Run Node.js API
cd vallox-control-api/ && npm install && npm start

# Train ML model
cd python/
pip install -r requirements.txt
python simulate_data.py          # Generate synthetic data
python train_model.py --deploy   # Train & deploy to Vertex AI

# Deploy cloud functions
./deploy_predict.sh              # AI defrost controller
./deploy_training.sh             # Model training function

# Run dashboard locally
cd dashboard/ && npm install && npm run dev
```

### GCP Configuration

```
Project:  pekan-vallox-control
Region:   europe-north1
```

| Environment Variable | Purpose |
|---|---|
| `PREDICTION_MODE` | `rules` or `vertex_ai` |
| `VERTEX_ENDPOINT_ID` | Vertex AI model endpoint |
| `VALLOX_API_URL` | RPi API address |
| `VALLOX_API_TOKEN` | API authentication |

---

## Repository Structure

```
vallox_control/
├── c/                          # Embedded C firmware (Raspberry Pi)
├── vallox-control-api/         # Node.js Express gateway API
├── cloud-functions/            # GCP: status logging (Pub/Sub trigger)
├── cloud-functions-predict/    # GCP: AI defrost controller (HTTP)
├── cloud-functions-training/   # GCP: model training pipeline
├── python/                     # ML training scripts & data simulator
├── rpi-predictor/              # On-device AI orchestrator (60s loop)
├── dashboard/                  # Next.js 16 monitoring UI
├── BMP280_driver/              # Bosch pressure sensor driver
├── deploy_predict.sh           # Cloud Function deployment script
├── deploy_training.sh          # Training function deployment script
├── DEFROST_IMPROVEMENT_PLAN.md # Detailed firmware improvement spec
└── CLAUDE.md                   # Developer reference & architecture
```

---

## License

Private project — not open source.
