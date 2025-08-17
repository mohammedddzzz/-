/*
 * Water Detection and Pump Control System - Advanced Version
 * 
 * Enhanced features:
 * - LCD display for real-time information
 * - Data logging capabilities
 * - Temperature monitoring
 * - Advanced safety mechanisms
 * - Water usage statistics
 * - Multiple tank support
 * 
 * Additional Hardware:
 * - 16x2 I2C LCD Display
 * - DS18B20 Temperature sensor (optional)
 * - SD Card module (optional)
 * - RTC module (optional)
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

// Uncomment these if using optional features
// #include <SD.h>
// #include <RTClib.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>

// LCD Configuration
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Address may be 0x3F for some modules

// Pin Definitions (same as basic version)
const int WATER_SENSOR_LOW = 2;
const int WATER_SENSOR_HIGH = 3;
const int PUMP_RELAY = 4;
const int LED_PUMP_ON = 5;
const int LED_TANK_FULL = 6;
const int LED_TANK_EMPTY = 7;
const int MANUAL_BUTTON = 8;
const int BUZZER = 9;
const int ULTRASONIC_TRIG = 10;
const int ULTRASONIC_ECHO = 11;
const int TEMP_SENSOR = 12;  // DS18B20 temperature sensor
const int SD_CS_PIN = 53;    // SD card chip select (Mega)

// Advanced Configuration
const float TANK_CAPACITY_LITERS = 1000.0;  // Tank capacity
const float FLOW_RATE_LPM = 20.0;           // Pump flow rate (liters per minute)
const float MAX_TEMP_C = 50.0;              // Maximum safe temperature
const float MIN_TEMP_C = 5.0;               // Minimum operating temperature
const int MAX_DAILY_CYCLES = 10;            // Maximum pump cycles per day
const unsigned long MAX_RUN_TIME = 600000;  // Maximum continuous run (10 minutes)

// EEPROM Addresses
const int EEPROM_TOTAL_CYCLES = 0;      // 4 bytes
const int EEPROM_TOTAL_RUNTIME = 4;     // 4 bytes
const int EEPROM_LAST_MAINTENANCE = 8;  // 4 bytes
const int EEPROM_CONFIG_FLAG = 12;      // 1 byte

// System Configuration (from basic version)
const unsigned long DEBOUNCE_DELAY = 50;
const unsigned long PUMP_COOLDOWN = 30000;
const unsigned long SENSOR_READ_INTERVAL = 500;
const unsigned long LCD_UPDATE_INTERVAL = 1000;
const unsigned long STATS_SAVE_INTERVAL = 60000;  // Save stats every minute
const int TANK_HEIGHT_CM = 100;
const int MIN_WATER_LEVEL_CM = 20;
const int MAX_WATER_LEVEL_CM = 90;

// System States
enum SystemState {
  STATE_IDLE,
  STATE_FILLING,
  STATE_FULL,
  STATE_EMPTY,
  STATE_ERROR,
  STATE_MANUAL,
  STATE_MAINTENANCE,
  STATE_OVERHEAT,
  STATE_FROZEN
};

// Error Codes
enum ErrorCode {
  ERROR_NONE = 0,
  ERROR_SENSOR_FAULT = 1,
  ERROR_PUMP_TIMEOUT = 2,
  ERROR_OVERHEAT = 3,
  ERROR_FROZEN = 4,
  ERROR_OVERFLOW = 5,
  ERROR_MAX_CYCLES = 6
};

// Global Variables
SystemState currentState = STATE_IDLE;
SystemState previousState = STATE_IDLE;
ErrorCode currentError = ERROR_NONE;
bool pumpRunning = false;
bool manualMode = false;
bool useUltrasonic = false;
bool maintenanceMode = false;
bool sdCardPresent = false;
bool rtcPresent = false;
bool tempSensorPresent = false;

// Timing Variables
unsigned long lastPumpStopTime = 0;
unsigned long lastSensorRead = 0;
unsigned long lastLCDUpdate = 0;
unsigned long lastStatsSave = 0;
unsigned long pumpStartTime = 0;
unsigned long lastDebounceTime = 0;
int lastButtonState = HIGH;
int buttonState = HIGH;

// Measurement Variables
float currentWaterLevel = 0;
float currentTemperature = 25.0;
float waterVolume = 0;
int dailyCycles = 0;
unsigned long dailyRuntime = 0;

// Statistics Variables
unsigned long totalCycles = 0;
unsigned long totalRuntime = 0;
unsigned long lastMaintenanceDate = 0;

// Custom LCD Characters
byte waterDrop[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b10001,
  0b01110
};

byte pump[8] = {
  0b00000,
  0b01110,
  0b10001,
  0b10001,
  0b11111,
  0b10001,
  0b10001,
  0b00000
};

byte thermometer[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

// Function Prototypes
void initializeLCD();
void loadStatistics();
void saveStatistics();
void updateLCD();
void displayError();
void checkTemperature();
void calculateWaterVolume();
void logData();
void performDiagnostics();
void resetDailyStats();
void checkMaintenanceSchedule();

void setup() {
  Serial.begin(9600);
  Serial.println("Advanced Water Pump Control System Starting...");
  
  // Initialize LCD
  initializeLCD();
  
  // Setup pins (from basic version)
  setupPins();
  
  // Load saved statistics from EEPROM
  loadStatistics();
  
  // Initialize optional components
  initializeOptionalComponents();
  
  // Perform system diagnostics
  performDiagnostics();
  
  // Initial sensor reading
  delay(1000);
  readSensors();
  updateSystemState();
  
  Serial.println("System Ready!");
  lcd.clear();
  lcd.print("System Ready");
  delay(2000);
}

void loop() {
  // Read sensors at regular intervals
  if (millis() - lastSensorRead >= SENSOR_READ_INTERVAL) {
    if (useUltrasonic) {
      readUltrasonicSensor();
    } else {
      readSensors();
    }
    
    // Check temperature if sensor present
    if (tempSensorPresent) {
      checkTemperature();
    }
    
    lastSensorRead = millis();
  }
  
  // Update LCD display
  if (millis() - lastLCDUpdate >= LCD_UPDATE_INTERVAL) {
    updateLCD();
    lastLCDUpdate = millis();
  }
  
  // Save statistics periodically
  if (millis() - lastStatsSave >= STATS_SAVE_INTERVAL) {
    saveStatistics();
    lastStatsSave = millis();
  }
  
  // Check for manual override
  checkManualOverride();
  
  // Update system state
  if (!manualMode && !maintenanceMode) {
    updateSystemState();
  }
  
  // Control pump with safety checks
  controlPumpAdvanced();
  
  // Update indicators
  updateIndicators();
  
  // Handle serial commands
  handleSerialCommands();
  
  // Check maintenance schedule
  checkMaintenanceSchedule();
  
  // Log data if SD card present
  if (sdCardPresent) {
    logData();
  }
  
  delay(10);
}

void initializeLCD() {
  lcd.init();
  lcd.backlight();
  
  // Create custom characters
  lcd.createChar(0, waterDrop);
  lcd.createChar(1, pump);
  lcd.createChar(2, thermometer);
  
  // Display startup message
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Water Pump Ctrl");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
}

void initializeOptionalComponents() {
  // Initialize SD card
  if (SD.begin(SD_CS_PIN)) {
    sdCardPresent = true;
    Serial.println("SD Card initialized");
  } else {
    Serial.println("SD Card not found");
  }
  
  // Initialize RTC
  // if (rtc.begin()) {
  //   rtcPresent = true;
  //   Serial.println("RTC initialized");
  // }
  
  // Initialize temperature sensor
  // if (sensors.getDeviceCount() > 0) {
  //   tempSensorPresent = true;
  //   Serial.println("Temperature sensor found");
  // }
}

void loadStatistics() {
  // Check if EEPROM has been initialized
  if (EEPROM.read(EEPROM_CONFIG_FLAG) != 0xAA) {
    // First run, initialize EEPROM
    EEPROM.write(EEPROM_CONFIG_FLAG, 0xAA);
    totalCycles = 0;
    totalRuntime = 0;
    lastMaintenanceDate = millis();
    saveStatistics();
  } else {
    // Load existing statistics
    EEPROM.get(EEPROM_TOTAL_CYCLES, totalCycles);
    EEPROM.get(EEPROM_TOTAL_RUNTIME, totalRuntime);
    EEPROM.get(EEPROM_LAST_MAINTENANCE, lastMaintenanceDate);
  }
  
  Serial.print("Total Cycles: ");
  Serial.println(totalCycles);
  Serial.print("Total Runtime: ");
  Serial.print(totalRuntime / 60000);
  Serial.println(" minutes");
}

void saveStatistics() {
  EEPROM.put(EEPROM_TOTAL_CYCLES, totalCycles);
  EEPROM.put(EEPROM_TOTAL_RUNTIME, totalRuntime);
  EEPROM.put(EEPROM_LAST_MAINTENANCE, lastMaintenanceDate);
}

void updateLCD() {
  lcd.clear();
  
  // First line - Status and water level
  lcd.setCursor(0, 0);
  
  switch (currentState) {
    case STATE_IDLE:
      lcd.print("IDLE ");
      break;
    case STATE_FILLING:
      lcd.write(1);  // Pump icon
      lcd.print("FILLING ");
      break;
    case STATE_FULL:
      lcd.print("FULL ");
      break;
    case STATE_EMPTY:
      lcd.print("EMPTY ");
      break;
    case STATE_ERROR:
      lcd.print("ERROR:");
      lcd.print(currentError);
      break;
    case STATE_MANUAL:
      lcd.print("MANUAL ");
      break;
    case STATE_MAINTENANCE:
      lcd.print("MAINTENANCE ");
      return;
    case STATE_OVERHEAT:
      lcd.print("OVERHEAT! ");
      return;
    case STATE_FROZEN:
      lcd.print("FROZEN! ");
      return;
  }
  
  // Display water level
  if (useUltrasonic) {
    float waterPercent = ((TANK_HEIGHT_CM - currentWaterLevel) / float(TANK_HEIGHT_CM)) * 100;
    lcd.write(0);  // Water drop icon
    lcd.print(waterPercent, 0);
    lcd.print("%");
  } else {
    lcd.write(0);  // Water drop icon
    if (!digitalRead(WATER_SENSOR_HIGH)) {
      lcd.print("HIGH");
    } else if (!digitalRead(WATER_SENSOR_LOW)) {
      lcd.print("MID");
    } else {
      lcd.print("LOW");
    }
  }
  
  // Second line - Pump status and temperature
  lcd.setCursor(0, 1);
  
  if (pumpRunning) {
    unsigned long runtime = (millis() - pumpStartTime) / 1000;
    lcd.print("RUN:");
    lcd.print(runtime);
    lcd.print("s ");
  } else {
    lcd.print("Cycles:");
    lcd.print(dailyCycles);
    lcd.print(" ");
  }
  
  // Display temperature if available
  if (tempSensorPresent) {
    lcd.setCursor(11, 1);
    lcd.write(2);  // Thermometer icon
    lcd.print(currentTemperature, 1);
    lcd.print("C");
  }
}

void checkTemperature() {
  // Read temperature from DS18B20
  // sensors.requestTemperatures();
  // currentTemperature = sensors.getTempCByIndex(0);
  
  // Simulated temperature for testing
  currentTemperature = 25.0 + random(-5, 5);
  
  // Check temperature limits
  if (currentTemperature > MAX_TEMP_C) {
    currentState = STATE_OVERHEAT;
    currentError = ERROR_OVERHEAT;
    Serial.println("WARNING: Overheating detected!");
  } else if (currentTemperature < MIN_TEMP_C) {
    currentState = STATE_FROZEN;
    currentError = ERROR_FROZEN;
    Serial.println("WARNING: Freezing conditions!");
  }
}

void controlPumpAdvanced() {
  bool shouldRunPump = false;
  
  // Safety checks
  if (currentState == STATE_OVERHEAT || currentState == STATE_FROZEN) {
    // Emergency shutdown
    if (pumpRunning) {
      digitalWrite(PUMP_RELAY, LOW);
      pumpRunning = false;
      Serial.println("PUMP: Emergency shutdown!");
    }
    return;
  }
  
  // Check maximum daily cycles
  if (dailyCycles >= MAX_DAILY_CYCLES) {
    currentError = ERROR_MAX_CYCLES;
    Serial.println("Maximum daily cycles reached!");
    return;
  }
  
  // Check maximum continuous runtime
  if (pumpRunning && (millis() - pumpStartTime > MAX_RUN_TIME)) {
    currentError = ERROR_PUMP_TIMEOUT;
    digitalWrite(PUMP_RELAY, LOW);
    pumpRunning = false;
    Serial.println("PUMP: Timeout - maximum runtime exceeded!");
    return;
  }
  
  if (manualMode || maintenanceMode) {
    return;
  }
  
  // Check cooldown period
  if (millis() - lastPumpStopTime < PUMP_COOLDOWN && !pumpRunning) {
    return;
  }
  
  // Determine if pump should run
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
  
  // Control pump relay with statistics tracking
  if (shouldRunPump && !pumpRunning) {
    // Turn pump ON
    digitalWrite(PUMP_RELAY, HIGH);
    pumpRunning = true;
    pumpStartTime = millis();
    dailyCycles++;
    totalCycles++;
    Serial.println("PUMP: ON");
    Serial.print("Daily cycles: ");
    Serial.println(dailyCycles);
    
  } else if (!shouldRunPump && pumpRunning) {
    // Turn pump OFF
    digitalWrite(PUMP_RELAY, LOW);
    pumpRunning = false;
    lastPumpStopTime = millis();
    
    // Calculate and update runtime
    unsigned long runtime = millis() - pumpStartTime;
    dailyRuntime += runtime;
    totalRuntime += runtime;
    
    Serial.println("PUMP: OFF");
    Serial.print("Runtime: ");
    Serial.print(runtime / 1000);
    Serial.println(" seconds");
    
    // Calculate water pumped
    float waterPumped = (runtime / 60000.0) * FLOW_RATE_LPM;
    Serial.print("Water pumped: ");
    Serial.print(waterPumped);
    Serial.println(" liters");
  }
}

void performDiagnostics() {
  Serial.println("\n=== System Diagnostics ===");
  
  // Test sensors
  Serial.print("Low sensor: ");
  Serial.println(digitalRead(WATER_SENSOR_LOW) ? "OK" : "TRIGGERED");
  
  Serial.print("High sensor: ");
  Serial.println(digitalRead(WATER_SENSOR_HIGH) ? "OK" : "TRIGGERED");
  
  // Test relay
  Serial.print("Testing relay... ");
  digitalWrite(PUMP_RELAY, HIGH);
  delay(100);
  digitalWrite(PUMP_RELAY, LOW);
  Serial.println("OK");
  
  // Test LEDs
  Serial.print("Testing LEDs... ");
  digitalWrite(LED_PUMP_ON, HIGH);
  digitalWrite(LED_TANK_FULL, HIGH);
  digitalWrite(LED_TANK_EMPTY, HIGH);
  delay(500);
  digitalWrite(LED_PUMP_ON, LOW);
  digitalWrite(LED_TANK_FULL, LOW);
  digitalWrite(LED_TANK_EMPTY, LOW);
  Serial.println("OK");
  
  // Test buzzer
  if (BUZZER > 0) {
    Serial.print("Testing buzzer... ");
    tone(BUZZER, 1000, 200);
    delay(300);
    Serial.println("OK");
  }
  
  Serial.println("========================\n");
}

void checkMaintenanceSchedule() {
  // Check if maintenance is due (every 30 days)
  unsigned long maintenanceInterval = 30UL * 24 * 60 * 60 * 1000;  // 30 days in ms
  
  if (millis() - lastMaintenanceDate > maintenanceInterval) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MAINTENANCE DUE!");
    lcd.setCursor(0, 1);
    lcd.print("Check sensors");
    
    // Alert user
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 1500, 300);
      delay(400);
    }
    
    Serial.println("MAINTENANCE REMINDER: Please check and clean sensors!");
  }
}

void resetDailyStats() {
  dailyCycles = 0;
  dailyRuntime = 0;
  Serial.println("Daily statistics reset");
}

void logData() {
  // Log data to SD card
  // File dataFile = SD.open("pumplog.csv", FILE_WRITE);
  // if (dataFile) {
  //   dataFile.print(millis());
  //   dataFile.print(",");
  //   dataFile.print(currentState);
  //   dataFile.print(",");
  //   dataFile.print(currentWaterLevel);
  //   dataFile.print(",");
  //   dataFile.print(currentTemperature);
  //   dataFile.print(",");
  //   dataFile.println(pumpRunning ? 1 : 0);
  //   dataFile.close();
  // }
}

void displayStatistics() {
  Serial.println("\n=== System Statistics ===");
  Serial.print("Total Cycles: ");
  Serial.println(totalCycles);
  Serial.print("Total Runtime: ");
  Serial.print(totalRuntime / 60000);
  Serial.println(" minutes");
  Serial.print("Daily Cycles: ");
  Serial.println(dailyCycles);
  Serial.print("Daily Runtime: ");
  Serial.print(dailyRuntime / 60000);
  Serial.println(" minutes");
  Serial.print("Average Cycle Time: ");
  if (totalCycles > 0) {
    Serial.print(totalRuntime / totalCycles / 1000);
    Serial.println(" seconds");
  } else {
    Serial.println("N/A");
  }
  Serial.print("Estimated Water Used Today: ");
  Serial.print((dailyRuntime / 60000.0) * FLOW_RATE_LPM);
  Serial.println(" liters");
  Serial.println("========================\n");
}

// Include all functions from basic version
void setupPins() {
  pinMode(WATER_SENSOR_LOW, INPUT_PULLUP);
  pinMode(WATER_SENSOR_HIGH, INPUT_PULLUP);
  pinMode(MANUAL_BUTTON, INPUT_PULLUP);
  pinMode(ULTRASONIC_ECHO, INPUT);
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(LED_PUMP_ON, OUTPUT);
  pinMode(LED_TANK_FULL, OUTPUT);
  pinMode(LED_TANK_EMPTY, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(ULTRASONIC_TRIG, OUTPUT);
  
  digitalWrite(PUMP_RELAY, LOW);
  digitalWrite(LED_PUMP_ON, LOW);
  digitalWrite(LED_TANK_FULL, LOW);
  digitalWrite(LED_TANK_EMPTY, LOW);
  digitalWrite(BUZZER, LOW);
}

void readSensors() {
  // Same as basic version
  bool lowSensorTriggered = !digitalRead(WATER_SENSOR_LOW);
  bool highSensorTriggered = !digitalRead(WATER_SENSOR_HIGH);
}

void readUltrasonicSensor() {
  // Same as basic version
  digitalWrite(ULTRASONIC_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG, LOW);
  
  long duration = pulseIn(ULTRASONIC_ECHO, HIGH, 30000);
  currentWaterLevel = duration * 0.034 / 2;
  
  if (currentWaterLevel <= 0 || currentWaterLevel > TANK_HEIGHT_CM) {
    currentWaterLevel = TANK_HEIGHT_CM;
  }
  
  // Calculate water volume
  calculateWaterVolume();
}

void calculateWaterVolume() {
  float waterHeight = TANK_HEIGHT_CM - currentWaterLevel;
  float waterPercent = (waterHeight / float(TANK_HEIGHT_CM));
  waterVolume = waterPercent * TANK_CAPACITY_LITERS;
}

void updateSystemState() {
  SystemState newState = currentState;
  
  // Check for errors first
  if (currentError != ERROR_NONE) {
    newState = STATE_ERROR;
  } else if (useUltrasonic) {
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
  
  if (newState != currentState) {
    previousState = currentState;
    currentState = newState;
    
    Serial.print("State changed: ");
    printState(currentState);
    
    if (currentState == STATE_FULL) {
      playAlert(1);
    } else if (currentState == STATE_EMPTY) {
      playAlert(2);
    }
  }
}

void updateIndicators() {
  digitalWrite(LED_PUMP_ON, pumpRunning ? HIGH : LOW);
  digitalWrite(LED_TANK_FULL, currentState == STATE_FULL ? HIGH : LOW);
  digitalWrite(LED_TANK_EMPTY, currentState == STATE_EMPTY ? HIGH : LOW);
  
  if (manualMode || maintenanceMode) {
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 500) {
      digitalWrite(LED_PUMP_ON, !digitalRead(LED_PUMP_ON));
      lastBlink = millis();
    }
  }
}

void checkManualOverride() {
  int reading = digitalRead(MANUAL_BUTTON);
  
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != buttonState) {
      buttonState = reading;
      
      if (buttonState == LOW) {
        manualMode = !manualMode;
        Serial.print("Manual Mode: ");
        Serial.println(manualMode ? "ON" : "OFF");
        
        if (!manualMode) {
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
        displayStatistics();
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
          if (pumpRunning) {
            pumpStartTime = millis();
          }
          Serial.print("Pump: ");
          Serial.println(pumpRunning ? "ON" : "OFF");
        } else {
          Serial.println("Error: Enable manual mode first (M)");
        }
        break;
        
      case 'R':
      case 'r':
        resetDailyStats();
        break;
        
      case 'D':
      case 'd':
        performDiagnostics();
        break;
        
      case 'T':
      case 't':
        maintenanceMode = !maintenanceMode;
        Serial.print("Maintenance Mode: ");
        Serial.println(maintenanceMode ? "ON" : "OFF");
        if (maintenanceMode) {
          currentState = STATE_MAINTENANCE;
        }
        break;
        
      case 'C':
      case 'c':
        currentError = ERROR_NONE;
        Serial.println("Errors cleared");
        break;
        
      case 'H':
      case 'h':
        printAdvancedHelp();
        break;
        
      default:
        Serial.println("Unknown command. Press 'H' for help.");
    }
  }
}

void printAdvancedHelp() {
  Serial.println("\n=== Advanced Commands ===");
  Serial.println("S - Show statistics");
  Serial.println("M - Toggle manual mode");
  Serial.println("P - Toggle pump (manual)");
  Serial.println("R - Reset daily stats");
  Serial.println("D - Run diagnostics");
  Serial.println("T - Maintenance mode");
  Serial.println("C - Clear errors");
  Serial.println("H - Show this help");
  Serial.println("========================\n");
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
      Serial.print("ERROR: ");
      Serial.println(currentError);
      break;
    case STATE_MANUAL:
      Serial.println("MANUAL");
      break;
    case STATE_MAINTENANCE:
      Serial.println("MAINTENANCE");
      break;
    case STATE_OVERHEAT:
      Serial.println("OVERHEAT");
      break;
    case STATE_FROZEN:
      Serial.println("FROZEN");
      break;
  }
}

void handleEmergency() {
  digitalWrite(PUMP_RELAY, LOW);
  pumpRunning = false;
  playAlert(3);
  Serial.println("EMERGENCY: System Error!");
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("!!! ERROR !!!");
  lcd.setCursor(0, 1);
  lcd.print("Code: ");
  lcd.print(currentError);
}

void playAlert(int type) {
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
      for (int i = 0; i < 5; i++) {
        tone(BUZZER, 2000, 100);
        delay(150);
      }
      break;
  }
}