# AI APPLICATION SPECIFICATION

# DrainGuard AI

## Trustworthy AI-Powered IoT Drainage Intelligence Platform

> **IMPORTANT:** This document is the primary specification for the AI application.
>
> Read this entire document before modifying or creating code.
>
> The existing ESP32 + Blynk + physical drainage prototype is already working and tested. Do NOT unnecessarily rewrite, replace, or break the existing IoT system.
>
> The objective is NOT to build another generic "IoT dashboard + AI blockage prediction" project.
>
> The objective is to build a **trustworthy, explainable, fault-aware drainage intelligence platform** that combines real-time sensing, machine learning, sensor-health analysis, uncertainty-aware decisions, predictive analytics, maintenance prioritization, and a prototype-scale digital twin.

---

# 1. PROJECT IDENTITY

## Project Name

**DrainGuard AI**

## Full Name

**Trustworthy AI-Powered IoT Drainage Intelligence Platform**

## One-Line Definition

> DrainGuard AI is a low-cost intelligent drainage monitoring platform that combines multi-sensor IoT data, machine learning, sensor-fault awareness, explainable predictions, drainage health scoring, and digital-twin-based decision support to detect and predict drainage problems before they become critical.

---

# 2. THE REAL PROBLEM

Urban drainage systems can experience:

* Plastic accumulation
* Leaves
* Mud
* Sediment
* Solid waste
* Partial pipe obstruction
* Severe blockage
* Rapid water accumulation
* Sensor failures
* Noisy measurements
* Communication failures

A simple threshold system can detect abnormal flow.

A simple AI model can predict blockage.

However, real drainage environments introduce an important complication:

> **An abnormal sensor reading does not necessarily mean a physical blockage.**

For example:

```text
Flow Sensor 1 = HIGH
Flow Sensor 2 = LOW
Water Level = NORMAL
```

Possible explanations include:

```text
1. Actual blockage
2. Flow Sensor 2 malfunction
3. Temporary turbulence
4. Sensor noise
5. Communication/data error
```

Another example:

```text
Rainfall / inflow increases
Flow increases
Water level increases
Flow difference remains normal
```

This may represent heavy inflow rather than blockage.

Therefore, DrainGuard AI must reason across multiple signals instead of blindly converting a single threshold into an alarm.

---

# 3. RESEARCH-DRIVEN DIFFERENTIATION

The project must NOT claim:

> "Nobody has used AI for drainage blockage detection."

That would be inaccurate.

Existing commercial and research systems already use:

* Machine learning
* Sewer-level monitoring
* Rainfall information
* Anomaly detection
* Predictive blockage alerts
* Explainable ML
* Digital twins
* Sensor networks
* Predictive maintenance

The project's differentiation should instead be:

> **A low-cost, physically validated prototype that integrates multi-sensor blockage reasoning, sensor-fault awareness, uncertainty/confidence estimation, explainable AI, drainage health assessment, maintenance prioritization, and a prototype-scale digital twin in one system.**

The application should therefore be designed around these capabilities.

---

# 4. CORE DESIGN PHILOSOPHY

DrainGuard AI should answer five questions:

### Question 1

**What is happening right now?**

### Question 2

**Is this actually a blockage or could the sensors/environment explain it?**

### Question 3

**How confident is the system?**

### Question 4

**What is likely to happen next?**

### Question 5

**What should a maintenance operator do?**

The dashboard should therefore progress from:

```text
OBSERVE
   ↓
DIAGNOSE
   ↓
PREDICT
   ↓
EXPLAIN
   ↓
PRIORITIZE
   ↓
ACT
```

---

# 5. EXISTING SYSTEM — DO NOT BREAK

The physical IoT system is already operational.

Current components:

* ESP32 DevKit V1
* AJ-SR04M waterproof ultrasonic sensor
* ZJ-S201 Flow Sensor 1
* ZJ-S201 Flow Sensor 2
* 1-channel relay
* AC submersible cooler pump
* Active buzzer
* Blue LED
* Green LED
* Yellow LED
* Red LED
* Blynk IoT dashboard

The AI application must be built around this system.

---

# 6. EXISTING HARDWARE PIN CONFIGURATION

| Component       |    GPIO |
| --------------- | ------: |
| Flow Sensor 1   | GPIO 32 |
| Flow Sensor 2   | GPIO 33 |
| Ultrasonic TRIG | GPIO 27 |
| Ultrasonic ECHO | GPIO 26 |
| Blue LED        | GPIO 25 |
| Green LED       | GPIO 19 |
| Yellow LED      | GPIO 21 |
| Red LED         | GPIO 18 |
| Buzzer          | GPIO 22 |
| Relay           | GPIO 23 |

Do not change these assignments unless absolutely necessary.

---

# 7. EXISTING BLOCKAGE DETECTION

Current variables:

```text
Flow 1 = upstream flow
Flow 2 = downstream flow
Flow Difference = Flow 1 - Flow 2
```

Current parameters:

```text
BLOCKAGE_DIFFERENCE = 15
BLOCKAGE_CONFIRM_TIME = 3 seconds
```

Current deterministic rule:

```text
Flow 1 >= 20
AND
Flow 1 - Flow 2 >= 15
FOR
3 consecutive seconds
```

