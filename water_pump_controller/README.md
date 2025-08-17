# Water Detection and Pump Control System

## 🚰 Project Overview

An intelligent Arduino-based water management system that automatically monitors water levels in tanks and controls a water pump to maintain optimal water levels. The system features automatic pump control with safety mechanisms to prevent overflow and dry running.

## ✨ Key Features

- **Automatic Water Level Monitoring**: Continuously monitors tank water levels
- **Smart Pump Control**: Automatically starts/stops pump based on water levels
- **Overflow Protection**: Stops pump when tank is full to prevent overflow
- **Dry Run Protection**: Prevents pump damage by not running when tank is empty
- **Dual Sensor Support**: Works with both float switches and ultrasonic sensors
- **Manual Override**: Button control for manual pump operation
- **Visual Indicators**: LED status lights for system monitoring
- **Audio Alerts**: Buzzer notifications for critical events
- **Serial Monitor Interface**: Real-time system monitoring and control
- **Safety Features**: Pump cooldown periods and emergency shutdown

## 📋 Hardware Requirements

### Essential Components
- Arduino Uno/Nano/Mega
- Water level sensors (Choose one):
  - 2× Float switches (recommended for reliability)
  - 1× HC-SR04 Ultrasonic sensor (for continuous level monitoring)
- 1× 5V Relay module (rated for your pump current)
- 1× Water pump (DC or AC)
- 3× LEDs (Red, Green, Blue)
- 3× 220Ω resistors (for LEDs)
- 1× Push button
- 1× 10kΩ resistor (pull-up for button)
- Power supply for Arduino and pump
- Jumper wires and breadboard

### Optional Components
- Active buzzer (5V)
- LCD display (16×2 I2C)
- Voltage regulator (if using single power supply)
- Flyback diode (1N4007 for relay protection)

## 🔧 Installation

### Step 1: Hardware Setup

1. **Connect the sensors** according to the wiring diagram in `circuit_diagram.md`
2. **Wire the relay module** to control the pump
3. **Install LEDs** with appropriate resistors
4. **Connect the manual override button**
5. **Double-check all connections** before powering on

### Step 2: Software Setup

