/*
 * Water Detection and Pump Control System
 * 
 * This Arduino system monitors water levels in tanks and automatically
 * controls a water pump to maintain optimal water levels.
 * 
 * Features:
 * - Automatic pump control based on water levels
 * - Multiple sensor support (low and high water sensors)
 * - LED status indicators
 * - Safety features to prevent overflow and dry running
 * - Manual override capability
 * 
 * Hardware Requirements:
 * - Arduino Uno/Nano/Mega
 * - 2x Water level sensors (or ultrasonic sensor)
 * - Relay module for pump control
 * - LEDs for status indication
 * - Push button for manual control
 * - Buzzer for alerts (optional)
 */

// Pin Definitions
const int WATER_SENSOR_LOW = 2;     // Low water level sensor
const int WATER_SENSOR_HIGH = 3;    // High water level sensor (tank full)
const int PUMP_RELAY = 4;           // Relay to control water pump
const int LED_PUMP_ON = 5;          // LED indicator - pump running
const int LED_TANK_FULL = 6;        // LED indicator - tank full
const int LED_TANK_EMPTY = 7;       // LED indicator - tank empty
const int MANUAL_BUTTON = 8;        // Manual override button
const int BUZZER = 9;               // Buzzer for alerts (optional)
const int ULTRASONIC_TRIG = 10;     // Ultrasonic sensor trigger (alternative)
const int ULTRASONIC_ECHO = 11;     // Ultrasonic sensor echo (alternative)

// System Configuration
const unsigned long DEBOUNCE_DELAY = 50;      // Debounce time in ms
const unsigned long PUMP_COOLDOWN = 30000;    // Pump cooldown period (30 seconds)
const unsigned long SENSOR_READ_INTERVAL = 500; // Sensor reading interval
const int TANK_HEIGHT_CM = 100;               // Tank height in cm (for ultrasonic)
const int MIN_WATER_LEVEL_CM = 20;            // Minimum water level
const int MAX_WATER_LEVEL_CM = 90;            // Maximum water level

// System States
enum SystemState {
  STATE_IDLE,
  STATE_FILLING,
  STATE_FULL,
  STATE_EMPTY,
  STATE_ERROR,
  STATE_MANUAL
};

// Global Variables
SystemState currentState = STATE_IDLE;
SystemState previousState = STATE_IDLE;
bool pumpRunning = false;
bool manualMode = false;
bool useUltrasonic = false;  // Set to true if using ultrasonic sensor
unsigned long lastPumpStopTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastDebounceTime = 0;
int lastButtonState = HIGH;
int buttonState = HIGH;
float currentWaterLevel = 0;  // For ultrasonic sensor

// Function Prototypes
void setupPins();
void readSensors();
void readUltrasonicSensor();
float getUltrasonicDistance();
void updateSystemState();
void controlPump();
void updateIndicators();
void checkManualOverride();
void handleEmergency();
void playAlert(int type);
void debugPrint(String message);

void setup() {
  Serial.begin(9600);
  Serial.println("Water Pump Control System Starting...");
  
  setupPins();
  
  // Initial sensor reading
  delay(1000);
  readSensors();
  updateSystemState();
  
  Serial.println("System Ready!");
  Serial.println("Commands: 'S' - Status, 'M' - Toggle Manual, 'P' - Toggle Pump (manual mode)");
}

void loop() {
  // Read sensors at regular intervals
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    if (useUltrasonic) {
      readUltrasonicSensor();
    } else {
      readSensors();
    }
    lastSensorRead = millis();
  }
  
  // Check for manual override
  checkManualOverride();
  
  // Update system state based on sensor readings
  if (!manualMode) {
    updateSystemState();
  }
  
  // Control pump based on current state
  controlPump();
  
  // Update LED indicators
  updateIndicators();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Small delay to prevent CPU overload
  delay(10);
}

