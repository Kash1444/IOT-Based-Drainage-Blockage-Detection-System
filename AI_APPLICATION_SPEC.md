# AI APPLICATION SPECIFICATION

## IoT-Based Drainage Blockage Detection — AI Monitoring Platform

> This document is the primary specification for building the AI-powered web application for this project.
> Read this file completely before modifying or creating code.
> Treat the existing IoT/ESP32/Blynk implementation as a working system that must not be unnecessarily modified or broken.

---

# 1. PROJECT OVERVIEW

## Project Name

**IoT-Based Drainage Blockage Detection**

## Core Idea

This project is an IoT-based smart drainage monitoring system designed to detect drainage blockages and abnormal water-level conditions in real time.

The physical prototype represents a road drainage system commonly affected by blockage and water accumulation during heavy rainfall.

The system uses:

* ESP32
* Two flow sensors
* Waterproof ultrasonic water-level sensor
* Relay-controlled pump
* Buzzer
* Four status LEDs
* Blynk IoT dashboard

The current embedded system already performs deterministic blockage detection and water-level protection.

The next major component is an **AI-powered monitoring and prediction web application**.

The goal is to transform raw IoT sensor information into an intelligent dashboard capable of:

* Monitoring current drainage conditions
* Detecting abnormal patterns
* Estimating blockage risk
* Predicting developing blockage conditions
* Visualizing historical sensor data
* Explaining why the AI made a prediction
* Providing actionable warnings
* Showing system health
* Presenting the project in a professional smart-city style

---

# 2. IMPORTANT: CURRENT SYSTEM IS ALREADY WORKING

The following components are already implemented and tested.

DO NOT unnecessarily rewrite them.

### Hardware

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

### ESP32 Pin Configuration

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

The ultrasonic echo uses an appropriate voltage-divider arrangement.

---

# 3. CURRENT BLOCKAGE DETECTION LOGIC

The ESP32 currently compares the upstream and downstream flow sensors.

### Variables

* Flow 1 = upstream flow
* Flow 2 = downstream flow
* Flow Difference = Flow 1 - Flow 2

Current blockage parameters:

```text
BLOCKAGE_DIFFERENCE = 15
BLOCKAGE_CONFIRM_TIME = 3 seconds
```

The system requires:

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

This existing deterministic logic should remain the primary embedded safety mechanism.

The AI system should be an **additional intelligence layer**, not a replacement for the existing safety logic.

---

# 4. CURRENT WATER-LEVEL SYSTEM

Water level is calculated using the AJ-SR04M ultrasonic sensor.

Current calibration:

```text
EMPTY_DISTANCE = 27.75 cm
FULL_DISTANCE  = 20.49 cm
```

Water level is represented as:

```text
0% → empty
100% → critical/full
```

Current warning behavior:

| Water Level | Current Behavior            |
| ----------- | --------------------------- |
| 0–30%       | Green LED                   |
| 30–60%      | Yellow LED + slow warning   |
| 60–80%      | Red LED + faster warning    |
| 80–90%      | Rapid red warning           |
| 90–<99%     | All warning LEDs blink      |
| 99–100%     | Continuous critical warning |

The system also maintains a cumulative full-level count.

After more than 20 confirmed critical/full-level readings:

```text
Pump OFF
Relay OFF
Blue LED OFF
Warning LEDs OFF
Buzzer OFF
Shutdown latched
```

The cumulative count does NOT reset merely because the water level temporarily falls below 99%.

---

# 5. CURRENT BLYNK DATASTREAMS

The existing Blynk dashboard contains the following datastreams.

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

These represent the current IoT telemetry.

---

# 6. MAIN AI APPLICATION OBJECTIVE

Build a professional web application called:

# **DrainGuard AI**

Subtitle:

**Intelligent IoT Drainage Monitoring & Predictive Blockage Detection**

The application should look like a real-world smart-city infrastructure monitoring platform.

It should NOT look like:

* a generic student CRUD application
* a basic Bootstrap dashboard
* a simple chart page
* a static portfolio website
* a fake AI chatbot

It should look like a serious engineering product.

---

# 7. DESIGN DIRECTION

The visual design should communicate:

**Smart City + IoT + AI + Infrastructure Monitoring**

Preferred visual characteristics:

* Modern
* Professional
* Dark monitoring-dashboard aesthetic
* High information density without becoming cluttered
* Clean cards
* Subtle gradients
* Glass / translucent panels where appropriate
* Professional charts
* Status indicators
* Smooth animations
* Responsive design
* Excellent desktop experience
* Usable mobile experience