1. **Install Arduino IDE** from [arduino.cc](https://www.arduino.cc/en/software)
2. **Open the sketch**: `water_pump_controller.ino`
3. **Select your Arduino board**: Tools → Board → Arduino Uno/Nano
4. **Select the correct port**: Tools → Port → COMx (Windows) or /dev/ttyUSBx (Linux/Mac)
5. **Upload the code**: Click the Upload button (→)

### Step 3: System Configuration

Adjust these parameters in the code based on your setup:

```cpp
// Tank dimensions (for ultrasonic sensor)
const int TANK_HEIGHT_CM = 100;        // Your tank height
const int MIN_WATER_LEVEL_CM = 20;     // Minimum water level
const int MAX_WATER_LEVEL_CM = 90;     // Maximum water level

// Timing configurations
const unsigned long PUMP_COOLDOWN = 30000;  // Pump rest period (ms)
const unsigned long SENSOR_READ_INTERVAL = 500;  // Sensor reading frequency

// Sensor type selection
bool useUltrasonic = false;  // Set to true for ultrasonic sensor
```

## 📊 System States

The system operates in several states:

| State | Description | Pump Status | LED Indicator |
|-------|-------------|-------------|---------------|
| **IDLE** | Normal water level | OFF | Green |
| **EMPTY** | Low water detected | Starting | Red blinking |
| **FILLING** | Pump running | ON | Green solid |
| **FULL** | Tank full | OFF | Blue solid |
| **ERROR** | System fault | OFF | Red solid |
| **MANUAL** | Manual control | User controlled | Green blinking |

## 🎮 Usage Instructions

### Automatic Operation

1. **Power on the system** - The Arduino will initialize and read sensors
2. **System runs automatically** - No intervention needed
3. **Monitor status via LEDs**:
   - 🟢 Green: Pump running
   - 🔵 Blue: Tank full
   - 🔴 Red: Tank empty

### Manual Control

1. **Press the manual button** to enter manual mode
2. **Use serial commands** to control the pump:
   - Send 'P' to toggle pump on/off
   - Send 'M' to exit manual mode

### Serial Monitor Commands

Open Serial Monitor (9600 baud) and use these commands:

| Command | Function |
|---------|----------|
| **S** | Show system status |
| **M** | Toggle manual/auto mode |
| **P** | Toggle pump (manual mode only) |
| **U** | Switch sensor type |
| **H** | Display help menu |

### Serial Monitor Output Example
```
Water Pump Control System Starting...
System Ready!
Commands: 'S' - Status, 'M' - Toggle Manual, 'P' - Toggle Pump

State changed: EMPTY
PUMP: ON
State changed: FILLING
State changed: FULL
PUMP: OFF
```

## 🔍 Sensor Placement Guide

### Float Switch Installation
```
Tank Top ────────────────
         │ HIGH Sensor │  ← Stops pump when triggered
         │             │
         │   ~ Water ~ │
         │             │
         │ LOW Sensor  │  ← Starts pump when dry
Tank Bottom ─────────────
```

### Ultrasonic Sensor Installation
- Mount at the **top center** of tank lid
- Ensure clear path to water surface
- Keep sensor above maximum water level
- Protect from moisture and splashes

## ⚠️ Safety Guidelines

### Electrical Safety
- **Use proper insulation** for all connections
- **Install circuit breakers** for AC pumps
- **Keep electronics dry** and away from water
- **Use appropriate wire gauge** for pump current

### System Safety
- **Test with low voltage** first (use LED instead of pump)
- **Monitor first 24 hours** of operation
- **Install overflow drain** as backup
- **Regular maintenance** of sensors

### Pump Protection
- System includes **30-second cooldown** between pump cycles
- **Dry run protection** prevents pump damage
- **Emergency stop** via manual button

## 🛠️ Troubleshooting

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| **Pump won't start** | No power | Check power connections and relay |
| | Sensors not detecting | Clean sensors, check wiring |
| | In cooldown period | Wait 30 seconds |
| **Pump won't stop** | High sensor stuck | Clean or replace sensor |
| | Wiring issue | Check sensor connections |
| **False readings** | Sensor dirty | Clean sensor contacts |
| | Electrical noise | Add shielding, use twisted pairs |
| **System resets** | Power instability | Add capacitors, check supply |
| **No LED indicators** | Wiring issue | Check LED polarity and resistors |

## 📈 Advanced Features

### LCD Display Addition
Add an I2C LCD display for detailed status:
1. Connect LCD to I2C pins (A4-SDA, A5-SCL)
2. Install LiquidCrystal_I2C library
3. Uncomment LCD code sections

### WiFi Monitoring
Add ESP8266/ESP32 for remote monitoring:
1. Connect via serial to Arduino
2. Send status updates to web server
3. Control pump from smartphone

### Data Logging
Log water usage patterns:
1. Add SD card module
2. Record pump runtime and cycles
3. Analyze water consumption

## 🔄 Maintenance Schedule

| Interval | Task |
|----------|------|
| **Weekly** | Check LED indicators |
| **Monthly** | Clean sensors |
| | Test manual override |
| **Quarterly** | Inspect wiring |
| | Clean relay contacts |
| **Annually** | Replace worn components |
| | Calibrate sensors |

## 📝 Customization Options

### Adjust Water Levels
```cpp
const int MIN_WATER_LEVEL_CM = 20;  // Change minimum level
const int MAX_WATER_LEVEL_CM = 90;  // Change maximum level
```

### Change Pump Timing
```cpp
const unsigned long PUMP_COOLDOWN = 30000;  // Adjust cooldown (ms)
```

### Modify Sensor Type
```cpp
bool useUltrasonic = true;  // Switch to ultrasonic sensor
```

## 🎯 Performance Specifications

- **Response Time**: < 1 second
- **Sensor Reading Rate**: 2Hz (500ms interval)
- **Pump Cooldown**: 30 seconds minimum
- **Power Consumption**: ~50mA (Arduino) + pump current
- **Operating Voltage**: 5V (logic), 12-240V (pump)
- **Temperature Range**: 0-50°C

## 📚 Learning Resources

- [Arduino Documentation](https://www.arduino.cc/reference/en/)
- [Relay Module Guide](https://randomnerdtutorials.com/guide-for-relay-module-with-arduino/)
- [HC-SR04 Tutorial](https://randomnerdtutorials.com/complete-guide-for-ultrasonic-sensor-hc-sr04/)
- [Float Switch Basics](https://www.electronicshub.org/float-switch/)

## 🤝 Contributing

Feel free to improve this project:
1. Test with different sensors
2. Add new features
3. Improve safety mechanisms
4. Optimize code performance

## ⚖️ License

This project is open source and available for educational and personal use.

## 🙏 Acknowledgments

- Arduino community for excellent documentation
- Contributors to sensor libraries
- Water conservation initiatives

## 📞 Support

For questions or issues:
1. Check the troubleshooting section
2. Review serial monitor output
3. Verify wiring against circuit diagram
4. Test components individually

---

**Remember**: Safety first! Always disconnect power when making changes to the circuit.

**Happy Building!** 🚰🤖