void setupPins() {
  // Configure input pins
  pinMode(WATER_SENSOR_LOW, INPUT_PULLUP);
  pinMode(WATER_SENSOR_HIGH, INPUT_PULLUP);
  pinMode(MANUAL_BUTTON, INPUT_PULLUP);
  pinMode(ULTRASONIC_ECHO, INPUT);
  
  // Configure output pins
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(LED_PUMP_ON, OUTPUT);
  pinMode(LED_TANK_FULL, OUTPUT);
  pinMode(LED_TANK_EMPTY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  
  // Ensure pump is off initially
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(LED_PUMP_ON, LOW);
  digitalWrite(LED_TANK_FULL, LOW);
  digitalWrite(LED_TANK_EMPTY, LOW);
  digitalWrite(BUZZER, LOW);
}

void readSensors() {
  // Read water level sensors (active LOW - sensor triggered when in water)
  bool lowSensorTriggered = !digitalRead(WATER_SENSOR_LOW);
  bool highSensorTriggered = !digitalRead(WATER_SENSOR_HIGH);
  
  // Debug output
  if (Serial.available() > 0) {
    debugPrint("Low Sensor: " + String(lowSensorTriggered ? "WET" : "DRY"));
    debugPrint("High Sensor: " + String(highSensorTriggered ? "WET" : "DRY"));
  }
}

void readUltrasonicSensor() {
  currentWaterLevel = getUltrasonicDistance();
  
  // Convert distance to water level (inverse relationship)
  float waterLevelPercent = ((TANK_HEIGHT_CM - currentWaterLevel) / float(TANK_HEIGHT_CM)) * 100;
  
  if (Serial.available() > 0) {
    debugPrint("Water Level: " + String(waterLevelPercent) + "%");
  }
}

float getUltrasonicDistance() {
  // Send ultrasonic pulse
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  // Read echo time
  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  
  // Calculate distance in cm
  float distance = duration * 0.034 / 2;
  
  // Validate reading
  if (distance <= 0 || distance > TANK_HEIGHT_CM) {
    return TANK_HEIGHT_CM;  // Return max distance if invalid
  }
  
  return distance;
}

void updateSystemState() {
  SystemState newState = currentState;
  
  if (useUltrasonic) {
    // Ultrasonic sensor logic
    float waterHeight = TANK_HEIGHT_CM - currentWaterLevel;
    
    if (waterHeight >= MAX_WATER_LEVEL_CM) {
      newState = STATE_FULL;
    } else if (waterHeight <= MIN_WATER_LEVEL_CM) {
      newState = STATE_EMPTY;
    } else {
      if (currentState == STATE_EMPTY || currentState == STATE_FILLING) {
        newState = STATE_FILLING;
      } else {
        newState = STATE_IDLE;
      }
    }
  } else {
    // Float switch sensor logic
    bool lowSensor = !digitalRead(WATER_SENSOR_LOW);
    bool highSensor = !digitalRead(WATER_SENSOR_HIGH);
    
    if (highSensor) {
      newState = STATE_FULL;
    } else if (!lowSensor) {
      newState = STATE_EMPTY;
    } else {
      if (currentState == STATE_EMPTY || currentState == STATE_FILLING) {
        newState = STATE_FILLING;
      } else {
        newState = STATE_IDLE;
      }
    }
  }
  
  // State transition handling
  if (newState != currentState) {
    previousState = currentState;
    currentState = newState;
    
    // Log state change
    Serial.print("State changed: ");
    printState(currentState);
    
    // Play alert on state change
    if (currentState == STATE_FULL) {
      playAlert(1);  // Full tank alert
    } else if (currentState == STATE_EMPTY) {
      playAlert(2);  // Empty tank alert
    }
  }
}

void controlPump() {
  bool shouldRunPump = false;
  
  if (manualMode) {
    // Manual mode - pump controlled by serial commands
    return;
  }
  
  // Check cooldown period
  if (millis() - lastPumpStopTime < PUMP_COOLDOWN && !pumpRunning) {
    return;  // Still in cooldown
  }
  
  // Determine if pump should run based on state
  switch (currentState) {
    case STATE_EMPTY:
    case STATE_FILLING:
      shouldRunPump = true;
      break;
      
    case STATE_FULL:
    case STATE_IDLE:
      shouldRunPump = false;
      break;
      
    case STATE_ERROR:
      shouldRunPump = false;
      handleEmergency();
      break;
      
    default:
      shouldRunPump = false;
  }
  
  // Control pump relay
  if (shouldRunPump && !pumpRunning) {
    // Turn pump ON
    digitalWrite(PUMP_RELAY, HIGH);
    pumpRunning = true;
    Serial.println("PUMP: ON");
  } else if (!shouldRunPump && pumpRunning) {
    // Turn pump OFF
    digitalWrite(PUMP_RELAY, LOW);
    pumpRunning = false;
    lastPumpStopTime = millis();
    Serial.println("PUMP: OFF");
  }
}

void updateIndicators() {
  // Pump running LED
  digitalWrite(LED_PUMP_ON, pumpRunning ? HIGH : LOW);
  
  // Tank status LEDs
  digitalWrite(LED_TANK_FULL, currentState == STATE_FULL ? HIGH : LOW);
  digitalWrite(LED_TANK_EMPTY, currentState == STATE_EMPTY ? HIGH : LOW);
  
  // Blink LED in manual mode
  if (manualMode) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 500) {
      digitalWrite(LED_PUMP_ON, !digitalRead(LED_PUMP_ON));
      lastBlink = millis();
    }
  }
}