Suggested visual language:

* Deep dark background
* Cyan / blue technology accents
* Green = safe
* Yellow = warning
* Red = critical
* White/light-gray typography

Do NOT overuse neon effects.

Do NOT make it look like a gaming website.

The visual style should be closer to:

```text
Smart City Operations Center
+
AI Infrastructure Monitoring
+
Modern SaaS Dashboard
```

---

# 8. APPLICATION STRUCTURE

The application should have a professional navigation structure.

Suggested navigation:

```text
Dashboard
Live Monitoring
AI Predictions
Analytics
Drainage Health
Alerts
Sensor Data
System Diagnostics
About System
```

A sidebar navigation is preferred on desktop.

A responsive navigation system should be used on mobile.

---

# 9. DASHBOARD

The Dashboard is the most important page.

It should immediately answer:

> "What is happening with the drainage system right now?"

Top-level status card:

## Overall Drainage Status

Example states:

```text
NORMAL
LOW RISK
MEDIUM RISK
HIGH RISK
CRITICAL
```

Display:

* Current status
* AI confidence
* Last updated time
* System connectivity
* Current blockage state

---

# 10. LIVE SENSOR CARDS

Create highly polished real-time cards for:

### Flow 1

Display:

```text
Flow 1
XX pulses/sec
```

### Flow 2

Display:

```text
Flow 2
XX pulses/sec
```

### Flow Difference

Display:

```text
Flow Difference
XX
```

### Water Level

Display:

```text
Water Level
XX %
```

### Pump

Display:

```text
Pump
RUNNING / STOPPED
```

### Full-Level Count

Display:

```text
Critical Count
X / 20
```

Each card should contain a small trend indicator where meaningful.

---

# 11. LIVE DRAINAGE VISUALIZATION

Create a central visual representation of the drainage pipeline.

The visualization should conceptually show:

```text
                 Water Flow
                     ↓

       ┌─────────────────────────┐
       │                         │
       │       FLOW SENSOR 1     │
       │             ↓           │
       │        ───────────      │
       │             ↓           │
       │        BLOCKAGE?        │
       │             ↓           │
       │       FLOW SENSOR 2     │
       │             ↓           │
       └─────────────────────────┘

                    ↓

              Water Reservoir
                    ↓
               Water Level
```

The visualization should change according to system state.

For example:

NORMAL:

```text
Healthy flow → Healthy flow
```

BLOCKAGE:

```text
Healthy flow → Restricted flow
```

CRITICAL:

```text
Restricted flow → Rising water → Pump protection
```

This should be visually impressive but still understandable.

---

# 12. AI PREDICTION PAGE

Create a dedicated page:

## AI Predictions

This is the core differentiating feature of the project.

The AI should estimate:

### Current Risk

```text
NORMAL
LOW
MEDIUM
HIGH
CRITICAL
```

Display:

* Predicted risk
* Probability/confidence
* Important contributing factors
* Prediction timestamp
* Current sensor values
* Historical trend

Example:

```text
HIGH BLOCKAGE RISK

Confidence
87%

Primary indicators:

Flow difference increasing
Downstream flow decreasing
Water level rising
```

---

# 13. AI EXPLANATION

The application should not simply say:

> "AI says HIGH RISK."

It should explain the prediction.

Create a section:

## Why is the AI predicting this?

Example:

```text
The system detected:

• Downstream flow is significantly lower than upstream flow
• Flow difference has increased over the last 60 seconds
• Water level is rising
• The pattern resembles previous blockage events

These combined signals increased the predicted blockage risk.
```

This is extremely important for demonstrating responsible AI.

---

# 14. AI RISK GAUGE

Create a visually impressive risk gauge.

Example:

```text
                 87%
              HIGH RISK

       LOW ───────────── HIGH
```

The gauge should dynamically reflect the AI prediction.

---

# 15. AI FEATURES

The AI model should eventually use features such as:

```text
flow1
flow2
flow_difference
water_level
water_level_change_rate
flow_difference_change_rate
pump_status
blockage_status
full_level_count
```

Additional engineered features can include:

```text
flow_ratio
rolling_mean_flow_difference
rolling_std_flow_difference
water_level_velocity
flow_drop_percentage
```

Example:

```text
flow_ratio = flow2 / flow1
```

Handle division by zero safely.

---

# 16. AI MODEL

The first production model should preferably be:

## Random Forest Classifier

Reason:

* Works well with tabular sensor data
* Handles nonlinear relationships
* Easy to explain
* Robust for a student/engineering prototype
* Already aligned with the project's ML direction

Possible target:

```text
risk_level
```

with classes:

```text
NORMAL
LOW
MEDIUM
HIGH
CRITICAL
```

However, if the available real dataset is initially too small for five reliable classes, begin with:

```text
NORMAL
BLOCKAGE RISK
CRITICAL
```

and expand later.

DO NOT fabricate model accuracy.

If the model has not yet been trained on real data, clearly label the AI module as:

```text
AI MODEL: DEVELOPMENT / DATA COLLECTION
```

Do not display fake percentages.

---

# 17. DATA COLLECTION

The system should support historical IoT sensor data.

Recommended dataset structure:

```text
timestamp
flow1
flow2
flow_difference
water_level
water_level_change_rate
flow_difference_change_rate
flow_ratio
pump_status
blockage_status
full_level_count
risk_label
```

Example:

```csv
timestamp,flow1,flow2,flow_difference,water_level,water_level_change_rate,flow_ratio,pump_status,blockage_status,risk_label
2026-09-13 10:00:01,52,51,1,22.4,0.1,0.98,1,0,NORMAL
2026-09-13 10:00:02,52,48,4,22.6,0.2,0.92,1,0,NORMAL
2026-09-13 10:00:03,52,30,22,23.1,0.5,0.58,1,1,HIGH
```

This is an example schema only.

Do NOT treat these example values as real training data.

---

# 18. ANALYTICS PAGE

Create an analytics page showing historical behavior.

Include:

### Flow History

Line chart:

```text
Flow 1
Flow 2
```

### Flow Difference

Line chart showing:

```text
Flow Difference vs Time
```

### Water Level

Area/line chart:

```text
Water Level vs Time
```

### AI Risk

Timeline showing:

```text
Risk Level vs Time
```

### Blockage Events

Show:

```text
Number of detected events
Average duration
Peak severity
```

---

# 19. CORRELATION / AI INSIGHTS

Create an intelligent insights section.

Example:

```text
AI Insight

Downstream flow has fallen 42% relative to upstream flow
over the last 30 seconds.

Water level is increasing simultaneously.

This pattern is consistent with developing flow restriction.
```

The insights must be generated from actual data.

Do not create fake insights.

---

# 20. ALERTS PAGE

Create an Alerts page.

Alerts should include:

### Information

```text
System operating normally
```

### Warning

```text
Potential drainage restriction detected
```

### High Risk

```text
High blockage probability detected
```

### Critical

```text
Critical water level detected
Pump protection activated
```

Each alert should contain:

* Timestamp
* Severity
* Event
* Sensor evidence
* Status

---

# 21. SENSOR DATA PAGE

Create a detailed data table.

Columns:

```text
Timestamp
Flow 1
Flow 2
Difference
Water Level
Pump
Blockage
AI Risk
```

Features:

* Search
* Filtering
* Sorting
* Date filtering
* Export CSV
* Pagination

---

# 22. SYSTEM DIAGNOSTICS

Create a System Diagnostics page.

Show:

```text
ESP32
CONNECTED / DISCONNECTED

Blynk
CONNECTED / DISCONNECTED

Flow Sensor 1
ONLINE / OFFLINE

Flow Sensor 2
ONLINE / OFFLINE

Ultrasonic Sensor
ONLINE / OFFLINE

Relay
ACTIVE / INACTIVE

Pump
RUNNING / STOPPED

Last Data Received
XX seconds ago
```

If real connectivity information is unavailable, clearly indicate:

```text
DATA NOT AVAILABLE
```

Do not pretend that a component is connected.

---

# 23. DATA FRESHNESS

Real-time data must display a clear timestamp.

Example:

```text
Updated 2 seconds ago
```

If data becomes stale:

```text
DATA STALE
Last update: 47 seconds ago
```

If disconnected:

```text
OFFLINE
```

This makes the dashboard feel like a real monitoring platform.

---

# 24. BACKEND ARCHITECTURE

Preferred architecture:

```text
ESP32
   ↓
Blynk / IoT Layer
   ↓
Data Ingestion API
   ↓
Database
   ↓
Feature Engineering
   ↓
ML Prediction Service
   ↓
REST API
   ↓
React Frontend
```

Recommended backend technology:

```text
Python
FastAPI
scikit-learn
pandas
NumPy
```

Recommended database for the prototype:

```text
SQLite
```

or

```text
PostgreSQL
```

