// ============================================================
// ESP32 DRAINAGE BLOCKAGE DETECTION SYSTEM
// Blynk IoT + Water Level + 2 Flow Sensors
// LEDs + Buzzer + Relay + Automatic Pump Protection
// + Blynk Manual Pump Control
// ============================================================

// ============================================================
// BLYNK CONFIGURATION
// ============================================================

#define BLYNK_TEMPLATE_ID "TMPL3zJG9PmKu"
#define BLYNK_TEMPLATE_NAME "IoT based Drainage Block Detection"

// IMPORTANT:
// Put your Blynk Auth Token here.
// Do NOT share this token publicly.
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// -------------------- WIFI --------------------

// Put your existing Wi-Fi credentials here.
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// Blynk timer
BlynkTimer timer;


// ============================================================
// ULTRASONIC
// ============================================================

const int TRIG_PIN = 27;
const int ECHO_PIN = 26;


// ============================================================
// LEDs
// ============================================================

const int BLUE_LED   = 25;
const int GREEN_LED  = 19;
const int YELLOW_LED = 21;
const int RED_LED    = 18;


// ============================================================
// BLYNK LED VIRTUAL PINS
// ============================================================

#define BLYNK_BLUE_LED   V8
#define BLYNK_GREEN_LED  V9
#define BLYNK_YELLOW_LED V10
#define BLYNK_RED_LED    V11


// ============================================================
// BLYNK PUMP CONTROL
// ============================================================

// V12 = Manual Pump Control
// 0 = Pump OFF
// 1 = Pump ON

#define BLYNK_PUMP_CONTROL V12


// ============================================================
// BUZZER
// ============================================================

const int BUZZER = 22;


// ============================================================
// FLOW SENSOR PINS
// ============================================================

const int FLOW_SENSOR_1 = 32;
const int FLOW_SENSOR_2 = 33;


// ============================================================
// RELAY
// ============================================================

const int RELAY_PIN = 23;

// Relay:
// LOW  = Pump ON
// HIGH = Pump OFF


// ============================================================
// ULTRASONIC CALIBRATION
// ============================================================

const float EMPTY_DISTANCE = 27.75;
const float FULL_DISTANCE  = 20.49;


// ============================================================
// FLOW SENSOR VARIABLES
// ============================================================

volatile unsigned long pulseCount1 = 0;
volatile unsigned long pulseCount2 = 0;

unsigned long flow1Pulses = 0;
unsigned long flow2Pulses = 0;

int flowDifference = 0;


// ============================================================
// BLOCKAGE DETECTION
// ============================================================

const int BLOCKAGE_DIFFERENCE = 15;
const int BLOCKAGE_CONFIRM_TIME = 3;

int blockageSeconds = 0;
bool blockageDetected = false;


// ============================================================
// FULL LEVEL PUMP SHUTDOWN
// ============================================================

const int FULL_LEVEL_CONFIRM_COUNT = 20;

int fullLevelCount = 0;

bool pumpShutdown = false;


// ============================================================
// MANUAL PUMP CONTROL
// ============================================================

// false = pump OFF
// true  = pump ON

bool manualPumpRequest = false;


// ============================================================
// WATER LEVEL VARIABLE
// ============================================================

float currentWaterLevel = 0.0;
float currentDistance = 0.0;


// ============================================================
// TIMERS
// ============================================================

unsigned long previousFlowMillis = 0;
unsigned long previousLedMillis = 0;
unsigned long previousBuzzerMillis = 0;
unsigned long previousPrintMillis = 0;
unsigned long previousFullCheckMillis = 0;


// ============================================================
// SERIAL OUTPUT
// ============================================================

const unsigned long PRINT_INTERVAL = 500;


// ============================================================
// LED / BUZZER STATES
// ============================================================

bool redBlinkState = false;
bool emergencyBlinkState = false;
bool buzzerState = false;


// ============================================================
// BLYNK LED STATE VARIABLES
// ============================================================

bool blueLedState = false;
bool greenLedState = false;
bool yellowLedState = false;
bool redLedState = false;


// ============================================================
// SET LED + UPDATE BLYNK
// ============================================================

