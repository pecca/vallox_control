# Vallox Control - IoT Home Automation for Vallox 150 SE MLV

Monitoring and control system for a Vallox 150 SE MLV ventilation unit with AI-powered defrost optimization.

## System Components

- **C firmware** (`c/`) - Raspberry Pi application interfacing with Vallox via RS485/DIGIT protocol
- **Python ML pipeline** (`python/`) - Machine learning training for defrost prediction
- **Node.js API** (`vallox-control-api/`) - Express REST API
- **Cloud Functions** - GCP functions for data collection and AI defrost control
- **Dashboard** (`dashboard/`) - Next.js 15 monitoring UI

---

## AI/ML Training Data

The ML system predicts optimal defrost cycles for the heat exchanger. Below is every piece of data used for training, what it represents physically, and why it matters for the model.

### Training Features

The model uses input features captured at the **start of each defrost cycle**. These are stored in Firestore collection `defrost_cycles`.

| #   | Feature                  | Unit | Source                               | Physical Meaning                                    | Why Used for ML                                                           |
| --- | ------------------------ | ---- | ------------------------------------ | --------------------------------------------------- | ------------------------------------------------------------------------- |
| 1   | `start_outside_temp`     | C    | DIGIT protocol NTC sensor            | Outdoor air temperature entering the heat exchanger | Primary driver of ice formation.                                          |
| 2   | `start_ds_exhaust_temp`  | C    | DS18B20 sensor                       | Temperature of air leaving the building             | More accurate than NTC. Indicates available heat energy.                  |
| 3   | `start_humidity`         | % RH | DIGIT protocol RH1 sensor            | Relative humidity of exhaust air                    | Direct measure of moisture content. Higher humidity = more ice potential. |
| 4   | `start_incoming_temp`    | C    | DIGIT protocol                       | Temperature of fresh air delivered to building      | Reflects heat exchanger performance (lower = blocked).                    |
| 5   | `start_fan_speed`        | %    | DIGIT protocol                       | Current ventilation fan speed setting               | Affects condensation rate and heat removal.                               |
| 6   | `start_dew_point_delta`  | C    | Calculated: Exhaust (DS) - Dew Point | Temperature margin above the dew point              | **Key physics-based feature.** Negative value means active condensation.  |
| 7   | `start_ds_outside_temp`  | C    | DS18B20 sensor                       | Outdoor air temperature                             | Independent, higher precision measurement.                                |
| 8   | `start_ds_incoming_temp` | C    | DS18B20 sensor                       | Supply air temperature                              | Independent, higher precision measurement.                                |

### Training Targets

The model trains on targets per defrost cycle:

| Target              | Type           | Description                                     | Why                            |
| ------------------- | -------------- | ----------------------------------------------- | ------------------------------ |
| **Defrost success** | Binary (0/1)   | Based on `end_reason` (Recovered/TargetReached) | Predicts if defrost is needed. |
| **Total duration**  | Regression (s) | Full cycle duration                             | Predicts optimal defrost time. |

### Additional Cycle Data Captured

| Field                 | Description                                                                  |
| --------------------- | ---------------------------------------------------------------------------- |
| `total_duration`      | Full cycle duration (seconds)                                                |
| `end_in_eff`          | Efficiency at cycle end                                                      |
| `end_reason`          | How the cycle ended (enum: EffRecovered, TempTarget, Timeout, SafetyShutoff) |
| `cycle_number`        | Sequential cycle counter                                                     |
| `start_incoming_temp` | NTC sensor reading for supply air                                            |
| `start_dew_point`     | Calculated dew point at start                                                |

### Why These Specific Features Were Chosen

**Physics-driven feature selection:** Each feature maps directly to a physical variable that influences ice formation in a counter-flow heat exchanger:

1. **Temperature features** (outside, exhaust, supply): Ice forms when warm humid exhaust air cools below 0C on the exchanger surfaces. The temperature differential between inside and outside air determines the cooling rate and where on the exchanger surfaces ice will form.

2. **Humidity** (exhaust RH): The amount of available moisture determines how much ice can form per unit time. This is why sauna use (very high humidity) causes rapid, heavy ice buildup.

3. **Dew point delta** (calculated): This is the single most important predictor because it directly encodes the thermodynamic condition for condensation. When `exhaust_temp - dew_point < 0`, water vapor is guaranteed to condense. The magnitude tells the model how aggressively ice is forming.

