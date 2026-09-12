// ============================================================
// ESP32 DRAINAGE BLOCKAGE DETECTION SYSTEM
// Blynk IoT + Water Level + 2 Flow Sensors
// LEDs + Buzzer + Relay + Automatic Pump Protection
// ============================================================

// ============================================================
// BLYNK CONFIGURATION
// ============================================================

#define BLYNK_TEMPLATE_ID "TMPL3zJG9PmKu"
#define BLYNK_TEMPLATE_NAME "IoT based Drainage Block Detection"
#define BLYNK_AUTH_TOKEN "a9VC5Mt3RT-c3hAm1TBjnEGXPD4t3Dkm"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// -------------------- WIFI --------------------

char ssid[] = "Airtel_Los Pollos Hermanos";
char pass[] = "Kash@144";

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
// SEND DATA TO BLYNK
// ============================================================

void sendDataToBlynk() {

  // ----------------------------------------------------------
  // FLOW SENSOR DATA
  // ----------------------------------------------------------

  Blynk.virtualWrite(V0, flow1Pulses);
  Blynk.virtualWrite(V1, flow2Pulses);


  // ----------------------------------------------------------
  // WATER LEVEL
  // ----------------------------------------------------------

  Blynk.virtualWrite(V2, currentWaterLevel);


  // ----------------------------------------------------------
  // FLOW DIFFERENCE
  // ----------------------------------------------------------

  Blynk.virtualWrite(V3, flowDifference);


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

  if (pumpShutdown) {

    Blynk.virtualWrite(
      V5,
      "OFF - SHUTDOWN"
    );

  }

  else {

    Blynk.virtualWrite(
      V5,
      "ON"
    );
  }


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
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);


  // ==========================================================
  // ULTRASONIC
  // ==========================================================

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);


  // ==========================================================
  // LEDs
  // ==========================================================

  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);


  // ==========================================================
  // BUZZER
  // ==========================================================

  pinMode(BUZZER, OUTPUT);


  // ==========================================================
  // FLOW SENSORS
  // ==========================================================

  pinMode(FLOW_SENSOR_1, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_2, INPUT_PULLUP);


  // ==========================================================
  // RELAY
  // ==========================================================

  pinMode(RELAY_PIN, OUTPUT);


  // ==========================================================
  // INITIAL STATES
  // ==========================================================

  // Pump ON
  digitalWrite(RELAY_PIN, LOW);


  // ----------------------------------------------------------
  // Blue LED = Pump ON
  // ----------------------------------------------------------

  blueLedState = true;

  digitalWrite(
    BLUE_LED,
    HIGH
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
    "=============================================================="
  );

  Serial.println(
    "Pump: ON"
  );

  Serial.println(
    "Blue LED: Pump Status"
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
    // Blue LED = pump status
    // --------------------------------------------------------

    if (pumpShutdown) {

      setLed(
        BLUE_LED,
        false,
        BLYNK_BLUE_LED,
        blueLedState
      );

    }

    else {

      setLed(
        BLUE_LED,
        true,
        BLYNK_BLUE_LED,
        blueLedState
      );
    }


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
          "OFF"
        );

      }

      else {

        Serial.println(
          "ON"
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

          pumpShutdown =
            true;


          // Pump OFF

          digitalWrite(
            RELAY_PIN,
            HIGH
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


          // Buzzer OFF

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
    // Only operates while pump is running.
    // ========================================================

    if (
      !pumpShutdown
    ) {


      // ------------------------------------------------------
      // BLUE LED = PUMP STATUS
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
    // AFTER PUMP SHUTDOWN
    // ========================================================

    else {

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
          "OFF"
        );

      }

      else {

        Serial.println(
          "ON"
        );
      }
    }
  }


  delay(50);
}