void setLed(
  int physicalPin,
  bool state,
  int blynkPin,
  bool &storedState
) {

  // Only do something if the LED state actually changed
  if (storedState != state) {

    storedState = state;

    digitalWrite(
      physicalPin,
      state ? HIGH : LOW
    );

    // Send the new LED state to Blynk
    Blynk.virtualWrite(
      blynkPin,
      state ? 1 : 0
    );
  }
}


// ============================================================
// FLOW SENSOR INTERRUPTS
// ============================================================

void IRAM_ATTR pulseCounter1() {
  pulseCount1++;
}

void IRAM_ATTR pulseCounter2() {
  pulseCount2++;
}


// ============================================================
// ACTUAL PUMP STATE
// ============================================================

bool isPumpOn() {

  // Relay LOW = Pump ON
  return digitalRead(RELAY_PIN) == LOW;
}


// ============================================================
// UPDATE PUMP LED
// ============================================================

void updatePumpLED() {

  bool pumpState = isPumpOn();

  setLed(
    BLUE_LED,
    pumpState,
    BLYNK_BLUE_LED,
    blueLedState
  );
}


// ============================================================
// UPDATE PUMP STATUS ON BLYNK
// ============================================================

void updatePumpStatus() {

  // ----------------------------------------------------------
  // SAFETY SHUTDOWN HAS HIGHEST PRIORITY
  // ----------------------------------------------------------

  if (pumpShutdown) {

    Blynk.virtualWrite(
      V5,
      "OFF - SAFETY SHUTDOWN"
    );

    return;
  }


  // ----------------------------------------------------------
  // ACTUAL RELAY / PUMP STATE
  // ----------------------------------------------------------

  if (isPumpOn()) {

    Blynk.virtualWrite(
      V5,
      "ON - MANUAL"
    );

  }

  else {

    Blynk.virtualWrite(
      V5,
      "OFF - MANUAL"
    );
  }
}


// ============================================================
// UPDATE PUMP CONTROL
// ============================================================

void updatePumpControl() {

  // ==========================================================
  // SAFETY OVERRIDE
  // ==========================================================

  // If automatic safety shutdown is active,
  // the phone cannot turn the pump back ON.

  if (pumpShutdown) {

    manualPumpRequest = false;

    digitalWrite(
      RELAY_PIN,
      HIGH
    );

    updatePumpLED();

    return;
  }


  // ==========================================================
  // MANUAL BLYNK CONTROL
  // ==========================================================

  if (manualPumpRequest) {

    // Pump ON
    digitalWrite(
      RELAY_PIN,
      LOW
    );

  }

  else {

    // Pump OFF
    digitalWrite(
      RELAY_PIN,
      HIGH
    );
  }


  // Update blue LED
  updatePumpLED();
}


// ============================================================
// BLYNK PUMP BUTTON
// ============================================================

BLYNK_WRITE(BLYNK_PUMP_CONTROL)
{

  int value = param.asInt();


  // ==========================================================
  // SAFETY SHUTDOWN ACTIVE
  // ==========================================================

  if (pumpShutdown) {

    manualPumpRequest = false;

    // Force relay OFF
    digitalWrite(
      RELAY_PIN,
      HIGH
    );


    // Force Blynk switch OFF
    Blynk.virtualWrite(
      BLYNK_PUMP_CONTROL,
      0
    );


    Serial.println();
    Serial.println(
      "BLYNK PUMP REQUEST IGNORED"
    );

    Serial.println(
      "Reason: SAFETY SHUTDOWN ACTIVE"
    );

    Serial.println();


    updatePumpLED();

    return;
  }


  // ==========================================================
  // STORE MANUAL REQUEST
  // ==========================================================

  if (value == 1) {

    manualPumpRequest = true;

    Serial.println();
    Serial.println(
      "BLYNK COMMAND: PUMP ON"
    );

    Serial.println(
      "Pump control mode: MANUAL"
    );

  }

  else {

    manualPumpRequest = false;

    Serial.println();
    Serial.println(
      "BLYNK COMMAND: PUMP OFF"
    );

    Serial.println(
      "Pump control mode: MANUAL"
    );
  }


  // Apply command
  updatePumpControl();


  // Update Blynk status
  updatePumpStatus();

}