Use PostgreSQL if deployment architecture benefits from it.

---

# 25. FRONTEND

Preferred:

```text
React
```

Use the project's existing frontend if one exists.

Do NOT create an unnecessary second frontend framework.

Recommended libraries where useful:

```text
React Router
Recharts
Lucide React
Axios
```

Use modern CSS or the project's existing styling system.

Avoid unnecessary dependencies.

---

# 26. API DESIGN

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
```

Possible response:

```json
{
  "risk_level": "HIGH",
  "probability": 0.87,
  "timestamp": "2026-09-13T10:00:03",
  "reasons": [
    "High flow difference",
    "Downstream flow reduction",
    "Rising water level"
  ]
}
```

---

# 27. AI MODEL INFORMATION PAGE

Show model transparency.

Example:

```text
Model
Random Forest Classifier

Features
10

Training Samples
XXXX

Validation Accuracy
XX%

Precision
XX%

Recall
XX%

F1 Score
XX%
```

ONLY show these values when they actually exist.

Never invent metrics.

---

# 28. MODEL EVALUATION

When real training data is available, evaluate using:

```text
Accuracy
Precision
Recall
F1 Score
Confusion Matrix
ROC-AUC where appropriate
```

For a multiclass model, use appropriate multiclass metrics.

The application should ideally provide a visual confusion matrix on the AI/model page.

---

# 29. FEATURE IMPORTANCE

Because Random Forest supports feature importance, show:

```text
Feature Importance
```

Example:

```text
Flow Difference              ███████████
Water Level                  █████████
Flow Ratio                   ███████
Downstream Flow              ██████
Water Level Change Rate      █████
```

These values must come from the actual trained model.

This feature is useful for demonstrating how the AI reaches its predictions.

---

# 30. AI VS RULE-BASED DETECTION

The application should clearly distinguish between:

### Existing Embedded Rule Engine

```text
Flow difference threshold
+
3-second confirmation
```

and:

### AI Prediction

```text
Multiple sensor patterns
+
Historical behavior
+
Machine learning
```

The AI should ideally detect developing risk before the deterministic threshold is fully triggered.

This distinction should be clearly explained in the UI.

---

# 31. IMPORTANT SAFETY ARCHITECTURE

The AI must NEVER directly control the pump safety mechanism unless explicitly designed and tested later.

Current architecture:

```text
ESP32 SAFETY LOGIC
        ↓
Pump Protection
```

AI:

```text
ESP32 / IoT Data
        ↓
AI MODEL
        ↓
Prediction / Recommendation
```

AI failure must not disable:

* Water-level protection
* Relay safety
* Pump shutdown
* Existing blockage detection

This separation is mandatory.

---

# 32. DEMO MODE

The application should support a controlled demonstration mode.

Create:

```text
Demo Mode
```

This allows the project to demonstrate different conditions without requiring physical hardware every time.

Possible scenarios:

```text
Normal Drainage
Partial Blockage
Severe Blockage
Rapid Water Rise
Critical Water Level
Recovery
```

Demo Mode should be clearly labelled:

```text
DEMO DATA
```

Never mix demo data with real sensor data without clearly identifying it.

---

# 33. LIVE DEMONSTRATION FLOW

The application should make this demonstration extremely easy:

### Scenario 1 — Normal

Display:

```text
NORMAL
Low blockage risk
Healthy flow
```

### Scenario 2 — Partial Blockage

Simulate reduction in downstream flow.

Display:

```text
FLOW RESTRICTION DETECTED
AI RISK INCREASING
```

### Scenario 3 — Severe Blockage

Display:

```text
HIGH BLOCKAGE RISK
```

### Scenario 4 — Rising Water Level

Display:

```text
WATER LEVEL WARNING
```

### Scenario 5 — Critical

Display:

```text
CRITICAL WATER LEVEL
PUMP PROTECTION ACTIVE
```

This should make the final project demonstration visually strong.

---

# 34. PROJECT LANDING PAGE

The root page can also include a professional project overview.

Hero section:

```text
AI-Powered Drainage Monitoring

Detect blockages.
Predict risk.
Protect urban drainage infrastructure.
```

Buttons:

```text
Open Live Dashboard
Explore AI
View System
```

Include a simple architecture visualization:

```text
SENSORS
   ↓
ESP32
   ↓
IoT CLOUD
   ↓
AI ENGINE
   ↓
PREDICTION
   ↓
