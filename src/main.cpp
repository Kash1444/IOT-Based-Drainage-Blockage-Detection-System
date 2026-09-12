#include <Arduino.h>
#include <WiFi.h>

#include <BlynkSimpleEsp32.h>

#include "config.h"
#include "secrets.h"

namespace {
volatile uint32_t upstreamPulses = 0;
volatile uint32_t downstreamPulses = 0;

float upstreamFlowLpm = 0.0F;
float downstreamFlowLpm = 0.0F;
float waterLevelPercent = 0.0F;
bool blockageDetected = false;
bool criticalLevel = false;
bool pumpCommand = false;
bool pumpRunning = false;
String currentAlert = "System starting...";

BlynkTimer timer;

void IRAM_ATTR onUpstreamPulse() { upstreamPulses++; }
void IRAM_ATTR onDownstreamPulse() { downstreamPulses++; }

float mapDistanceToLevelPercent(float distanceCm) {
  const float usableRange = Config::ULTRASONIC_EMPTY_DISTANCE_CM - Config::ULTRASONIC_FULL_DISTANCE_CM;
  if (usableRange <= 0.0F) {
    return 0.0F;
  }

  const float constrainedDistance =
      constrain(distanceCm, Config::ULTRASONIC_FULL_DISTANCE_CM, Config::ULTRASONIC_EMPTY_DISTANCE_CM);
  const float fillRatio = (Config::ULTRASONIC_EMPTY_DISTANCE_CM - constrainedDistance) / usableRange;
  return constrain(fillRatio * 100.0F, 0.0F, 100.0F);
}

float readUltrasonicDistanceCm() {
  digitalWrite(Config::ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(Config::ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(Config::ULTRASONIC_TRIG_PIN, LOW);

  const unsigned long durationUs = pulseIn(Config::ULTRASONIC_ECHO_PIN, HIGH, 30000);
  if (durationUs == 0) {
    return Config::ULTRASONIC_EMPTY_DISTANCE_CM;
  }

  return static_cast<float>(durationUs) * 0.0343F * 0.5F;
}

void setRelayState(bool enabled) {
  digitalWrite(Config::RELAY_PIN, (enabled ^ Config::RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

void setLedStates(bool blue, bool green, bool yellow, bool red) {
  digitalWrite(Config::LED_BLUE_PIN, blue ? HIGH : LOW);
  digitalWrite(Config::LED_GREEN_PIN, green ? HIGH : LOW);
  digitalWrite(Config::LED_YELLOW_PIN, yellow ? HIGH : LOW);
  digitalWrite(Config::LED_RED_PIN, red ? HIGH : LOW);
}

void updateFlowRates() {
  static unsigned long lastUpdateMs = millis();
  const unsigned long nowMs = millis();
  const unsigned long elapsedMs = nowMs - lastUpdateMs;
  if (elapsedMs == 0) {
    return;
  }

  noInterrupts();
  const uint32_t upstreamPulseCount = upstreamPulses;
  const uint32_t downstreamPulseCount = downstreamPulses;
  upstreamPulses = 0;
  downstreamPulses = 0;
  interrupts();

  const float elapsedSeconds = static_cast<float>(elapsedMs) / 1000.0F;
  upstreamFlowLpm = (static_cast<float>(upstreamPulseCount) / elapsedSeconds) / Config::FLOW_CALIBRATION_FACTOR;
  downstreamFlowLpm = (static_cast<float>(downstreamPulseCount) / elapsedSeconds) / Config::FLOW_CALIBRATION_FACTOR;

  lastUpdateMs = nowMs;
}

void evaluateSystemState() {
  const float downstreamRatio = (upstreamFlowLpm > 0.0F) ? (downstreamFlowLpm / upstreamFlowLpm) : 1.0F;
  blockageDetected = upstreamFlowLpm >= Config::MIN_FLOW_FOR_BLOCKAGE_CHECK_LPM &&
                     downstreamRatio < Config::BLOCKAGE_RATIO_THRESHOLD;

  criticalLevel = waterLevelPercent >= Config::CRITICAL_WATER_LEVEL_PERCENT;

  if (criticalLevel && pumpCommand) {
    pumpCommand = false;
    Blynk.virtualWrite(V6, 0);
  }

  pumpRunning = pumpCommand && !criticalLevel;
  setRelayState(pumpRunning);

  if (criticalLevel && blockageDetected) {
    currentAlert = "CRITICAL: high level + blockage. Pump forced OFF.";
  } else if (criticalLevel) {
    currentAlert = "CRITICAL: water level reached threshold. Pump forced OFF.";
  } else if (blockageDetected) {
    currentAlert = "WARNING: possible blockage detected.";
  } else if (waterLevelPercent >= Config::WARNING_WATER_LEVEL_PERCENT) {
    currentAlert = "Warning: water level rising.";
  } else {
    currentAlert = "Normal operation.";
  }
}

void updateIndicators() {
  const bool blynkConnected = Blynk.connected();
  const bool warningLevel = waterLevelPercent >= Config::WARNING_WATER_LEVEL_PERCENT;

  if (criticalLevel) {
    setLedStates(blynkConnected, false, false, true);
  } else if (blockageDetected || warningLevel) {
    setLedStates(blynkConnected, false, true, false);
  } else {
    setLedStates(blynkConnected, true, false, false);
  }

  digitalWrite(Config::BUZZER_PIN, (criticalLevel || blockageDetected) ? HIGH : LOW);
}

void publishToBlynk() {
  Blynk.virtualWrite(V0, upstreamFlowLpm);
  Blynk.virtualWrite(V1, downstreamFlowLpm);
  Blynk.virtualWrite(V2, waterLevelPercent);
  Blynk.virtualWrite(V3, blockageDetected ? 1 : 0);
  Blynk.virtualWrite(V4, currentAlert);
  Blynk.virtualWrite(V5, pumpRunning ? "ON" : "OFF");
}

void updateSensorsAndControl() {
  updateFlowRates();
  waterLevelPercent = mapDistanceToLevelPercent(readUltrasonicDistanceCm());
  evaluateSystemState();
  updateIndicators();
}
}  // namespace

BLYNK_CONNECTED() { Blynk.syncVirtual(V6); }

BLYNK_WRITE(V6) { pumpCommand = param.asInt() == 1; }

void setup() {
  Serial.begin(115200);

  pinMode(Config::FLOW_SENSOR_UPSTREAM_PIN, INPUT_PULLUP);
  pinMode(Config::FLOW_SENSOR_DOWNSTREAM_PIN, INPUT_PULLUP);

  pinMode(Config::ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(Config::ULTRASONIC_ECHO_PIN, INPUT);

  pinMode(Config::LED_BLUE_PIN, OUTPUT);
  pinMode(Config::LED_GREEN_PIN, OUTPUT);
  pinMode(Config::LED_YELLOW_PIN, OUTPUT);
  pinMode(Config::LED_RED_PIN, OUTPUT);
  pinMode(Config::BUZZER_PIN, OUTPUT);
  pinMode(Config::RELAY_PIN, OUTPUT);

  setRelayState(false);
  setLedStates(false, false, false, false);
  digitalWrite(Config::BUZZER_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(Config::FLOW_SENSOR_UPSTREAM_PIN), onUpstreamPulse, RISING);
  attachInterrupt(digitalPinToInterrupt(Config::FLOW_SENSOR_DOWNSTREAM_PIN), onDownstreamPulse, RISING);

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);

  timer.setInterval(Config::SENSOR_UPDATE_INTERVAL_MS, updateSensorsAndControl);
  timer.setInterval(Config::BLYNK_PUSH_INTERVAL_MS, publishToBlynk);
}

void loop() {
  Blynk.run();
  timer.run();
}