// ============================================================
// SEND DATA TO BLYNK
// ============================================================

void sendDataToBlynk() {

  // ----------------------------------------------------------
  // FLOW SENSOR DATA
  // ----------------------------------------------------------

  Blynk.virtualWrite(
    V0,
    flow1Pulses
  );

  Blynk.virtualWrite(
    V1,
    flow2Pulses
  );


  // ----------------------------------------------------------
  // WATER LEVEL
  // ----------------------------------------------------------

  Blynk.virtualWrite(
    V2,
    currentWaterLevel
  );


  // ----------------------------------------------------------
  // FLOW DIFFERENCE
  // ----------------------------------------------------------

  Blynk.virtualWrite(
    V3,
    flowDifference
  );


  // ----------------------------------------------------------
  // BLOCKAGE STATUS
  // ----------------------------------------------------------

  if (blockageDetected) {

    Blynk.virtualWrite(
      V4,
      "BLOCKAGE DETECTED"
    );

  }

  else {

    Blynk.virtualWrite(
      V4,
      "NORMAL"
    );
  }


  // ----------------------------------------------------------
  // PUMP STATUS
  // ----------------------------------------------------------

  updatePumpStatus();


  // ----------------------------------------------------------
  // FULL LEVEL COUNT
  // ----------------------------------------------------------

  Blynk.virtualWrite(
    V6,
    fullLevelCount
  );


  // ----------------------------------------------------------
  // WATER LEVEL STATUS
  // ----------------------------------------------------------

  if (pumpShutdown) {

    Blynk.virtualWrite(
      V7,
      "CRITICAL - PUMP SHUTDOWN"
    );

  }

  else if (currentWaterLevel >= 99.0) {

    Blynk.virtualWrite(
      V7,
      "CRITICAL"
    );

  }

  else if (currentWaterLevel >= 90.0) {

    Blynk.virtualWrite(
      V7,
      "VERY HIGH"
    );

  }

  else if (currentWaterLevel >= 80.0) {

    Blynk.virtualWrite(
      V7,
      "HIGH"
    );

  }

  else if (currentWaterLevel >= 60.0) {

    Blynk.virtualWrite(
      V7,
      "MEDIUM"
    );

  }

  else if (currentWaterLevel >= 30.0) {

    Blynk.virtualWrite(
      V7,
      "LOW-MEDIUM"
    );

  }

  else {

    Blynk.virtualWrite(
      V7,
      "NORMAL"
    );
  }


  // ----------------------------------------------------------
  // LED STATES
  // ----------------------------------------------------------

  Blynk.virtualWrite(
    BLYNK_BLUE_LED,
    blueLedState ? 1 : 0
  );

  Blynk.virtualWrite(
    BLYNK_GREEN_LED,
    greenLedState ? 1 : 0
  );

  Blynk.virtualWrite(
    BLYNK_YELLOW_LED,
    yellowLedState ? 1 : 0
  );

  Blynk.virtualWrite(
    BLYNK_RED_LED,
    redLedState ? 1 : 0
  );


  // ----------------------------------------------------------
  // PUMP CONTROL STATE
  // ----------------------------------------------------------

  // Keep V12 synchronized with the actual requested state.
  // During safety shutdown it is forced OFF.

  if (pumpShutdown) {

    Blynk.virtualWrite(
      BLYNK_PUMP_CONTROL,
      0
    );

  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);


  // ==========================================================
  // ULTRASONIC
  // ==========================================================

  pinMode(
    TRIG_PIN,
    OUTPUT
  );

  pinMode(
    ECHO_PIN,
    INPUT
  );


  // ==========================================================
  // LEDs
  // ==========================================================

  pinMode(
    BLUE_LED,
    OUTPUT
  );

  pinMode(
    GREEN_LED,
    OUTPUT
  );

  pinMode(
    YELLOW_LED,
    OUTPUT
  );

  pinMode(
    RED_LED,
    OUTPUT
  );


  // ==========================================================
  // BUZZER
  // ==========================================================

  pinMode(
    BUZZER,
    OUTPUT
  );


  // ==========================================================
  // FLOW SENSORS
  // ==========================================================

  pinMode(
    FLOW_SENSOR_1,
    INPUT_PULLUP
  );

  pinMode(
    FLOW_SENSOR_2,
    INPUT_PULLUP
  );


  // ==========================================================
  // RELAY
  // ==========================================================

  pinMode(
    RELAY_PIN,
    OUTPUT
  );


  // ==========================================================
  // INITIAL PUMP STATE
  // ==========================================================

  // IMPORTANT:
  // Pump starts OFF.
  //
  // The Blynk V12 switch will determine whether
  // the pump should be ON or OFF.

  manualPumpRequest = false;

  digitalWrite(
    RELAY_PIN,
    HIGH
  );


  // ----------------------------------------------------------
  // Blue LED = Pump OFF initially
  // ----------------------------------------------------------

  blueLedState = false;

  digitalWrite(
    BLUE_LED,
    LOW
  );


  // ----------------------------------------------------------
  // Other LEDs OFF
  // ----------------------------------------------------------

  greenLedState = false;
  yellowLedState = false;
  redLedState = false;

  digitalWrite(
    GREEN_LED,
    LOW
  );

  digitalWrite(
    YELLOW_LED,
    LOW
  );

  digitalWrite(
    RED_LED,
    LOW
  );


  // ----------------------------------------------------------
  // Buzzer OFF
  // ----------------------------------------------------------

  digitalWrite(
    BUZZER,
    HIGH
  );


  // ----------------------------------------------------------
  // Ultrasonic trigger LOW
  // ----------------------------------------------------------

  digitalWrite(
    TRIG_PIN,
    LOW
  );


  // ==========================================================
  // FLOW SENSOR INTERRUPTS
  // ==========================================================

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_1),
    pulseCounter1,
    RISING
  );

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR_2),
    pulseCounter2,
    RISING
  );


  // ==========================================================
  // CONNECT TO BLYNK
  // ==========================================================

  Serial.println();
  Serial.println(
    "Connecting to WiFi and Blynk..."
  );

  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );


  // ==========================================================
  // SYNCHRONIZE BLYNK PUMP SWITCH
  // ==========================================================

  // Ask Blynk server for the current V12 value.
  //
  // BLYNK_WRITE(V12) will automatically be called
  // when the value is received.

  Blynk.syncVirtual(
    BLYNK_PUMP_CONTROL
  );


  // ==========================================================
  // SEND DATA TO BLYNK EVERY 1 SECOND
  // ==========================================================

  timer.setInterval(
    1000L,
    sendDataToBlynk
  );


  // ==========================================================
  // STARTUP MESSAGE
  // ==========================================================

  delay(1000);

  Serial.println();
  Serial.println(
    "=============================================================="
  );

  Serial.println(
    " ESP32 DRAINAGE MONITORING SYSTEM"
  );

  Serial.println(
    "=============================================================="
  );

  Serial.println(
    "Water Level + Flow Sensors + Blockage Detection"
  );

  Serial.println(
    "Automatic Pump Protection Enabled"
  );

  Serial.println(
    "Blynk IoT Monitoring Enabled"
  );

  Serial.println(
    "4 LED Blynk Monitoring Enabled"
  );

  Serial.println(
    "Blynk Manual Pump Control Enabled"
  );

  Serial.println(
    "=============================================================="
  );

  Serial.println(
    "Pump: Waiting for Blynk Control"
  );

  Serial.println(
    "V12: Manual Pump Control"
  );

  Serial.println(
    "Blue LED: Actual Pump Status"
  );

  Serial.println(
    "Full-Level Shutdown: 20 cumulative confirmations"
  );

  Serial.println(
    "=============================================================="
  );

  Serial.println();
}