Then:

```text
blockageDetected = true
```

Otherwise:

```text
blockageDetected = false
```

This rule-based system is already part of the embedded safety mechanism.

---

# 8. RULE ENGINE VS AI ENGINE

This distinction must be clearly represented in the application.

## Existing Rule Engine

```text
Flow difference threshold
+
3-second confirmation
```

Purpose:

**Immediate deterministic embedded detection**

---

## AI Engine

```text
Multiple sensor signals
+
Temporal behaviour
+
Historical patterns
+
Sensor health
+
Environmental context
+
Machine learning
```

Purpose:

**Prediction, diagnosis and decision support**

---

# 9. CRITICAL SAFETY PRINCIPLE

AI must NOT replace the existing embedded safety system.

Architecture:

```text
                    ESP32
                      │
             ┌────────┴────────┐
             │                 │
       SAFETY LOGIC        TELEMETRY
             │                 │
             ▼                 ▼
       Pump Protection     AI Platform
                               │
                               ▼
                         Prediction
                         Explanation
                         Recommendation
```

AI failure must never disable:

* Water-level protection
* Relay safety
* Pump shutdown
* Existing blockage detection
* Local alarms

The AI layer is an intelligence layer, not the primary safety controller.

---

# 10. CURRENT WATER-LEVEL SYSTEM

Ultrasonic calibration:

```text
EMPTY_DISTANCE = 27.75 cm
FULL_DISTANCE  = 20.49 cm
```

Water level:

```text
0%   = empty
100% = critical/full
```

Current warning levels:

| Level   | Behaviour              |
| ------- | ---------------------- |
| 0–30%   | Green                  |
| 30–60%  | Yellow + slow warning  |
| 60–80%  | Red + faster warning   |
| 80–90%  | Rapid red warning      |
| 90–<99% | All warning LEDs blink |
| 99–100% | Critical warning       |

The system maintains a cumulative full-level count.

After the configured critical confirmation count:

```text
Pump OFF
Relay OFF
Blue LED OFF
Warning LEDs OFF
Buzzer OFF
Shutdown latched
```

This existing functionality must remain unchanged.

---

# 11. BLYNK DATASTREAMS

Existing telemetry:

| Virtual Pin | Name               | Type    |
| ----------- | ------------------ | ------- |
| V0          | Flow 1             | Double  |
| V1          | Flow 2             | Double  |
| V2          | Water Level        | Double  |
| V3          | Flow Difference    | Integer |
| V4          | Blockage Status    | String  |
| V5          | Pump Status        | String  |
| V6          | Full_Level Count   | Integer |
| V7          | Water Level Status | String  |
| V8          | Blue LED           | Integer |
| V9          | Green LED          | Integer |
| V10         | Yellow LED         | Integer |
| V11         | Red LED            | Integer |

These are the initial IoT inputs.

---

# 12. NEW CORE ARCHITECTURE

The target architecture is:

```text
                  PHYSICAL DRAINAGE SYSTEM
                           │
             ┌─────────────┼─────────────┐
             │             │             │
          FLOW 1        FLOW 2       WATER LEVEL
             │             │             │
             └─────────────┼─────────────┘
                           │
                          ESP32
                           │
                       BLYNK / IoT
                           │
                    DATA INGESTION
                           │
                  ┌────────┴────────┐
                  │                 │
           SENSOR HEALTH      FEATURE ENGINE
                  │                 │
                  └────────┬────────┘
                           │
                     AI FUSION ENGINE
                           │
            ┌──────────────┼──────────────┐
            │              │              │
       BLOCKAGE RISK   SENSOR FAULT   WATER/FLOOD
            │              │              │
            └──────────────┼──────────────┘
                           │
                    CONFIDENCE SCORE
                           │
                  SEVERITY ESTIMATION
                           │
                 DRAIN HEALTH INDEX
                           │
                MAINTENANCE PRIORITY
                           │
                    DIGITAL TWIN
                           │
                    WEB APPLICATION
```

---

# 13. PRIMARY AI OUTPUT

The system should not produce only:

```text
BLOCKAGE = YES
```

Instead produce a structured diagnosis:

```text
Overall Status
Blockage Risk
Sensor Health
Confidence
Severity
Recommended Action
```

Example:

```text
HIGH BLOCKAGE RISK

Blockage Probability:
84%

Confidence:
91%

Sensor Reliability:
96%

Severity:
HIGH

Recommended Action:
Inspect downstream section
```

---

# 14. MULTI-CONDITION DIAGNOSIS

The AI should eventually distinguish between:

```text
NORMAL
DEVELOPING BLOCKAGE
BLOCKAGE
CRITICAL WATER ACCUMULATION
SENSOR FAULT
UNCERTAIN
```

Do not force every abnormal condition into "BLOCKAGE".

---

# 15. SENSOR HEALTH INTELLIGENCE

This is one of the major differentiating features.

The system should assess whether its own sensor data is trustworthy.

Potential sensor states:

```text
HEALTHY
WARNING
SUSPECTED FAULT
OFFLINE
UNCERTAIN
```

For example:

```text
Flow Sensor 1
HEALTHY

Flow Sensor 2
SUSPECTED FAULT

Ultrasonic Sensor
HEALTHY
```

---

# 16. CROSS-SENSOR CONSISTENCY

The system should use physical relationships between sensors.

Example:

```text
Flow 1 = 50
Flow 2 = 48
Water Level = stable
```

This is likely coherent.

But:

```text
Flow 1 = 50
Flow 2 = 0
Water Level = unchanged
```

may indicate:

```text
Possible blockage
OR
Flow Sensor 2 fault
```

The AI/diagnostic layer should explicitly consider both possibilities.

---

# 17. SENSOR FAILURE DEMONSTRATION

The application must support a demonstration of sensor failure.

Example:

Disconnect Flow Sensor 2.

Instead of automatically saying:

```text
BLOCKAGE DETECTED
```

the application should be capable of showing:

```text
DATA QUALITY WARNING

Flow Sensor 2:
SUSPECTED FAULT

Blockage:
UNCONFIRMED

AI Confidence:
LOW

Recommended Action:
Inspect sensor before maintenance dispatch
```

This should become a major project demonstration.

---

# 18. UNCERTAINTY-AWARE AI

Do not treat every model output as absolute truth.

The system should communicate:

```text
Prediction
+
Confidence
+
Data Quality
```

Example:

```text
BLOCKAGE RISK
78%

CONFIDENCE
92%

DATA QUALITY
GOOD
```

versus:

```text
BLOCKAGE RISK
82%

CONFIDENCE
31%

DATA QUALITY
POOR
```

In the second case, the application should explicitly warn that the prediction is unreliable.

Never display a raw model probability as if it were guaranteed real-world probability without appropriate calibration/evaluation.

---

# 19. AI EXPLANABILITY

The application must explain why a prediction was made.

Example:

```text
WHY THIS PREDICTION?

↑ Flow difference increasing
↓ Downstream flow decreasing
↑ Water level rising
✓ Sensors are mutually consistent
✓ Similar pattern observed previously
```

The explanation must be generated from actual model features and current data.

Do not hard-code fake explanations.

---

# 20. FEATURE ENGINEERING

Initial features:

```text
flow1
flow2
flow_difference
water_level
pump_status
blockage_status
full_level_count
```

Derived features:

```text
flow_ratio
flow_drop_percentage
water_level_change_rate
flow_difference_change_rate
rolling_mean_flow_difference
rolling_std_flow_difference
```

Potential contextual features:

```text
rainfall
temperature
time_of-day
recent rainfall duration
```

Only include features for which reliable data is actually available.

---

# 21. FLOW RATIO

Use:

```text
flow_ratio = flow2 / flow1
```

Handle:

```text
flow1 = 0
```

safely.

Do not allow division-by-zero.

---

# 22. ENVIRONMENTAL CONTEXT

Rainfall is important because heavy rain can create high water levels without necessarily indicating a blockage.

The system should eventually distinguish patterns such as:

### Heavy inflow

```text
Rainfall ↑
Flow 1 ↑
Flow 2 ↑
Water level ↑
Flow ratio remains relatively healthy
```

from:

### Developing blockage

```text
Rainfall ↑
Flow 1 ↑
Flow 2 ↓
Flow difference ↑
Water level ↑
```

The rainfall feature should therefore be treated as contextual evidence, not as a direct blockage label.

If rainfall data is unavailable initially, the system must clearly state that environmental context is unavailable.

Do not fabricate rainfall data.

---

# 23. DATA COLLECTION IS A FIRST-CLASS COMPONENT

Do not train an AI model using fabricated examples.

Real data must be collected from the physical prototype.

Recommended experiment classes:

```text
NORMAL
PARTIAL BLOCKAGE
SEVERE BLOCKAGE
RISING WATER LEVEL
SENSOR FAULT
NOISY SENSOR CONDITION
RECOVERY
```

The dataset should contain multiple experiments for each condition.

---

# 24. DATASET SCHEMA

Recommended:

```text
timestamp
flow1
flow2
flow_difference
flow_ratio
water_level
water_level_change_rate
flow_difference_change_rate
pump_status
full_level_count
sensor1_health
sensor2_health
ultrasonic_health
rainfall
blockage_label
sensor_fault_label
risk_level
```

Not every field needs to exist in the first dataset.

The pipeline must support gradual expansion.

---

# 25. IMPORTANT: DATA LABELING

Labels must be based on controlled physical experiments.

For example:

```text
Valve open
→ NORMAL
```

```text
Valve partially closed
→ PARTIAL BLOCKAGE
```

```text
Valve mostly closed
→ SEVERE BLOCKAGE
```

```text
Flow Sensor 2 disconnected
→ SENSOR FAULT
```

Do not automatically use the existing rule engine as the only source of ground truth.

The experimental condition should be recorded separately.

---

# 26. MACHINE LEARNING STRATEGY

Start simple and defensible.

Preferred first model:

## Random Forest

Reasons:

* Suitable for tabular sensor data
* Captures nonlinear relationships
* Robust
* Easy to inspect
* Works well with engineered features
* Practical for a final-year engineering prototype

Do NOT use deep learning merely to make the project appear more advanced.

---

# 27. MODEL EVOLUTION

Potential progression:

```text
Version 1
Random Forest
```

then, if enough data exists:

```text
Version 2
Gradient Boosting / XGBoost
```

then optionally:

```text
Version 3
Time-series model
```

Possible anomaly detection:

```text
Isolation Forest
One-Class SVM
Autoencoder
```

Only implement these if the dataset and project objectives justify them.

---

# 28. MULTI-MODEL POSSIBILITY

The final system may use separate models:

```text
Model A
Blockage Risk

Model B
Sensor Fault Detection

Model C
Water-Level / Criticality Prediction
```

These can feed an AI fusion layer.

Example:

```text
Blockage Risk       84%
Sensor Fault Risk    7%
Water Risk           76%
```

The fusion engine can then produce:

```text
HIGH BLOCKAGE RISK
Confidence: 89%
```

Do not implement unnecessary complexity before the basic system works.

---

# 29. MODEL EVALUATION

When real data exists, evaluate using:

```text
Accuracy
Precision
Recall
F1 Score
Confusion Matrix
```

For multiclass classification, use appropriate macro/weighted metrics.

For binary classification, ROC-AUC may also be useful.

Metrics must be calculated from actual held-out data.

Never invent:

```text
97% accuracy
99% precision
```

just for presentation.

---

# 30. TEMPORAL DATA SPLITTING

Because this is sensor time-series data, avoid careless random splitting that can leak near-identical adjacent samples into both training and test sets.

Prefer:

```text
Earlier experiments
        ↓
Training

Later unseen experiments
        ↓
Validation/Test
```

Where practical, evaluate on entire unseen physical experiments rather than randomly shuffled individual readings.

This is important for demonstrating genuine generalization.

---

# 31. FEATURE IMPORTANCE

If Random Forest is used, show actual feature importance.

Example UI:

```text
FEATURE IMPORTANCE

Flow Difference          ███████████
Water Level              █████████
Flow Ratio               ███████
Downstream Flow          ██████
Water Level Rate         █████
```

These values must come from the trained model.

---

# 32. SHAP / EXPLAINABLE AI

If practical, integrate SHAP for local and global explanations.

For example:

```text
CURRENT PREDICTION

Flow Difference
        ↑ increases risk

Water Level
        ↑ increases risk

Flow Ratio
        ↓ increases risk

Sensor Reliability
        → reduces confidence
```

The UI should distinguish:

```text
Feature contribution
```

from:

```text
Raw feature value
```

Do not imply causal relationships unless experimentally established.

---

# 33. DRAIN HEALTH INDEX

Create:

# Drain Health Index — DHI

This should summarize the current condition of the monitored drainage segment.

Possible contributing dimensions:

```text
Flow Health
Water-Level Health
Blockage Risk
Sensor Reliability
Recent Events
```

Example:

```text
DRAIN HEALTH

82 / 100

Flow Health          91
Water Level Health   76
Sensor Reliability   95
Blockage Risk        68
Recent Events        74
```

The exact formula must be documented.

Do not invent a score without defining how it is calculated.

---

# 34. MAINTENANCE PRIORITY

The system should eventually answer:

> Which monitored drainage segment needs attention first?

For the current prototype, there may be only one physical segment.

Therefore the UI should support multiple virtual/physical nodes even if only one exists initially.

Example:

```text
NODE A
Risk: 91
Confidence: 94%
Priority: IMMEDIATE

NODE B
Risk: 67
Priority: HIGH

NODE C
Risk: 21
Priority: ROUTINE
```

The prototype can initially represent one monitored segment.

---

# 35. RECOMMENDED ACTION

The AI should provide an operational recommendation.

Examples:

```text
NORMAL
Continue monitoring
```

```text
DEVELOPING BLOCKAGE
Increase monitoring frequency
```

```text
HIGH BLOCKAGE RISK
Inspect downstream section
```

```text
SENSOR FAULT
Inspect sensor before dispatching blockage maintenance
```

```text
CRITICAL WATER LEVEL
Follow existing emergency/pump protection procedure
```

These recommendations are decision-support outputs.

They must not directly control safety hardware.

---

# 36. DIGITAL TWIN

Build a lightweight prototype-scale digital twin of the actual drainage system.

It should visually represent:

```text
Flow Sensor 1
      ↓
Pipe Segment
      ↓
Restriction / Blockage
      ↓
Flow Sensor 2
      ↓
Reservoir / Manhole
      ↓
Water Level
      ↓
Pump
```

The digital twin should synchronize with live sensor values.

---

# 37. DIGITAL TWIN STATES

### NORMAL

```text
Healthy flow
```

### RESTRICTED

```text
Reduced downstream flow
```

### BLOCKAGE

```text
Strong restriction
```

### WATER ACCUMULATION

```text
Rising virtual water level
```

### CRITICAL

```text
Pump protection active
```

The visualization should respond to actual telemetry.

---

# 38. WHAT-IF SIMULATION

Create a controlled simulation mode.

Example controls:

```text
Blockage Severity
[────────●────]

Initial Water Level
[────●────────]

Inflow / Rainfall
[──────●──────]
```

Then:

```text
RUN SIMULATION
```

Output:

```text
Predicted Water Level:
XX%

Predicted Risk:
HIGH

Estimated Time to Critical:
XX minutes
```

If the underlying predictive model is not yet available, clearly label this feature:

```text
SIMULATION / DEVELOPMENT MODE
```

Do not fabricate scientifically valid predictions.

---

# 39. DASHBOARD

The dashboard must immediately answer:

> What is happening with the drainage system right now?

Top-level card:

```text
OVERALL DRAINAGE STATUS
```

Possible states:

```text
NORMAL
LOW RISK
MEDIUM RISK
HIGH RISK
CRITICAL
SENSOR FAULT
UNCERTAIN
```

---

# 40. LIVE SENSOR CARDS

Display:

### Flow 1

```text
Flow 1
XX pulses/sec
```

### Flow 2

```text
Flow 2
XX pulses/sec
```

### Flow Difference

```text
Flow Difference
XX
```

### Water Level

```text
Water Level
XX%
```

### Pump

```text
RUNNING / STOPPED
```

### Full-Level Count

```text
X / 20
```

### Sensor Health

```text
GOOD / WARNING / FAULT
```

---

# 41. LIVE PIPE VISUALIZATION

Show:

```text
FLOW SENSOR 1
      ↓
   FLOWING
      ↓
  RESTRICTION
      ↓
FLOW SENSOR 2
      ↓
 WATER LEVEL
      ↓
    PUMP
```

Use animation to represent actual flow direction.

Do not overuse animation.

---

# 42. AI PREDICTION PAGE

Create:

# AI Diagnostics

Show:

```text
BLOCKAGE RISK
84%

CONFIDENCE
91%

SENSOR RELIABILITY
96%

SEVERITY
HIGH
```

Then:

```text
WHY?
```

Then show actual contributing features.

---

# 43. AI DIAGNOSIS PANEL

Example:

```text
AI DIAGNOSIS

Primary condition:
Developing blockage

Supporting evidence:
• Flow difference increasing
• Downstream flow decreasing
• Water level increasing
• Sensor readings consistent

Alternative explanation:
Low probability of sensor fault

Recommended action:
Inspect downstream pipe section
```

The alternative explanation is important.

---

# 44. ANALYTICS PAGE

Include:

### Flow History

```text
Flow 1
Flow 2
```

### Flow Difference

```text
Flow Difference vs Time
```

### Water Level

```text
Water Level vs Time
```

### AI Risk

```text
Risk vs Time
```

### Sensor Health

```text
Sensor reliability over time
```

### Events

```text
Blockage Events
Sensor Fault Events
Critical Water Events
```

---

# 45. ALERTS

Alerts should contain:

```text
Timestamp
Severity
Event
Sensor Evidence
AI Confidence
Recommended Action
Status
```

Examples:

```text
WARNING
Developing flow restriction detected
```

```text
HIGH
High blockage risk
```

```text
CRITICAL
Critical water level
Pump protection active
```

```text
DIAGNOSTIC
Flow Sensor 2 may be malfunctioning
```

---

# 46. SENSOR DATA PAGE

Columns:

```text
Timestamp
Flow 1
Flow 2
Difference
Flow Ratio
Water Level
Pump
Blockage
Sensor Health
AI Risk
Confidence
```

Features:

* Search
* Sorting
* Filtering
* Date range
* Export CSV
* Pagination

---

# 47. SYSTEM DIAGNOSTICS

Show:

```text
ESP32
CONNECTED / DISCONNECTED

Blynk
CONNECTED / DISCONNECTED

Flow Sensor 1
HEALTHY / FAULT

Flow Sensor 2
HEALTHY / FAULT

Ultrasonic Sensor
HEALTHY / FAULT

Relay
ACTIVE / INACTIVE

Pump
RUNNING / STOPPED
```

If information is unavailable:

```text
DATA NOT AVAILABLE
```

Never pretend.

---

# 48. DATA FRESHNESS

Always show:

```text
Updated 2 seconds ago
```

If stale:

```text
DATA STALE
Last update: 47 seconds ago
```

If disconnected:

```text
OFFLINE
```

---

# 49. BACKEND ARCHITECTURE

Preferred:

```text
ESP32
   ↓
Blynk / IoT
   ↓
Data Ingestion
   ↓
Database
   ↓
Feature Engineering
   ↓
Sensor Health
   ↓
ML Prediction
   ↓
AI Fusion
   ↓
REST API
   ↓
React Frontend
```

Backend:

```text
Python
FastAPI
pandas
NumPy
scikit-learn
```

Database:

```text
SQLite initially
```

PostgreSQL can be introduced if deployment requires it.

Do not introduce unnecessary microservices.

---

# 50. API DESIGN

Suggested endpoints:

```text
GET /api/health

GET /api/sensors/latest

GET /api/sensors/history

GET /api/status

GET /api/alerts

GET /api/analytics

GET /api/ai/prediction

POST /api/ai/predict

GET /api/ai/model-info

GET /api/sensors/health

GET /api/drain-health

GET /api/maintenance/priority
```

---

# 51. AI PREDICTION API

Example request:

```json
{
  "flow1": 52,
  "flow2": 31,
  "flow_difference": 21,
  "water_level": 67.4
}
```

Possible response:

```json
{
  "condition": "DEVELOPING_BLOCKAGE",
  "risk_score": 0.84,
  "confidence": 0.91,
  "sensor_reliability": 0.96,
  "severity": "HIGH",
  "reasons": [
    "Increasing flow difference",
    "Reduced downstream flow",
    "Rising water level"
  ],
  "recommended_action": "Inspect downstream section"
}
```

The exact API schema can evolve.

Do not expose raw internal model details unnecessarily.

---

# 52. MODEL INFORMATION PAGE

Show:

```text
Model
Random Forest Classifier

Features
XX

Training Experiments
XX

Validation Accuracy
XX%

Precision
XX%

Recall
XX%

F1 Score
XX%
```

Only display real values.

If training has not happened:

```text
MODEL STATUS
Awaiting real training data
```

---

# 53. DEMO MODE

Create:

# Demo Mode

Scenarios:

```text
Normal
Partial Blockage
Severe Blockage
Rapid Water Rise
Critical
Sensor Failure
Recovery
```

Clearly label:

```text
DEMO DATA
```

Never present demo values as live physical sensor data.

---

# 54. IMPORTANT DEMONSTRATION SEQUENCE

The final project demonstration should ideally show:

## Test 1 — Normal

```text
Flow 1 ≈ Flow 2
Water level stable

AI:
NORMAL
High confidence
```

## Test 2 — Partial blockage

Partially close the existing valve.

```text
Flow 1 > Flow 2
Difference increases

AI:
DEVELOPING BLOCKAGE
Risk increasing
```

## Test 3 — Severe blockage

Close further.

```text
Large flow difference
Water level rising

AI:
HIGH / CRITICAL
```

## Test 4 — Sensor fault

Disconnect Flow Sensor 2.

Expected behaviour:

```text
SENSOR FAULT SUSPECTED

Blockage:
UNCONFIRMED

Confidence:
LOW
```

## Test 5 — Recovery

Open the valve again.

The system should show:

```text
Risk decreasing
Flow recovering
Water level recovering
```

This demonstrates that the system understands a changing physical process rather than simply displaying static alarms.

---

# 55. LANDING PAGE

Hero:

```text
DRAINING INTELLIGENCE FOR SMART CITIES

Detect.
Diagnose.
Predict.
Explain.
Act.
```

Subheading:

```text
A trustworthy IoT and AI platform for real-time drainage monitoring,
blockage prediction and intelligent maintenance decision support.
```

Buttons:

```text
Open Live Dashboard
Explore AI
View Digital Twin
```

---

# 56. ABOUT PAGE

Explain the problem:

```text
Urban drainage systems are vulnerable to blockage,
water accumulation and delayed maintenance.
```

Explain the solution:

```text
DrainGuard AI combines:

IoT sensing
+
Rule-based protection
+
Machine learning
+
Sensor-health intelligence
+
Explainable AI
+
Predictive analytics
+
Digital twin
+
Maintenance decision support
```

---

# 57. TECH STACK

## Hardware

```text
ESP32
AJ-SR04M
ZJ-S201
Relay
Pump
LEDs
Buzzer
```

## IoT

```text
Blynk
```

## AI/ML

```text
Python
pandas
NumPy
scikit-learn
Random Forest
SHAP where appropriate
```

## Backend

```text
FastAPI
```

## Frontend

```text
React
Recharts
Lucide React
Axios
```

## Database

```text
SQLite / PostgreSQL
```

---

# 58. RESPONSIVE DESIGN

Must work on:

```text
Desktop
Laptop
Tablet
Mobile
```

Desktop is the primary target.

Charts must resize correctly.

Tables must remain usable on smaller screens.

---

# 59. VISUAL DESIGN

The design should communicate:

```text
Smart City
+
Infrastructure
+
AI
+
Engineering
```

Preferred:

* Dark professional dashboard
* Deep background
* Cyan/blue technology accents
* Green = safe
* Yellow = warning
* Red = critical
* White/light-gray text
* Clean charts
* Subtle glass effects
* Professional typography
* Minimal but meaningful animation

Avoid:

* Gaming aesthetics
* Excessive neon
* Excessive gradients
* Giant decorative elements
* Fake 3D effects
* Generic Bootstrap appearance

---

# 60. UX PRINCIPLES

The user should understand system condition within approximately five seconds.

The interface must answer:

```text
What is happening?
Why?
How confident are we?
What should I do?
```

Use:

* Loading states
* Empty states
* Error states
* Offline states
* Tooltips
* Clear severity indicators
* Accessible contrast
* Consistent spacing
* Consistent typography

---

# 61. ERROR HANDLING

If IoT is unavailable:

```text
IoT CONNECTION UNAVAILABLE

Live sensor data cannot currently be retrieved.
```

If AI is unavailable:

```text
AI PREDICTION UNAVAILABLE

The underlying IoT monitoring system remains operational.
```

If a sensor is suspected faulty:

```text
SENSOR DATA QUALITY WARNING

AI predictions may be unreliable until the sensor is inspected.
```

Never replace unavailable real data with fake values.

---

# 62. SECURITY

Never commit:

```text
Blynk Auth Token
Wi-Fi SSID
Wi-Fi Password
API Keys
Database Credentials
```