4. **Pressure differential**: Provides a direct physical measurement of ice blockage severity independent of temperature calculations. Acts as ground truth for how blocked the exchanger actually is.

5. **Fan speed**: Modulates the relationship between all other variables. At higher fan speeds, more air volume passes through the exchanger per unit time, changing the dynamics of heat transfer and ice formation.

6. **Efficiency (raw + filtered)**: The primary observable symptom of ice buildup. Including both raw and filtered values gives the model access to both the current state and the trend direction.

### Data Sources

| Source                      | Collection Method                                                                                                                     | Update Rate                                | Storage                                |
| --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------ | -------------------------------------- |
| **Real sensor data**        | C firmware reads hardware sensors, sends via UDP to Node.js API, published to Pub/Sub every 5 min, Cloud Function writes to Firestore | 5 seconds (sensors), 5 minutes (Firestore) | Firestore: `vallox_status` collection  |
| **Defrost cycle records**   | Cloud Function detects completed cycles by monitoring `defrost_cycle_count` changes between status updates                            | Per cycle completion                       | Firestore: `defrost_cycles` collection |
| **Synthetic training data** | [python/simulate_data.py](python/simulate_data.py) generates 100 realistic cycles covering three scenarios                            | One-time generation                        | Firestore: `defrost_cycles` collection |
| **AI prediction logs**      | Cloud Function logs every prediction with full sensor context                                                                         | Every 60 seconds (when AI mode active)     | Firestore: `ai_predictions` collection |

### Synthetic Data Scenarios

Until enough real cycles are collected (target: 50-100), synthetic data is used. The generator ([python/simulate_data.py](python/simulate_data.py)) simulates three physically realistic scenarios:

| Scenario       | Outside Temp | Humidity      | Efficiency | Ice Severity   | Purpose                                                              |
| -------------- | ------------ | ------------- | ---------- | -------------- | -------------------------------------------------------------------- |
| **Clean**      | -5C to +5C   | Low-moderate  | 75-90%     | None/minimal   | Teaches model when defrost is NOT needed                             |
| **Preemptive** | -10C to -5C  | Moderate-high | 65-80%     | Early/moderate | Teaches model to detect early signs (dew point delta going negative) |
| **Reactive**   | -25C to -10C | High          | 40-65%     | Severe         | Teaches model to handle heavy ice (sauna + extreme cold scenarios)   |

### ML Models

Two scikit-learn Gradient Boosting models are trained ([python/ai_learner.py](python/ai_learner.py)):

| Model          | Algorithm                                       | Purpose                                                  | Readiness Criteria             |
| -------------- | ----------------------------------------------- | -------------------------------------------------------- | ------------------------------ |
| **Classifier** | GradientBoostingClassifier (100 trees, depth 5) | Predict if defrost will succeed given current conditions | F1 score > 0.80 on real data   |
| **Regressor**  | GradientBoostingRegressor (100 trees, depth 5)  | Predict optimal heating duration in seconds              | RMSE < 60 seconds on real data |

### AI Evolution Phases

| Phase       | Status                  | Data Source                   | Control Method                                           |
| ----------- | ----------------------- | ----------------------------- | -------------------------------------------------------- |
| **Phase 1** | Active                  | Synthetic + early real cycles | Rule-based (same thresholds as AUTO mode, runs in cloud) |
| **Phase 2** | Ready when criteria met | 50-100+ real cycles           | Trained ML models on Vertex AI                           |
| **Phase 3** | Future                  | Accumulated real-world data   | Physics-aware model with adaptive thresholds             |

---

## Quick Start

See [python/AI_README.md](python/AI_README.md) for full architecture diagrams and deployment instructions.

### Build C Firmware

```bash
cd c/ && make
```

### Run Node.js API

```bash
cd vallox-control-api/ && npm install && npm start
```

### Train ML Model

```bash
cd python/
pip install -r requirements.txt
python simulate_data.py          # Generate synthetic training data
python train_model.py            # Train models
python train_model.py --deploy   # Train and deploy to Vertex AI
```

### Deploy Cloud Functions

```bash
./deploy_predict.sh    # AI defrost controller
./deploy_training.sh   # Model training function
```
