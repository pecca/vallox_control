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

The model uses 9 input features captured at the **start of each defrost cycle**. These are defined in [python/ai_config.py](python/ai_config.py) and stored in Firestore collection `defrost_cycles`.

| # | Feature | Unit | Source | Physical Meaning | Why Used for ML |
|---|---------|------|--------|------------------|-----------------|
| 1 | `start_outside_temp` | C | DIGIT protocol NTC sensor | Outdoor air temperature entering the heat exchanger | Primary driver of ice formation. Colder temperatures increase the temperature differential across the heat exchanger, causing more moisture to condense and freeze. Below -15C, ice buildup accelerates significantly. |
| 2 | `start_exhaust_temp` | C | DIGIT protocol NTC sensor | Temperature of air leaving the building through the heat exchanger | Indicates how much heat energy is available for transfer. Combined with dew point, it determines whether condensation will freeze on exchanger surfaces. |
| 3 | `start_exhaust_humidity` | % RH | DIGIT protocol RH1 sensor | Relative humidity of exhaust (outgoing) air | Direct measure of moisture content in the air passing through the exchanger. Higher humidity means more water vapor available to condense and freeze. Critical during sauna use when indoor humidity spikes to 60-80%+. |
| 4 | `start_supply_temp` | C | DIGIT protocol (incoming_temp) | Temperature of fresh air delivered to the building after passing through the heat exchanger | Reflects current heat exchanger performance. A lower-than-expected supply temp indicates ice is already restricting airflow and heat transfer. |
| 5 | `start_pressure_diff` | Pa | BMP280 sensor (calculated) | Pressure drop across the heat exchanger | Physical indicator of airflow restriction. As ice accumulates on the exchanger surfaces, it blocks airflow passages, increasing the pressure differential. Higher pressure diff = more ice blockage. |
| 6 | `start_fan_speed` | % | DIGIT protocol | Current ventilation fan speed setting | Affects the rate of ice formation: higher fan speed pushes more humid air through the exchanger, increasing condensation rate. Also affects how quickly defrost heating can melt ice (more airflow = more heat removal). |
| 7 | `start_dew_point_delta` | C | Calculated: Exhaust Temp - Dew Point | Temperature margin above the dew point | **Key physics-based feature.** When this value is negative, the exhaust air temperature is below its dew point, meaning moisture is actively condensing. The more negative, the more aggressive the condensation and freezing. This is the strongest predictor of ice formation severity. |
| 8 | `start_in_eff` | % | Calculated: `((Incoming - Outside) / (Inside - Outside)) * 100` | Raw heat recovery efficiency at cycle start | Direct measure of how much heat the exchanger is recovering. When efficiency drops below ~72%, ice is significantly blocking heat transfer. The raw value captures instantaneous performance. |
| 9 | `start_in_eff_filtered` | % | 1-hour moving average of raw efficiency (720 samples at 5s intervals) | Smoothed heat recovery efficiency trend | Filters out short-term noise and fluctuations. The difference between raw and filtered efficiency indicates whether performance is trending down (raw < filtered = actively deteriorating). |

### Training Targets

The model trains on two targets per defrost cycle:

| Target | Type | Description | Why |
|--------|------|-------------|-----|
| **Defrost success** | Binary classification (0/1) | Whether the defrost cycle successfully recovered efficiency. Based on `end_reason` field: reasons 1 (efficiency recovered) and 2 (temperature target reached) count as success. | Predicts whether defrost should be initiated given current conditions. Avoids unnecessary cycles that waste energy. |
| **Heating duration** | Regression (seconds) | How long the infrared heater ran during successful cycles. Only trained on successful cycles. | Predicts optimal heating duration to avoid both under-heating (ice remains) and over-heating (wasted energy). |

### Additional Cycle Data Captured (Context, Not Features)

These fields are stored per cycle for analysis and future model improvements but are not currently used as training features:

| Field | Description | Potential Future Use |
|-------|-------------|---------------------|
| `heating_duration` | Actual heating time in seconds | Regression target |
| `fan_stop_duration` | Time with input fan stopped (seconds) | Could predict optimal fan stop duration |
| `total_duration` | Full cycle duration (seconds) | Efficiency metric for cycle optimization |
| `end_in_eff` | Efficiency at cycle end | Measures cycle effectiveness |
| `end_reason` | How the cycle ended (enum: EffRecovered, TempTarget, Timeout, SafetyShutoff) | Classification target |
| `start_in_eff_filtered` | Filtered efficiency at start | Trend detection |
| `cycle_number` | Sequential cycle counter | Temporal patterns (e.g., degradation over season) |
| DS18B20 temperatures (5 sensors) | Independent temperature readings at cycle start/end | More accurate than DIGIT NTC sensors; future feature candidates |