Use:

```text
.env
.env.example
```

Example:

```text
BLYNK_AUTH_TOKEN=
WIFI_SSID=
WIFI_PASSWORD=
DATABASE_URL=
```

Never expose credentials in frontend code.

---

# 63. PROJECT STRUCTURE

Suggested:

```text
project-root/

├── frontend/
│   ├── src/
│   │   ├── components/
│   │   ├── pages/
│   │   ├── services/
│   │   ├── hooks/
│   │   ├── utils/
│   │   └── App.jsx
│   └── package.json
│
├── backend/
│   ├── app/
│   │   ├── main.py
│   │   ├── api/
│   │   ├── services/
│   │   ├── models/
│   │   ├── ml/
│   │   └── database/
│   ├── requirements.txt
│   └── .env.example
│
├── ml/
│   ├── data/
│   ├── notebooks/
│   ├── models/
│   ├── train.py
│   ├── evaluate.py
│   └── feature_engineering.py
│
├── esp32/
│   └── drainage_monitor.ino
│
├── docs/
│
├── README.md
└── AI_APPLICATION_SPEC.md
```

Adapt this to the existing repository.

Do not blindly replace the existing project structure.

---

# 64. DEVELOPMENT PHASES

## PHASE 1 — Repository Analysis

Before writing code:

1. Inspect the entire repository.
2. Identify the current frontend.
3. Identify the current backend.
4. Identify the ESP32 code.
5. Identify Blynk integration.
6. Identify existing documentation.
7. Identify secrets and environment configuration.
8. Do not modify anything yet.

Then provide a concise implementation plan.

---

# 65. PHASE 2 — PROFESSIONAL UI

Build:

```text
Dashboard
Live Monitoring
AI Diagnostics
Analytics
Drain Health
Alerts
Sensor Data
System Diagnostics
Digital Twin
Simulation
About
```

Use demo data only where necessary.

Clearly mark demo data.

---

# 66. PHASE 3 — DATA PIPELINE

Implement:

```text
IoT
↓
Data ingestion
↓
Validation
↓
Storage
↓
Feature engineering
↓
Sensor health
```

Data validation must happen before ML.

---

# 67. PHASE 4 — SENSOR HEALTH

Implement basic rules first.

Examples:

```text
Impossible values
Missing values
Constant values
Sudden unrealistic jumps
Cross-sensor inconsistency
Communication timeout
```

Then optionally add ML-based anomaly detection.

---

# 68. PHASE 5 — MACHINE LEARNING

Implement:

```text
Real dataset
↓
Cleaning
↓
Feature engineering
↓
Experiment-based train/test split
↓
Random Forest
↓
Evaluation
↓
Model persistence
```

Save using:

```text
joblib
```

Never train using fabricated data and present the results as real.

---

# 69. PHASE 6 — EXPLAINABILITY

Implement:

```text
Feature importance
```

and, where practical:

```text
SHAP
```

The UI must clearly distinguish model explanation from causal proof.

---

# 70. PHASE 7 — AI FUSION

Combine:

```text
Blockage prediction
+
Sensor health
+
Water-level state
+
Environmental context
```

into a final decision.

Example:

```text
BLOCKAGE RISK       84%
SENSOR FAULT RISK    7%
WATER RISK          76%

FINAL:
HIGH BLOCKAGE RISK
```

If evidence conflicts:

```text
UNCERTAIN
```

should be an acceptable output.

---

# 71. PHASE 8 — DIGITAL TWIN

Connect the virtual drainage representation to actual telemetry.

Show:

```text
Flow
Restriction
Water Level
Pump
System State
```

---

# 72. PHASE 9 — WHAT-IF SIMULATION

Implement simulation only after the live system and ML pipeline work.

The simulation must clearly distinguish:

```text
REAL SENSOR DATA
```

from:

```text
SIMULATED DATA
```

---

# 73. PHASE 10 — TESTING

Test:

### Hardware integration

* Live sensor data
* Pump
* Relay
* Water level
* Blockage

### Sensor faults

* Flow sensor disconnect
* Ultrasonic abnormal reading
* Missing data
* Communication loss

### AI

* Normal condition
* Partial blockage
* Severe blockage
* Sensor fault
* Recovery

### Frontend

* Responsive layout
* API failure
* AI failure
* Empty state
* Offline state

---

# 74. PERFORMANCE

The application should:

* Avoid unnecessary API requests
* Poll at sensible intervals
* Clean up timers
* Avoid memory leaks
* Avoid unnecessary React renders
* Keep charts efficient
* Use lazy loading where useful

---

# 75. DO NOT DO THESE THINGS

DO NOT:

1. Rewrite the working ESP32 firmware unnecessarily.
2. Change GPIO assignments unnecessarily.
3. Break Blynk.
4. Remove existing sensor functionality.
5. Replace embedded safety with AI.
6. Claim AI accuracy without real evaluation.
7. Fabricate production sensor data.
8. Commit credentials.
9. Generate fake AI explanations.
10. Create a fake chatbot just to call the project "AI".
11. Build only a generic dashboard.
12. Add technologies without a real purpose.
13. Use deep learning without sufficient data.
14. Treat every anomaly as a blockage.
15. Treat every model probability as absolute truth.
16. Hide uncertainty from the user.
17. Present demo/simulated data as real sensor data.
18. Claim that the project is the first system in the world to use AI for drainage blockage detection.
19. Copy commercial product functionality without adapting it to this project's actual prototype.
20. Add unnecessary microservices.

