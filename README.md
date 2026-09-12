# IoT-Based Drainage Blockage Detection System (ESP32 + Blynk)

This project implements a college-level IoT drainage monitoring prototype using an ESP32, two ZJ-S201 flow sensors, and an AJ-SR04M waterproof ultrasonic sensor.

It detects possible blockages by comparing upstream and downstream flow, tracks water level continuously, controls a pump through a relay, and publishes all key data/alerts to Blynk IoT Cloud.

## 1) Hardware Required

- ESP32 development board
- 2 × ZJ-S201 water flow sensors
  - Upstream sensor (before blockage point)
  - Downstream sensor (after blockage point)
- AJ-SR04M waterproof ultrasonic sensor
- 1-channel relay module (for pump control)
- Drainage pump (low-voltage demo pump recommended for lab prototype)
- Active buzzer
- 4 LEDs (Blue, Green, Yellow, Red) + current-limiting resistors
- Tubing + manually controlled valve (used to simulate blockage)
- Stable power supply for ESP32 + external pump supply

## 2) Project Structure

- `platformio.ini` — PlatformIO environment setup for ESP32 Arduino
- `include/config.h` — pin mappings and system thresholds/constants
- `include/secrets.h` — Wi-Fi/Blynk credentials (edit before upload)
- `src/main.cpp` — firmware logic (sensing, decision, actuator control, cloud update)

## 3) Wiring (Suggested Pin Map)

Defined in `include/config.h`:

- Flow sensor upstream pulse pin: `GPIO34`
- Flow sensor downstream pulse pin: `GPIO35`
- Ultrasonic TRIG: `GPIO5`
- Ultrasonic ECHO: `GPIO18`
- Blue LED: `GPIO2`
- Green LED: `GPIO15`
- Yellow LED: `GPIO4`
- Red LED: `GPIO16`
- Active buzzer: `GPIO17`
- Relay (pump control): `GPIO19`

> Use a common ground between ESP32, sensors, relay input ground, and any external interface electronics.

## 4) Detection and Control Logic

### Blockage Detection

The system calculates both flow rates in L/min every second and computes:

`downstream_ratio = downstream_flow / upstream_flow`

Blockage is flagged when:
- upstream flow is above a minimum valid threshold, and
- downstream ratio drops below the configured blockage ratio threshold.

### Water Level Monitoring

Ultrasonic distance is mapped to water-level percentage:
- larger distance = lower water level
- smaller distance = higher water level

Level states:
- **Normal**
- **Warning** (high but not critical)
- **Critical** (overflow risk)

### Pump and Safety

- Relay controls pump ON/OFF.
- Pump can be commanded from Blynk (`V6`).
- If water level reaches critical threshold, pump is automatically forced OFF for safety and Blynk switch state is reset.

### LED and Buzzer Indication

- **Blue LED**: cloud connection status indicator
- **Green LED**: normal condition
- **Yellow LED**: warning/blockage condition
- **Red LED**: critical water level condition
- **Buzzer**: ON during critical or blockage alert states

## 5) Blynk Template Setup

Create a Blynk template and add datastreams:

- `V0` (Double) — Upstream flow rate (L/min)
- `V1` (Double) — Downstream flow rate (L/min)
- `V2` (Double) — Water level (%)
- `V3` (Integer) — Blockage status (0/1)
- `V4` (String) — Alert text
- `V5` (String) — Pump status text (`ON` / `OFF`)
- `V6` (Integer, switch) — Pump command (0/1)

Then copy `BLYNK_TEMPLATE_ID`, `BLYNK_TEMPLATE_NAME`, and `BLYNK_AUTH_TOKEN` into `include/secrets.h`.

## 6) Configuration

Edit values in `include/config.h` to match your setup:

- Flow calibration factor for ZJ-S201
- Empty/full ultrasonic distances
- Warning and critical water-level thresholds
- Blockage ratio threshold

## 7) Build and Upload (PlatformIO)

From repository root:

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## 8) Simulating Blockage for Demo

1. Keep pump and flow path running with valve open.
2. Gradually close manual valve at the blockage point.
3. Observe downstream flow drop while upstream flow remains higher.
4. Confirm yellow/red indicators, buzzer behavior, and Blynk alert updates.

This setup is intentionally simple and practical for an engineering college project demonstration.
