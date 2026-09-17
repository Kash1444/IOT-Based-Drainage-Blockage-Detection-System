# IoT Based Drainage Blockage Detection

An IoT-based smart drainage monitoring and blockage detection system designed to detect drainage blockages, monitor water levels, and automatically protect a drainage pumping system during critical water-level conditions.

The prototype uses an **ESP32**, two **flow sensors**, a waterproof **ultrasonic water-level sensor**, LEDs, a buzzer, a relay-controlled pump, and **Blynk IoT** for real-time monitoring.

![Drain Guard AI Dashboard](https://raw.githubusercontent.com/Kash1444/IOT-Based-Drainage-Blockage-Detection-System/5ec15b4f503e0e511ddbdd05ee7567ed31326c7b/Drain%20Guard%20AI%20Dashboard.png)

---

## Project Overview

Drainage blockages during heavy rainfall can cause water accumulation, road flooding, and damage to surrounding infrastructure.

This project presents a low-cost IoT prototype that continuously monitors:

* Water flow before and after a simulated blockage
* Drainage water level
* Difference between upstream and downstream flow
* Blockage conditions
* Pump status
* Critical water-level conditions

When a significant reduction in downstream flow is detected for a sustained period, the system identifies a potential blockage.

The system also provides automatic pump protection when the water level remains critically high.

---

## System Architecture

```text
                 ┌─────────────────────┐
                 │      Water Tank     │
                 │     / Reservoir     │
                 └──────────┬──────────┘
                            │
                            ▼
                     ┌─────────────┐
                     │   Pump      │
                     └──────┬──────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Flow Sensor 1 │
                    │   Upstream    │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Ball Valve /  │
                    │   Blockage    │
                    │  Simulation   │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Flow Sensor 2 │
                    │  Downstream   │
                    └───────┬───────┘
                            │
                            ▼
                    ┌───────────────┐
                    │ Drain / T-Joint│
                    │   Overflow    │
                    └───────────────┘

                            │
                            ▼
                  ┌───────────────────┐
                  │  Ultrasonic       │
                  │  Water Level      │
                  │     Sensor        │
                  └─────────┬─────────┘
                            │
                            ▼
                     ┌────────────┐
                     │   ESP32    │
                     │ Controller │
                     └─────┬──────┘
                           │
          ┌────────────────┼────────────────┐
          │                │                │
          ▼                ▼                ▼
     LEDs + Buzzer       Relay          Blynk Cloud
                           │                │
                           ▼                ▼
                         Pump          Web Dashboard
```

---

## Features

### 1. Dual Flow Monitoring

Two flow sensors are installed at different points in the drainage pipeline.

* **Flow Sensor 1:** Measures upstream flow
* **Flow Sensor 2:** Measures downstream flow

The system calculates:

```text
Flow Difference = Flow 1 - Flow 2
```

A significant positive difference indicates that water is entering the section faster than it is leaving it, suggesting a possible blockage.

---

### 2. Blockage Detection

The current detection logic uses:

```text
Minimum upstream flow = 20 pulses/sec
Minimum flow difference = 15 pulses/sec
Confirmation time = 3 seconds
```

A blockage is detected when:

```text
Flow 1 >= 20 pulses/sec
AND
Flow 1 - Flow 2 >= 15 pulses/sec
FOR
3 consecutive seconds
```

The system then changes the blockage status to:

```text
BLOCKAGE DETECTED
```

If the condition disappears, the blockage status returns to:

```text
NORMAL
```

---

### 3. Water-Level Monitoring

An **AJ-SR04M waterproof ultrasonic sensor** measures the water level inside the prototype reservoir.

The system converts the measured distance into a percentage:

```text
0%   → Empty
100% → Full
```

Current calibration:

```text
Empty distance = 27.75 cm
Full distance  = 20.49 cm
```

---

### 4. Multi-Level Warning System

The LEDs provide a local visual indication of the water level.

| Water Level | Indicator                                     |
| ----------- | --------------------------------------------- |
| 0–30%       | Green                                         |
| 30–60%      | Yellow                                        |
| 60–80%      | Red                                           |
| 80–90%      | Rapid red blinking                            |
| 90–99%      | Green + Yellow + Red blinking                 |
| 99–100%     | All warning LEDs blinking + continuous buzzer |

The blue LED indicates the pump status.

---

### 5. Automatic Pump Protection

To prevent continuous pumping during an extreme water-level condition, the system uses cumulative full-level confirmations.

Whenever:

```text
Water Level >= 99%
```

the system records a full-level confirmation.

The confirmations are **cumulative** and do not reset when the water level temporarily drops.

After:

```text
20 cumulative confirmations
```

the system:

1. Turns the pump OFF
2. Turns the relay OFF
3. Turns the blue LED OFF
4. Turns all warning LEDs OFF
5. Turns the buzzer OFF
6. Locks the system into shutdown state

This prevents the pump from continuously operating during a persistent critical condition.

---

## Blynk IoT Monitoring

The ESP32 sends real-time information to a Blynk dashboard.

### Datastreams

| Virtual Pin | Datastream         | Description                             |
| ----------- | ------------------ | --------------------------------------- |
| V0          | Flow 1             | Upstream flow                           |
| V1          | Flow 2             | Downstream flow                         |
| V2          | Water Level        | Current water level (%)                 |
| V3          | Flow Difference    | Difference between Flow 1 and Flow 2    |
| V4          | Blockage Status    | NORMAL / BLOCKAGE DETECTED              |
| V5          | Pump Status        | ON / OFF - SHUTDOWN                     |
| V6          | Full-Level Count   | Cumulative critical-level confirmations |
| V7          | Water Level Status | Current water-level condition           |
| V8          | Blue LED           | Pump indicator                          |
| V9          | Green LED          | Normal/low-level indicator              |
| V10         | Yellow LED         | Medium-level indicator                  |
| V11         | Red LED            | High/critical-level indicator           |

The physical LEDs and their Blynk indicators are synchronized with the current system state.

---

## Hardware Components

### Main Controller

* ESP32 DevKit V1

### Sensors

* 2 × ZJ-S201 flow sensors
* 1 × AJ-SR04M waterproof ultrasonic sensor

### Actuators

* 1 × AC submersible/cooler pump
* 1 × 5V relay module
* 1 × active buzzer
* 4 × LEDs

### Additional Components

* 150Ω resistors
* Breadboard
* Jumper wires
* PVC pipe
* 1/2" ball valve
* T-joint
* Water reservoir/container

---

## Pin Configuration

| Component       | ESP32 GPIO |
| --------------- | ---------: |
| Ultrasonic TRIG |    GPIO 27 |
| Ultrasonic ECHO |    GPIO 26 |
| Flow Sensor 1   |    GPIO 32 |
| Flow Sensor 2   |    GPIO 33 |
| Relay           |    GPIO 23 |
| Buzzer          |    GPIO 22 |
| Blue LED        |    GPIO 25 |
| Green LED       |    GPIO 19 |
| Yellow LED      |    GPIO 21 |
| Red LED         |    GPIO 18 |

> The ultrasonic sensor's ECHO signal uses the required voltage-divider arrangement for the ESP32 input.

---

## Software

The project is developed using:

* Arduino IDE
* C/C++ for ESP32
* Blynk IoT
* ESP32 Arduino Core
* Blynk ESP32 library

### Required Libraries

```text
WiFi.h
BlynkSimpleEsp32.h
```

`WiFi.h` is included with the ESP32 Arduino environment.

Install the **Blynk** library through the Arduino IDE Library Manager.

---

## Blynk Configuration

Create a Blynk template with:

```text
Template Name:
IoT based Drainage Block Detection

Template ID:
TMPL3zJG9PmKu
```

Create the following datastreams:

```text
V0  Flow 1
V1  Flow 2
V2  Water Level
V3  Flow Difference
V4  Blockage Status
V5  Pump Status
V6  Full-Level Count
V7  Water Level Status
V8  Blue LED
V9  Green LED
V10 Yellow LED
V11 Red LED
```

Add appropriate dashboard widgets for each datastream.

---

## How Blockage Detection Works

Under normal conditions:

```text
Flow 1 ≈ Flow 2
```

For example:

```text
Flow 1 = 52 pulses/sec
Flow 2 = 50 pulses/sec

Difference = 2 pulses/sec

Status = NORMAL
```

When the simulated blockage is introduced:

```text
Flow 1 = 52 pulses/sec
Flow 2 = 10 pulses/sec

Difference = 42 pulses/sec
```

If this condition continues for at least 3 seconds:

```text
Status = BLOCKAGE DETECTED
```

This allows the system to distinguish a sustained blockage condition from short-term flow variations.

---

## Demonstration Procedure

### Normal Condition

Keep the ball valve open.

Expected result:

```text
Flow 1 ≈ Flow 2
Blockage = NORMAL
Green LED = ON
Pump = ON
```

### Simulated Blockage

Gradually close the ball valve.

Expected result:

```text
Flow 1 remains relatively high
Flow 2 decreases
Flow Difference increases
```

After the configured confirmation period:

```text
BLOCKAGE DETECTED
```

will appear on the Blynk dashboard.

### Critical Water Level

Allow the water level to rise.

The warning system progresses through:

```text
GREEN
   ↓
YELLOW
   ↓
RED
   ↓
RAPID RED BLINK
   ↓
ALL WARNING LEDs BLINK
   ↓
CRITICAL
```

After 20 cumulative full-level confirmations:

```text
PUMP OFF
```

---

## Current System Status

The current prototype successfully integrates:

* ESP32 control
* Dual flow sensing
* Water-level sensing
* Blockage detection
* Multi-level warning system
* Audible warning
* Relay-controlled pump
* Automatic pump protection
* Cumulative critical-level counting
* Blynk cloud monitoring
* Real-time LED status monitoring

The complete sensor, actuator, firmware, and Blynk pipeline has been tested on the physical prototype.

---

## Future Enhancement: AI-Based Prediction

The current version uses deterministic threshold-based blockage detection.

A planned future version will introduce an **AI/ML prediction layer**.

Instead of detecting a blockage only after predefined thresholds are crossed, the system can collect historical sensor data such as:

```text
Timestamp
Flow 1
Flow 2
Flow Difference
Water Level
Rate of Water-Level Change
Pump Status
Blockage Status
```

This dataset can then be used to train a machine-learning model capable of estimating:

```text
Blockage Risk
```

For example:

```text
NORMAL
LOW RISK
MEDIUM RISK
HIGH RISK
CRITICAL
```

The AI layer could eventually provide early warnings before a complete blockage occurs.

### Planned Architecture

```text
Sensors
   ↓
ESP32
   ↓
IoT Data
   ↓
Data Storage
   ↓
Feature Engineering
   ↓
ML Model
   ↓
Blockage Risk Prediction
   ↓
Blynk Dashboard
```

Possible future ML approaches include:

* Random Forest
* Gradient Boosting
* Logistic Regression
* Time-series anomaly detection

The AI component is intentionally kept separate from the currently stable embedded control system so that failure of the prediction layer does not compromise the basic pump-protection mechanism.

---

## Project Goals

The project aims to demonstrate how IoT and machine learning can be combined to create a smarter drainage monitoring system capable of:

1. Monitoring drainage flow in real time
2. Detecting abnormal flow conditions
3. Detecting potential blockages
4. Monitoring water accumulation
5. Providing local visual and audible alerts
6. Automatically protecting the pumping system
7. Providing remote monitoring through Blynk
8. Enabling future AI-based blockage prediction

---

## Future Scope

Potential future improvements include:

* AI-based blockage prediction
* Cloud-based historical data storage
* Mobile notifications
* Multiple drainage-node monitoring
* GPS-based drainage location tracking
* Rainfall data integration
* Predictive maintenance
* Automatic blockage severity estimation
* Dashboard analytics and historical graphs
* Deployment across multiple urban drainage points

---

## Project Status

**Current Status: Working Prototype**

The IoT sensing, blockage detection, warning system, automatic pump protection, and Blynk monitoring components are operational.

**AI/ML prediction:** Planned next phase.

---

## Author

**Dharmaprakash**

B.E. Computer Science and Engineering
KCG College of Technology, Chennai