---

# 76. WHAT ACTUALLY MAKES THIS PROJECT AI

The AI is NOT:

```text
Chatbot
+
Dashboard
```

The AI is:

```text
Real IoT Data
      ↓
Feature Engineering
      ↓
Sensor Health Analysis
      ↓
Machine Learning
      ↓
Multi-Condition Diagnosis
      ↓
Confidence / Uncertainty
      ↓
Explainable Prediction
      ↓
Risk Assessment
      ↓
Maintenance Recommendation
```

---

# 77. FUTURE EXTENSIONS

Only after the core system is working:

### Rainfall Integration

Use real rainfall data to improve contextual reasoning.

### Time-Series Prediction

Predict:

```text
Future water level
Future blockage risk
```

### Anomaly Detection

Potential:

```text
Isolation Forest
One-Class SVM
Autoencoder
```

### Advanced Models

Potential:

```text
XGBoost
Gradient Boosting
Temporal models
```

### Computer Vision

Future camera-based detection:

```text
Plastic
Leaves
Garbage
Debris
```

### Acoustic Sensing

Future extension:

```text
Acoustic blockage localization
```

Do not add these merely for complexity.

---

# 78. FINAL DEMONSTRATION STORY

The final demonstration should tell this story:

## STEP 1

System is healthy.

```text
NORMAL
```

## STEP 2

Valve is partially closed.

```text
Downstream flow decreases
```

AI detects:

```text
DEVELOPING BLOCKAGE
```

## STEP 3

Restriction increases.

```text
Water level rises
```

AI predicts:

```text
HIGH RISK
```

## STEP 4

Explain the prediction.

```text
Flow difference increasing
Downstream flow decreasing
Water level rising
Sensors consistent
```

## STEP 5

Disconnect Flow Sensor 2.

System should recognize:

```text
SENSOR FAULT SUSPECTED
```

rather than blindly declaring a blockage.

## STEP 6

Reconnect the sensor and recover the physical system.

System should show:

```text
RISK DECREASING
SYSTEM RECOVERING
```

This sequence demonstrates:

```text
Detection
+
Prediction
+
Explainability
+
Fault Awareness
+
Recovery
```

---

# 79. FINAL SUCCESS CRITERIA

## IoT

* Live sensor values
* Water level
* Flow 1
* Flow 2
* Flow difference
* Pump status
* Blockage status
* LED states

## AI

* Real ML model
* Real training data
* Experiment-based evaluation
* Blockage risk
* Sensor fault awareness
* Confidence
* Explainability
* Feature importance
* Actual model metrics

## Intelligence

* Multi-sensor reasoning
* Alternative explanations
* Sensor reliability
* Drain health
* Severity
* Maintenance recommendation

## Digital Twin

* Live physical-system representation
* Synchronized state
* Restriction visualization
* Water-level visualization
* Pump state

## Simulation

* Controlled what-if scenarios
* Clearly labelled simulated data
* Future prediction where supported by validated models

## Reliability

* No fake production values
* No fake AI metrics
* No exposed credentials
* AI failure does not compromise safety
* Sensor failure is communicated
* IoT failure is communicated
* Uncertainty is visible

---

# 80. INSTRUCTIONS TO CLAUDE CODE

You are acting as:

* Senior full-stack engineer
* Machine-learning engineer
* IoT systems engineer
* AI/ML product designer
* Data engineer
* UX designer

Before writing code:

1. Read this entire specification.
2. Inspect the entire repository.
3. Understand the existing architecture.
4. Identify all working functionality.
5. Preserve working IoT functionality.
6. Inspect existing ESP32 and Blynk code.
7. Identify existing frontend/backend before creating replacements.
8. Identify secrets and move them to environment variables if necessary.
9. Do not fabricate data.
10. Do not fabricate model metrics.
11. Do not claim unsupported AI capabilities.
12. Build incrementally.
13. Test every major phase.
14. Prefer simple, defensible engineering over unnecessary complexity.

Before making significant changes, produce a concise plan containing:

```text
1. Existing architecture
2. Files to preserve
3. Files to modify
4. Files to create
5. Dependencies to add
6. Data flow
7. ML strategy
8. Testing strategy
```

Then implement the system phase by phase.

---

# 81. MOST IMPORTANT PRODUCT PRINCIPLE

The application should NOT merely answer:

> "Is there a blockage?"

It should answer:

> **"What is happening, can I trust the sensor data, what is likely causing it, how confident is the system, what is likely to happen next, and what should be done?"**

That is the core identity of DrainGuard AI.

---

# 82. FINAL PRODUCT DEFINITION

> **DrainGuard AI is a trustworthy, explainable and fault-aware IoT drainage intelligence platform that transforms real-time multi-sensor drainage data into blockage diagnosis, predictive risk assessment, sensor-health analysis, drainage health scoring, and maintenance decision support through a prototype-scale digital twin.**