SMART ALERTS
```

---

# 35. ABOUT SYSTEM PAGE

Explain:

### Problem

Urban drainage systems can become blocked by:

* Plastic
* Leaves
* Mud
* Waste
* Sediment

Blockages reduce water flow and can cause water accumulation and flooding.

### Solution

This project combines:

```text
IoT sensing
+
Real-time monitoring
+
Rule-based detection
+
Machine learning
+
Predictive analytics
```

to create an intelligent drainage monitoring platform.

---

# 36. TECH STACK

Display the technology stack:

### Hardware

```text
ESP32
AJ-SR04M
ZJ-S201
Relay
Pump
LEDs
Buzzer
```

### IoT

```text
Blynk
```

### AI/ML

```text
Python
pandas
NumPy
scikit-learn
Random Forest
```

### Backend

```text
FastAPI
```

### Frontend

```text
React
```

### Database

```text
SQLite / PostgreSQL
```

---

# 37. RESPONSIVENESS

The application MUST work properly on:

```text
Desktop
Laptop
Tablet
Mobile
```

Desktop should be the primary design target.

The dashboard should not break when the screen becomes narrow.

Charts should resize correctly.

Tables should become horizontally scrollable or responsive.

---

# 38. UX REQUIREMENTS

Use:

* Loading states
* Empty states
* Error states
* Offline states
* Tooltips
* Clear status labels
* Accessible contrast
* Keyboard-friendly controls
* Consistent spacing
* Consistent typography

Avoid:

* Excessive animations
* Giant text everywhere
* Random gradients
* Unnecessary popups
* Fake loading screens
* Fake data presented as real

---

# 39. PERFORMANCE

The application should:

* Avoid unnecessary API requests
* Poll live data at a sensible interval
* Clean up timers when components unmount
* Avoid memory leaks
* Avoid unnecessary React re-renders
* Lazy-load heavy pages where useful
* Keep chart rendering efficient

---

# 40. ERROR HANDLING

If the IoT backend is unavailable:

Show:

```text
IoT CONNECTION UNAVAILABLE

The dashboard cannot currently retrieve live sensor data.
```

Do NOT show fabricated sensor values.

If the AI model is unavailable:

```text
AI PREDICTION UNAVAILABLE

The underlying IoT monitoring system remains operational.
```

This distinction is important.

---

# 41. SECURITY

Never commit:

```text
Blynk Auth Token
Wi-Fi SSID
Wi-Fi Password
API Keys
Database Credentials
Secrets
```

Use:

```text
.env
```

and provide:

```text
.env.example
```

Example:

```text
BLYNK_AUTH_TOKEN=
WIFI_SSID=
WIFI_PASSWORD=
DATABASE_URL=
```

Never expose secrets in frontend source code.

---

# 42. CODE QUALITY

Write production-quality code.

Requirements:

* Clear folder structure
* Reusable components
* Meaningful variable names
* Modular backend
* Environment variables
* Proper error handling
* API separation
* No giant monolithic files
* No duplicated components
* No unnecessary packages
* Comments only where useful

---

# 43. RECOMMENDED PROJECT STRUCTURE

A possible structure:

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

Adapt this structure to the existing repository rather than blindly replacing it.

---

# 44. DEVELOPMENT STRATEGY

Build in phases.

## PHASE 1 — UI

Build:

* Dashboard
* Live Monitoring
* Analytics
* AI Predictions
* Alerts
* Sensor Data
* Diagnostics
* About

Use clearly labelled demo/mock data ONLY if real API data is not yet available.

---

## PHASE 2 — Backend

Implement:

* FastAPI
* Sensor endpoints
* Historical data
* Database
* Health checks
* Alert system

---

## PHASE 3 — Data Pipeline

Implement:

```text
IoT
↓
Data ingestion
↓
Storage
↓
Feature engineering
```

---

## PHASE 4 — ML

Implement:

```text
Dataset
↓
Cleaning
↓
Feature engineering
↓
Train/test split
↓
Random Forest
↓
Evaluation
↓
Model persistence
```

Save model using an appropriate method such as:

```text
joblib
```

---

## PHASE 5 — AI API

Expose:

```text
POST /api/ai/predict
```

Input:

```json
{
  "flow1": 52,
  "flow2": 31,
  "flow_difference": 21,
  "water_level": 67.4
}
```

Return:

```json
{
  "risk_level": "HIGH",
  "probability": 0.87,
  "reasons": [
    "Large upstream/downstream flow difference",
    "Reduced downstream flow"
  ]
}
```

---

## PHASE 6 — Frontend Integration

Connect the frontend to the backend.

Replace demo data with real API data.

---

## PHASE 7 — Testing

Test:

* API
* ML prediction
* Frontend
* Responsive design
* Error handling
* IoT disconnection
* AI failure
* Demo mode

---

# 45. DO NOT DO THESE THINGS

DO NOT:

1. Rewrite the working ESP32 firmware unnecessarily.
2. Change the existing GPIO assignments without a strong reason.
3. Break the Blynk integration.
4. Remove existing sensor functionality.
5. Replace the deterministic safety mechanism with AI.
6. Claim AI accuracy without real training/evaluation.
7. Fabricate sensor readings as real data.
8. Commit credentials.
9. Create fake AI explanations unrelated to actual features.
10. Add unnecessary technologies just to make the project appear complex.
11. Build a generic dashboard template.
12. Overcomplicate the system with microservices unless genuinely necessary.
13. Add an LLM chatbot merely because the project contains AI.
14. Make AI the only method of detecting dangerous water levels.

---

# 46. WHAT MAKES THIS PROJECT "AI"

The AI component should provide genuine machine-learning functionality.

The goal is NOT:

```text
Chatbot + Dashboard = AI
```

The goal is:

```text
IoT Sensor Data
       ↓