// ============================================================
// MAIN LOOP
// ============================================================

void loop() {

  unsigned long currentMillis = millis();


  // ==========================================================
  // BLYNK
  // ==========================================================

  Blynk.run();
  timer.run();


  // ==========================================================
  // UPDATE FLOW SENSOR VALUES EVERY 1 SECOND
  // ==========================================================

  if (
    currentMillis -
    previousFlowMillis >=
    1000
  ) {

    previousFlowMillis =
      currentMillis;

    noInterrupts();

    flow1Pulses =
      pulseCount1;

    flow2Pulses =
      pulseCount2;

    pulseCount1 = 0;
    pulseCount2 = 0;

    interrupts();


    // --------------------------------------------------------
    // SAFE FLOW DIFFERENCE CALCULATION
    // --------------------------------------------------------

    if (
      flow1Pulses >=
      flow2Pulses
    ) {

      flowDifference =
        flow1Pulses -
        flow2Pulses;

    }

    else {

      flowDifference =
        -(int)(
          flow2Pulses -
          flow1Pulses
        );
    }


    // --------------------------------------------------------
    // BLOCKAGE DETECTION
    // --------------------------------------------------------

    if (
      flow1Pulses >= 20 &&
      flowDifference >=
      BLOCKAGE_DIFFERENCE
    ) {

      blockageSeconds++;


      if (
        blockageSeconds >=
        BLOCKAGE_CONFIRM_TIME
      ) {

        blockageDetected =
          true;
      }

    }

    else {

      blockageSeconds = 0;
      blockageDetected = false;
    }
  }


  // ==========================================================
  // SEND ULTRASONIC PULSE
  // ==========================================================

  digitalWrite(
    TRIG_PIN,
    LOW
  );

  delayMicroseconds(2);

  digitalWrite(
    TRIG_PIN,
    HIGH
  );

  delayMicroseconds(10);

  digitalWrite(
    TRIG_PIN,
    LOW
  );


  // ==========================================================
  // READ ULTRASONIC ECHO
  // ==========================================================

  long duration =
    pulseIn(
      ECHO_PIN,
      HIGH,
      30000
    );


  // ==========================================================
  // NO ECHO
  // ==========================================================

  if (
    duration == 0
  ) {

    currentWaterLevel =
      0.0;


    // Warning LEDs OFF

    setLed(
      GREEN_LED,
      false,
      BLYNK_GREEN_LED,
      greenLedState
    );

    setLed(
      YELLOW_LED,
      false,
      BLYNK_YELLOW_LED,
      yellowLedState
    );

    setLed(
      RED_LED,
      false,
      BLYNK_RED_LED,
      redLedState
    );


    // Buzzer OFF

    digitalWrite(
      BUZZER,
      HIGH
    );


    // --------------------------------------------------------
    // Blue LED = actual pump status
    // --------------------------------------------------------

    updatePumpLED();


    // --------------------------------------------------------
    // SERIAL OUTPUT
    // --------------------------------------------------------

    if (
      currentMillis -
      previousPrintMillis >=
      PRINT_INTERVAL
    ) {

      previousPrintMillis =
        currentMillis;

      Serial.print(
        "No echo detected"
      );

      Serial.print(
        " | Flow 1: "
      );

      Serial.print(
        flow1Pulses
      );

      Serial.print(
        " pulses/sec | Flow 2: "
      );

      Serial.print(
        flow2Pulses
      );

      Serial.print(
        " pulses/sec | Difference: "
      );

      Serial.print(
        flowDifference
      );

      Serial.print(
        " | Blockage: "
      );

      if (blockageDetected) {

        Serial.print(
          "DETECTED"
        );

      }

      else {

        Serial.print(
          "NORMAL"
        );
      }

      Serial.print(
        " | Full Count: "
      );

      Serial.print(
        fullLevelCount
      );

      Serial.print(
        "/20"
      );

      Serial.print(
        " | Pump: "
      );

      if (pumpShutdown) {

        Serial.println(
          "OFF - SAFETY SHUTDOWN"
        );

      }

      else if (isPumpOn()) {

        Serial.println(
          "ON - MANUAL"
        );

      }

      else {

        Serial.println(
          "OFF - MANUAL"
        );
      }
    }
  }


  // ==========================================================
  // VALID ULTRASONIC READING
  // ==========================================================

  else {

    float distance =
      duration *
      0.0343 /
      2.0;


    float waterLevel =
      (
        (
          EMPTY_DISTANCE -
          distance
        )
        /
        (
          EMPTY_DISTANCE -
          FULL_DISTANCE
        )
      )
      *
      100.0;


    waterLevel =
      constrain(
        waterLevel,
        0.0,
        100.0
      );


    // --------------------------------------------------------
    // Save values for Blynk
    // --------------------------------------------------------

    currentDistance =
      distance;

    currentWaterLevel =
      waterLevel;


    // ========================================================
    // CUMULATIVE FULL LEVEL COUNT
    // ========================================================

    if (
      !pumpShutdown &&
      currentMillis -
      previousFullCheckMillis >=
      1000
    ) {

      previousFullCheckMillis =
        currentMillis;


      // 99% or above = FULL

      if (
        waterLevel >=
        99.0
      ) {

        // IMPORTANT:
        // Count is cumulative.
        // It NEVER resets when water level drops.

        fullLevelCount++;


        Serial.print(
          "FULL LEVEL CONFIRMATION: "
        );

        Serial.print(
          fullLevelCount
        );

        Serial.println(
          "/20"
        );


        // ----------------------------------------------------
        // 20 TOTAL CONFIRMATIONS
        // ----------------------------------------------------

        if (
          fullLevelCount >=
          FULL_LEVEL_CONFIRM_COUNT
        ) {

          // ==================================================
          // SAFETY SHUTDOWN
          // ==================================================

          pumpShutdown =
            true;


          // Manual request cancelled

          manualPumpRequest =
            false;


          // --------------------------------------------------
          // Pump OFF
          // --------------------------------------------------

          digitalWrite(
            RELAY_PIN,
            HIGH
          );


          // --------------------------------------------------
          // Force Blynk pump switch OFF
          // --------------------------------------------------

          Blynk.virtualWrite(
            BLYNK_PUMP_CONTROL,
            0
          );


          // --------------------------------------------------
          // All LEDs OFF
          // --------------------------------------------------

          setLed(
            BLUE_LED,
            false,
            BLYNK_BLUE_LED,
            blueLedState
          );

          setLed(
            GREEN_LED,
            false,
            BLYNK_GREEN_LED,
            greenLedState
          );

          setLed(
            YELLOW_LED,
            false,
            BLYNK_YELLOW_LED,
            yellowLedState
          );

          setLed(
            RED_LED,
            false,
            BLYNK_RED_LED,
            redLedState
          );


          // --------------------------------------------------
          // Buzzer OFF
          // --------------------------------------------------

          digitalWrite(
            BUZZER,
            HIGH
          );


          Serial.println();

          Serial.println(
            "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
          );

          Serial.println(
            " EMERGENCY WATER LEVEL CONDITION"
          );

          Serial.println(
            " 20 FULL CONFIRMATIONS COMPLETED"
          );

          Serial.println(
            " PUMP SHUT DOWN"
          );

          Serial.println(
            " BLYNK PUMP CONTROL DISABLED"
          );

          Serial.println(
            " BUZZER OFF"
          );

          Serial.println(
            " BLUE LED OFF"
          );

          Serial.println(
            " SYSTEM SHUTDOWN LATCHED"
          );

          Serial.println(
            "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
          );

          Serial.println();
        }
      }

      // NO RESET HERE.
      // fullLevelCount stays cumulative.
    }


    // ========================================================
    // NORMAL LED / BUZZER SYSTEM
    //
    // This operates only while the pump is actually ON.
    // ========================================================

    if (
      !pumpShutdown &&
      isPumpOn()
    ) {


      // ------------------------------------------------------
      // BLUE LED = ACTUAL PUMP STATUS
      // ------------------------------------------------------

      setLed(
        BLUE_LED,
        true,
        BLYNK_BLUE_LED,
        blueLedState
      );


      // ------------------------------------------------------
      // 0–30% → GREEN
      // ------------------------------------------------------

      if (
        waterLevel <= 30
      ) {

        setLed(
          GREEN_LED,
          true,
          BLYNK_GREEN_LED,
          greenLedState
        );

        setLed(
          YELLOW_LED,
          false,
          BLYNK_YELLOW_LED,
          yellowLedState
        );

        setLed(
          RED_LED,
          false,
          BLYNK_RED_LED,
          redLedState
        );

        digitalWrite(
          BUZZER,
          HIGH
        );

        buzzerState =
          false;
      }


      // ------------------------------------------------------
      // 30–60% → YELLOW + SLOW BEEP
      // ------------------------------------------------------

      else if (
        waterLevel <= 60
      ) {

        setLed(
          GREEN_LED,
          false,
          BLYNK_GREEN_LED,
          greenLedState
        );

        setLed(
          YELLOW_LED,
          true,
          BLYNK_YELLOW_LED,
          yellowLedState
        );

        setLed(
          RED_LED,
          false,
          BLYNK_RED_LED,
          redLedState
        );


        if (
          currentMillis -
          previousBuzzerMillis >=
          1000
        ) {

          previousBuzzerMillis =
            currentMillis;

          buzzerState =
            !buzzerState;


          if (buzzerState) {

            digitalWrite(
              BUZZER,
              LOW
            );

          }

          else {

            digitalWrite(
              BUZZER,
              HIGH
            );
          }
        }
      }


      // ------------------------------------------------------
      // 60–80% → RED + FASTER BEEP
      // ------------------------------------------------------

      else if (
        waterLevel <= 80
      ) {

        setLed(
          GREEN_LED,
          false,
          BLYNK_GREEN_LED,
          greenLedState
        );

        setLed(
          YELLOW_LED,
          false,
          BLYNK_YELLOW_LED,
          yellowLedState
        );

        setLed(
          RED_LED,
          true,
          BLYNK_RED_LED,
          redLedState
        );


        if (
          currentMillis -
          previousBuzzerMillis >=
          400
        ) {

          previousBuzzerMillis =
            currentMillis;

          buzzerState =
            !buzzerState;


          if (buzzerState) {

            digitalWrite(
              BUZZER,
              LOW
            );

          }

          else {

            digitalWrite(
              BUZZER,
              HIGH
            );
          }
        }
      }


      // ------------------------------------------------------
      // 80–90% → RAPID RED + RAPID BEEP
      // ------------------------------------------------------

      else if (
        waterLevel <= 90
      ) {

        setLed(
          GREEN_LED,
          false,
          BLYNK_GREEN_LED,
          greenLedState
        );

        setLed(
          YELLOW_LED,
          false,
          BLYNK_YELLOW_LED,
          yellowLedState
        );


        if (
          currentMillis -
          previousLedMillis >=
          150
        ) {

          previousLedMillis =
            currentMillis;

          redBlinkState =
            !redBlinkState;


          setLed(
            RED_LED,
            redBlinkState,
            BLYNK_RED_LED,
            redLedState
          );
        }


        if (
          currentMillis -
          previousBuzzerMillis >=
          200
        ) {

          previousBuzzerMillis =
            currentMillis;

          buzzerState =
            !buzzerState;


          if (buzzerState) {

            digitalWrite(
              BUZZER,
              LOW
            );

          }

          else {

            digitalWrite(
              BUZZER,
              HIGH
            );
          }
        }
      }


      // ------------------------------------------------------
      // 90–99% → ALL LEDs + RAPID BEEP
      // ------------------------------------------------------

      else if (
        waterLevel < 99.0
      ) {

        if (
          currentMillis -
          previousLedMillis >=
          200
        ) {

          previousLedMillis =
            currentMillis;

          emergencyBlinkState =
            !emergencyBlinkState;


          setLed(
            GREEN_LED,
            emergencyBlinkState,
            BLYNK_GREEN_LED,
            greenLedState
          );

          setLed(
            YELLOW_LED,
            emergencyBlinkState,
            BLYNK_YELLOW_LED,
            yellowLedState
          );

          setLed(
            RED_LED,
            emergencyBlinkState,
            BLYNK_RED_LED,
            redLedState
          );
        }


        if (
          currentMillis -
          previousBuzzerMillis >=
          100
        ) {

          previousBuzzerMillis =
            currentMillis;

          buzzerState =
            !buzzerState;


          if (buzzerState) {

            digitalWrite(
              BUZZER,
              LOW
            );

          }

          else {

            digitalWrite(
              BUZZER,
              HIGH
            );
          }
        }
      }


      // ------------------------------------------------------
      // 99–100% → ALL LEDs + CONTINUOUS BUZZER
      // ------------------------------------------------------

      else {

        if (
          currentMillis -
          previousLedMillis >=
          200
        ) {

          previousLedMillis =
            currentMillis;

          emergencyBlinkState =
            !emergencyBlinkState;


          setLed(
            GREEN_LED,
            emergencyBlinkState,
            BLYNK_GREEN_LED,
            greenLedState
          );

          setLed(
            YELLOW_LED,
            emergencyBlinkState,
            BLYNK_YELLOW_LED,
            yellowLedState
          );

          setLed(
            RED_LED,
            emergencyBlinkState,
            BLYNK_RED_LED,
            redLedState
          );
        }


        digitalWrite(
          BUZZER,
          LOW
        );
      }
    }


    // ========================================================
    // PUMP IS MANUALLY OFF
    // ========================================================

    else if (
      !pumpShutdown &&
      !isPumpOn()
    ) {

      // ------------------------------------------------------
      // Pump is OFF.
      // Blue LED OFF.
      // ------------------------------------------------------

      setLed(
        BLUE_LED,
        false,
        BLYNK_BLUE_LED,
        blueLedState
      );


      // ------------------------------------------------------
      // Warning LEDs OFF
      // ------------------------------------------------------

      setLed(
        GREEN_LED,
        false,
        BLYNK_GREEN_LED,
        greenLedState
      );

      setLed(
        YELLOW_LED,
        false,
        BLYNK_YELLOW_LED,
        yellowLedState
      );

      setLed(
        RED_LED,
        false,
        BLYNK_RED_LED,
        redLedState
      );


      // ------------------------------------------------------
      // Buzzer OFF
      // ------------------------------------------------------

      digitalWrite(
        BUZZER,
        HIGH
      );

      buzzerState =
        false;
    }


    // ========================================================
    // AFTER PUMP SHUTDOWN
    // ========================================================

    if (
      pumpShutdown
    ) {

      // Everything OFF

      setLed(
        BLUE_LED,
        false,
        BLYNK_BLUE_LED,
        blueLedState
      );

      setLed(
        GREEN_LED,
        false,
        BLYNK_GREEN_LED,
        greenLedState
      );

      setLed(
        YELLOW_LED,
        false,
        BLYNK_YELLOW_LED,
        yellowLedState
      );

      setLed(
        RED_LED,
        false,
        BLYNK_RED_LED,
        redLedState
      );


      digitalWrite(
        BUZZER,
        HIGH
      );


      // Make absolutely sure relay remains OFF

      digitalWrite(
        RELAY_PIN,
        HIGH
      );
    }


    // ========================================================
    // SERIAL OUTPUT — EVERY 500ms
    // ========================================================

    if (
      currentMillis -
      previousPrintMillis >=
      PRINT_INTERVAL
    ) {

      previousPrintMillis =
        currentMillis;


      Serial.print(
        "Distance: "
      );

      Serial.print(
        distance,
        2
      );

      Serial.print(
        " cm | Water Level: "
      );

      Serial.print(
        waterLevel,
        1
      );

      Serial.print(
        " % | Flow 1: "
      );

      Serial.print(
        flow1Pulses
      );

      Serial.print(
        " pulses/sec | Flow 2: "
      );

      Serial.print(
        flow2Pulses
      );

      Serial.print(
        " pulses/sec | Difference: "
      );

      Serial.print(
        flowDifference
      );

      Serial.print(
        " | Blockage: "
      );


      if (blockageDetected) {

        Serial.print(
          "DETECTED"
        );

      }

      else {

        Serial.print(
          "NORMAL"
        );
      }


      Serial.print(
        " | Full Count: "
      );

      Serial.print(
        fullLevelCount
      );

      Serial.print(
        "/20"
      );


      Serial.print(
        " | Pump: "
      );


      if (pumpShutdown) {

        Serial.println(
          "OFF - SAFETY SHUTDOWN"
        );

      }

      else if (isPumpOn()) {

        Serial.println(
          "ON - MANUAL"
        );

      }

      else {

        Serial.println(
          "OFF - MANUAL"
        );
      }
    }
  }


  delay(50);
}
