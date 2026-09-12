#pragma once

#include <Arduino.h>

namespace Config {
constexpr uint8_t FLOW_SENSOR_UPSTREAM_PIN = 34;
constexpr uint8_t FLOW_SENSOR_DOWNSTREAM_PIN = 35;

constexpr uint8_t ULTRASONIC_TRIG_PIN = 5;
constexpr uint8_t ULTRASONIC_ECHO_PIN = 18;

constexpr uint8_t LED_BLUE_PIN = 2;
constexpr uint8_t LED_GREEN_PIN = 15;
constexpr uint8_t LED_YELLOW_PIN = 4;
constexpr uint8_t LED_RED_PIN = 16;
constexpr uint8_t BUZZER_PIN = 17;
constexpr uint8_t RELAY_PIN = 19;

constexpr bool RELAY_ACTIVE_LOW = true;

constexpr float FLOW_CALIBRATION_FACTOR = 7.5F;
constexpr float MIN_FLOW_FOR_BLOCKAGE_CHECK_LPM = 1.0F;
constexpr float BLOCKAGE_RATIO_THRESHOLD = 0.55F;

constexpr float ULTRASONIC_EMPTY_DISTANCE_CM = 30.0F;
constexpr float ULTRASONIC_FULL_DISTANCE_CM = 5.0F;
constexpr float WARNING_WATER_LEVEL_PERCENT = 70.0F;
constexpr float CRITICAL_WATER_LEVEL_PERCENT = 90.0F;

constexpr unsigned long SENSOR_UPDATE_INTERVAL_MS = 1000;
constexpr unsigned long BLYNK_PUSH_INTERVAL_MS = 2000;
}  // namespace Config