void checkManualOverride() {
  int reading = digitalRead(MANUAL_BUTTON);
  
  // Debounce button
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      // Button pressed (active LOW)
      if (buttonState == LOW) {
        manualMode = !manualMode;
        Serial.print("Manual Mode: ");
        Serial.println(manualMode ? "ON" : "OFF");
        
        if (!manualMode) {
          // Exiting manual mode - update state
          readSensors();
          updateSystemState();
        }
      }
    }
  }
  
  lastButtonState = reading;
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    switch (command) {
      case 'S':
      case 's':
        printStatus();
        break;
        
      case 'M':
      case 'm':
        manualMode = !manualMode;
        Serial.print("Manual Mode: ");
        Serial.println(manualMode ? "ON" : "OFF");
        break;
        
      case 'P':
      case 'p':
        if (manualMode) {
          pumpRunning = !pumpRunning;
          digitalWrite(PUMP_RELAY, pumpRunning ? HIGH : LOW);
          Serial.print("Pump: ");
          Serial.println(pumpRunning ? "ON" : "OFF");
        } else {
          Serial.println("Error: Enable manual mode first (M)");
        }
        break;
        
      case 'U':
      case 'u':
        useUltrasonic = !useUltrasonic;
        Serial.print("Sensor Mode: ");
        Serial.println(useUltrasonic ? "Ultrasonic" : "Float Switches");
        break;
        
      case 'H':
      case 'h':
        printHelp();
        break;
        
      default:
        Serial.println("Unknown command. Press 'H' for help.");
    }
  }
}

void printStatus() {
  Serial.println("\n=== System Status ===");
  Serial.print("State: ");
  printState(currentState);
  Serial.print("Pump: ");
  Serial.println(pumpRunning ? "ON" : "OFF");
  Serial.print("Mode: ");
  Serial.println(manualMode ? "MANUAL" : "AUTO");
  
  if (!useUltrasonic) {
    Serial.print("Low Sensor: ");
    Serial.println(!digitalRead(WATER_SENSOR_LOW) ? "WET" : "DRY");
    Serial.print("High Sensor: ");
    Serial.println(!digitalRead(WATER_SENSOR_HIGH) ? "WET" : "DRY");
  } else {
    float waterHeight = TANK_HEIGHT_CM - currentWaterLevel;
    float waterPercent = (waterHeight / float(TANK_HEIGHT_CM)) * 100;
    Serial.print("Water Level: ");
    Serial.print(waterPercent);
    Serial.println("%");
  }
  
  if (!pumpRunning && (millis() - lastPumpStopTime < PUMP_COOLDOWN)) {
    Serial.print("Cooldown: ");
    Serial.print((PUMP_COOLDOWN - (millis() - lastPumpStopTime)) / 1000);
    Serial.println(" seconds remaining");
  }
  Serial.println("====================\n");
}

void printState(SystemState state) {
  switch (state) {
    case STATE_IDLE:
      Serial.println("IDLE");
      break;
    case STATE_FILLING:
      Serial.println("FILLING");
      break;
    case STATE_FULL:
      Serial.println("FULL");
      break;
    case STATE_EMPTY:
      Serial.println("EMPTY");
      break;
    case STATE_ERROR:
      Serial.println("ERROR");
      break;
    case STATE_MANUAL:
      Serial.println("MANUAL");
      break;
  }
}

void printHelp() {
  Serial.println("\n=== Help Menu ===");
  Serial.println("S - Show status");
  Serial.println("M - Toggle manual mode");
  Serial.println("P - Toggle pump (manual mode only)");
  Serial.println("U - Toggle sensor type");
  Serial.println("H - Show this help");
  Serial.println("=================\n");
}

void handleEmergency() {
  // Emergency shutdown
  digitalWrite(PUMP_RELAY, LOW);
  pumpRunning = false;
  
  // Alert user
  playAlert(3);
  
  Serial.println("EMERGENCY: System Error!");
}

void playAlert(int type) {
  if (digitalRead(BUZZER) == HIGH) return;  // Already playing
  
  switch (type) {
    case 1:  // Tank full
      tone(BUZZER, 1000, 200);
      delay(250);
      tone(BUZZER, 1000, 200);
      break;
      
    case 2:  // Tank empty
      tone(BUZZER, 500, 500);
      break;
      
    case 3:  // Error
      for (int i = 0; i < 3; i++) {
        tone(BUZZER, 2000, 100);
        delay(150);
      }
      break;
  }
}

void debugPrint(String message) {
  #ifdef DEBUG
  Serial.print("[DEBUG] ");
  Serial.println(message);
  #endif
}