### Why These Specific Features Were Chosen

**Physics-driven feature selection:** Each feature maps directly to a physical variable that influences ice formation in a counter-flow heat exchanger:

1. **Temperature features** (outside, exhaust, supply): Ice forms when warm humid exhaust air cools below 0C on the exchanger surfaces. The temperature differential between inside and outside air determines the cooling rate and where on the exchanger surfaces ice will form.

2. **Humidity** (exhaust RH): The amount of available moisture determines how much ice can form per unit time. This is why sauna use (very high humidity) causes rapid, heavy ice buildup.

3. **Dew point delta** (calculated): This is the single most important predictor because it directly encodes the thermodynamic condition for condensation. When `exhaust_temp - dew_point < 0`, water vapor is guaranteed to condense. The magnitude tells the model how aggressively ice is forming.

4. **Pressure differential**: Provides a direct physical measurement of ice blockage severity independent of temperature calculations. Acts as ground truth for how blocked the exchanger actually is.

5. **Fan speed**: Modulates the relationship between all other variables. At higher fan speeds, more air volume passes through the exchanger per unit time, changing the dynamics of heat transfer and ice formation.

6. **Efficiency (raw + filtered)**: The primary observable symptom of ice buildup. Including both raw and filtered values gives the model access to both the current state and the trend direction.

### Data Sources

| Source | Collection Method | Update Rate | Storage |
|--------|------------------|-------------|---------|
| **Real sensor data** | C firmware reads hardware sensors, sends via UDP to Node.js API, published to Pub/Sub every 5 min, Cloud Function writes to Firestore | 5 seconds (sensors), 5 minutes (Firestore) | Firestore: `vallox_status` collection |
| **Defrost cycle records** | Cloud Function detects completed cycles by monitoring `defrost_cycle_count` changes between status updates | Per cycle completion | Firestore: `defrost_cycles` collection |
| **Synthetic training data** | [python/simulate_data.py](python/simulate_data.py) generates 100 realistic cycles covering three scenarios | One-time generation | Firestore: `defrost_cycles` collection |
| **AI prediction logs** | Cloud Function logs every prediction with full sensor context | Every 60 seconds (when AI mode active) | Firestore: `ai_predictions` collection |

### Synthetic Data Scenarios

Until enough real cycles are collected (target: 50-100), synthetic data is used. The generator ([python/simulate_data.py](python/simulate_data.py)) simulates three physically realistic scenarios:

| Scenario | Outside Temp | Humidity | Efficiency | Ice Severity | Purpose |
|----------|-------------|----------|------------|-------------|---------|
| **Clean** | -5C to +5C | Low-moderate | 75-90% | None/minimal | Teaches model when defrost is NOT needed |
| **Preemptive** | -10C to -5C | Moderate-high | 65-80% | Early/moderate | Teaches model to detect early signs (dew point delta going negative) |
| **Reactive** | -25C to -10C | High | 40-65% | Severe | Teaches model to handle heavy ice (sauna + extreme cold scenarios) |

### ML Models

Two scikit-learn Gradient Boosting models are trained ([python/ai_learner.py](python/ai_learner.py)):

| Model | Algorithm | Purpose | Readiness Criteria |
|-------|-----------|---------|-------------------|
| **Classifier** | GradientBoostingClassifier (100 trees, depth 5) | Predict if defrost will succeed given current conditions | F1 score > 0.80 on real data |
| **Regressor** | GradientBoostingRegressor (100 trees, depth 5) | Predict optimal heating duration in seconds | RMSE < 60 seconds on real data |

### AI Evolution Phases

| Phase | Status | Data Source | Control Method |
|-------|--------|-------------|---------------|
| **Phase 1** | Active | Synthetic + early real cycles | Rule-based (same thresholds as AUTO mode, runs in cloud) |
| **Phase 2** | Ready when criteria met | 50-100+ real cycles | Trained ML models on Vertex AI |
| **Phase 3** | Future | Accumulated real-world data | Physics-aware model with adaptive thresholds |

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