Feature Engineering
       ↓
Machine Learning
       ↓
Blockage Risk Prediction
       ↓
Explainable Risk Indicators
       ↓
Early Warning
```

The AI should ideally identify patterns that indicate an increasing likelihood of blockage before the traditional threshold detector becomes fully triggered.

---

# 47. FUTURE AI EXTENSIONS

After the basic Random Forest implementation works, optional improvements can include:

### Time-Series Prediction

Use historical sequences to predict:

```text
Water level after 1 minute
Water level after 5 minutes
```

### Anomaly Detection

Detect unusual drainage behavior without requiring labels.

Possible methods:

```text
Isolation Forest
Autoencoder
One-Class SVM
```

### Predictive Maintenance

Estimate:

```text
Sensor health
Pump health
Abnormal sensor behavior
```

### Computer Vision

Future version could use a camera to detect:

```text
Garbage accumulation
Plastic blockage
Leaves
Debris
```

Do not implement these unless they provide real value and sufficient data exists.

---

# 48. FINAL PRODUCT EXPERIENCE

When a user opens the application, the experience should be:

```text
OPEN APPLICATION
       ↓
SYSTEM STATUS
       ↓
LIVE SENSOR DATA
       ↓
DRAINAGE VISUALIZATION
       ↓
AI RISK
       ↓
WHY AI THINKS THIS
       ↓
HISTORICAL TREND
       ↓
ALERTS
```

The user should understand the condition of the drainage system within approximately 5 seconds.

---

# 49. FINAL SUCCESS CRITERIA

The application is considered successful when:

### IoT

* Live sensor values can be displayed
* Water level is displayed
* Flow 1 and Flow 2 are displayed
* Flow difference is displayed
* Pump status is displayed
* Blockage status is displayed

### AI

* A real ML model can be trained
* Model predictions can be requested through an API
* Risk level is displayed
* Prediction confidence is displayed
* Feature importance is available
* AI reasoning is based on actual sensor features
* Model metrics are displayed only when genuinely calculated

### Dashboard

* Professional UI
* Responsive layout
* Live monitoring
* Historical charts
* Alerts
* Diagnostics
* AI prediction page

### Reliability

* No fake production data
* No exposed credentials
* AI failure does not break safety controls
* IoT failure is clearly communicated

---

# 50. DEVELOPMENT INSTRUCTION FOR CLAUDE

You are acting as a **senior full-stack engineer + ML engineer + product designer**.

Before writing code:

1. Inspect the existing repository.
2. Understand the current architecture.
3. Identify what already works.
4. Do not overwrite working functionality unnecessarily.
5. Follow this specification.
6. Build incrementally.
7. Test after every major phase.
8. Keep the implementation understandable.
9. Prefer a clean working system over unnecessary complexity.
10. Never fabricate AI results or performance metrics.

The final result should feel like a **real AI-powered smart-city drainage monitoring product**, while remaining technically honest and demonstrable using the actual IoT hardware.

---

# 51. ONE-SENTENCE PRODUCT DEFINITION

> **DrainGuard AI is an intelligent IoT drainage monitoring platform that combines real-time flow and water-level sensing with machine-learning-based blockage risk prediction to provide early warnings and protect drainage infrastructure.**
