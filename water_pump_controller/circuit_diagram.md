# Water Pump Controller - Circuit Diagram

## Components Required

### Essential Components:
1. **Arduino Board** (Uno/Nano/Mega)
2. **Water Level Sensors** (2x Float switches OR 1x HC-SR04 Ultrasonic sensor)
3. **5V Relay Module** (Single channel, rated for pump current)
4. **Water Pump** (12V/24V DC or 110V/220V AC with appropriate relay)
5. **Power Supply** (For Arduino and pump)
6. **LEDs** (3x - Red, Green, Yellow)
7. **Resistors** (3x 220Ω for LEDs)
8. **Push Button** (For manual override)
9. **10kΩ Resistor** (Pull-up for button)
10. **Jumper Wires**
11. **Breadboard or PCB**

### Optional Components:
1. **Buzzer** (5V active buzzer)
2. **LCD Display** (16x2 I2C)
3. **Voltage Regulator** (If pump voltage differs from Arduino)
4. **Diode** (1N4007 for relay protection)
5. **Capacitors** (100µF for power filtering)

## Wiring Diagram - Float Switch Version

```
                    ARDUINO UNO/NANO
                    ┌─────────────────┐
                    │                 │
                    │  [USB Port]     │
                    │                 │
                    │ D2 ─────────────┼──────[Float Switch LOW]──── GND
                    │                 │
                    │ D3 ─────────────┼──────[Float Switch HIGH]─── GND
                    │                 │
                    │ D4 ─────────────┼──────[Relay IN]
                    │                 │
                    │ D5 ─────[220Ω]──┼──────[LED Green]────────── GND
                    │                 │
                    │ D6 ─────[220Ω]──┼──────[LED Blue]─────────── GND
                    │                 │
                    │ D7 ─────[220Ω]──┼──────[LED Red]──────────── GND
                    │                 │
                    │ D8 ─────[10kΩ]──┼──┬───[Button]──────────── GND
                    │                 │  └── +5V
                    │ D9 ─────────────┼──────[Buzzer +]─────────── GND
                    │                 │
                    │ 5V ─────────────┼──────[Relay VCC]
                    │                 │
                    │ GND ────────────┼──────[Relay GND]
                    │                 │
                    └─────────────────┘

         RELAY MODULE                    WATER PUMP
         ┌──────────┐                   ┌─────────┐
         │   5V     │                   │         │
         │   GND    │                   │  PUMP   │
         │   IN ←───┼─── From D4        │         │
         │          │                   │   ┌─┐   │
         │   COM ───┼───────────────────┼───┤ ├───┼─── AC/DC Power (+)
         │   NO  ───┼───────────────────┼───┤ ├───┼─── To Pump
         │   NC     │                   │   └─┘   │
         └──────────┘                   └─────────┘
```

## Wiring Diagram - Ultrasonic Sensor Version

```
                    ARDUINO UNO/NANO
                    ┌─────────────────┐
                    │                 │
                    │  [USB Port]     │
                    │                 │
                    │ D10 ────────────┼──────[HC-SR04 Trig]
                    │                 │
                    │ D11 ────────────┼──────[HC-SR04 Echo]
                    │                 │
                    │ D4 ─────────────┼──────[Relay IN]
                    │                 │
                    │ D5 ─────[220Ω]──┼──────[LED Green]────────── GND
                    │                 │
                    │ D6 ─────[220Ω]──┼──────[LED Blue]─────────── GND
                    │                 │
                    │ D7 ─────[220Ω]──┼──────[LED Red]──────────── GND
                    │                 │
                    │ D8 ─────[10kΩ]──┼──┬───[Button]──────────── GND
                    │                 │  └── +5V
                    │ D9 ─────────────┼──────[Buzzer +]─────────── GND
                    │                 │
                    │ 5V ─────────────┼──┬───[HC-SR04 VCC]
                    │                 │  └───[Relay VCC]
                    │                 │
                    │ GND ────────────┼──┬───[HC-SR04 GND]
                    │                 │  └───[Relay GND]
                    └─────────────────┘

         HC-SR04 ULTRASONIC            
         ┌──────────────┐              
         │    ○    ○    │ <- Transducers
         │              │              
         │ VCC          │              
         │ Trig         │              
         │ Echo         │              
         │ GND          │              
         └──────────────┘              
```

## Pin Connections Table

| Arduino Pin | Component | Wire Color (Suggested) | Notes |
|------------|-----------|------------------------|-------|
| D2 | Float Switch LOW | Yellow | Active LOW (pulled high internally) |
| D3 | Float Switch HIGH | Orange | Active LOW (pulled high internally) |
| D4 | Relay IN | Blue | Controls pump power |
| D5 | LED Green (Pump ON) | Green | Through 220Ω resistor |
| D6 | LED Blue (Tank Full) | Blue | Through 220Ω resistor |
| D7 | LED Red (Tank Empty) | Red | Through 220Ω resistor |
| D8 | Manual Button | White | With 10kΩ pull-up to 5V |
| D9 | Buzzer | Purple | Optional alert system |
| D10 | HC-SR04 Trig | Brown | Ultrasonic trigger |
| D11 | HC-SR04 Echo | Gray | Ultrasonic echo |
| 5V | Power Rail | Red | Powers sensors and relay |
| GND | Ground Rail | Black | Common ground |

## Float Switch Installation

```
    Water Tank Cross-Section
    ┌─────────────────────┐
    │                     │ <- Tank Top
    │   [HIGH Sensor]────┼──── D3 (Tank Full Detection)
    │                     │
    │                     │ 
    │    ~~~Water~~~      │ <- Water Level
    │                     │
    │                     │
    │   [LOW Sensor]─────┼──── D2 (Low Water Detection)
    │                     │
    └─────────────────────┘ <- Tank Bottom
```

## Ultrasonic Sensor Installation

```
    Water Tank Top View
    ┌─────────────────────┐
    │   [HC-SR04 Sensor]  │ <- Mounted on tank lid
    │         ↓↓↓         │    pointing downward
    │                     │
    │                     │
    │    ~~~Water~~~      │ <- Measures distance to water
    │                     │
    │                     │
    └─────────────────────┘
```

## Relay Wiring for Different Pump Types

### DC Pump (12V/24V)
```
Power Supply (+) ──────┬──── Relay COM
                       │
                  Pump Motor
                       │
Power Supply (-) ──────┘
                 
Relay NO ──────────────── Pump (+)
```

### AC Pump (110V/220V)
```
AC Live ────────────────── Relay COM
                           
Relay NO ───────────────── Pump Live Terminal

AC Neutral ─────────────── Pump Neutral Terminal

⚠️ WARNING: AC voltage is dangerous! 
   Use proper insulation and safety measures.
```

## Power Supply Configuration

### Option 1: Separate Supplies
```
┌──────────────┐        ┌──────────────┐
│   9V/12V     │        │  12V/24V     │
│   Adapter    │        │  Pump Supply │
│      ↓       │        │      ↓       │
│   Arduino    │        │    Pump      │
└──────────────┘        └──────────────┘
```

### Option 2: Single Supply with Voltage Regulator
```
┌──────────────────┐
│   12V/24V        │
│   Power Supply   │
│        ↓         │
│    ┌───┴───┐     │
│    │  LM7805│────┼──── 5V to Arduino
│    └───────┘     │
│        ↓         │
│      Pump        │
└──────────────────┘
```

## Safety Considerations

1. **Electrical Safety**
   - Use appropriate gauge wires for pump current
   - Install fuses/circuit breakers
   - Ensure proper grounding
   - Keep electronics away from water

2. **Relay Protection**
   - Add flyback diode (1N4007) across relay coil
   - Use optocoupler for additional isolation
   - Choose relay with appropriate current rating

3. **Sensor Protection**
   - Waterproof all connections
   - Use sealed float switches
   - Mount ultrasonic sensor above maximum water level

4. **System Protection**
   - Add emergency stop button
   - Include overflow sensor
   - Implement software timeouts
   - Add thermal protection for pump

## PCB Layout Suggestion

```
┌─────────────────────────────────────┐
│  ○ ○ ○ ○  [ARDUINO]  ○ ○ ○ ○ ○ ○  │
│                                     │
│  [RELAY]     [LEDs]     [BUZZER]   │
│    ███        ●●●          ♪       │
│                                     │
│  Terminal Blocks:                   │
│  ▣ ▣  Sensors                       │
│  ▣ ▣  Pump                          │
│  ▣ ▣  Power                         │
└─────────────────────────────────────┘
```

## Troubleshooting Guide

| Problem | Possible Cause | Solution |
|---------|---------------|----------|
| Pump won't start | No power | Check power connections |
| | Relay faulty | Test relay with multimeter |
| | Arduino not sending signal | Check D4 output with LED |
| Pump won't stop | Sensor stuck | Clean/replace sensor |
| | Software bug | Check serial monitor |
| False readings | Sensor dirty | Clean sensor |
| | Loose connections | Check wiring |
| System resets | Power issues | Add capacitors, check supply |

## Testing Procedure

1. **Dry Test**
   - Power system without water
   - Manually trigger sensors
   - Verify LED indicators
   - Check relay clicking

2. **Wet Test**
   - Use small container first
   - Test with actual water
   - Verify all water levels
   - Check automatic operation

3. **Full System Test**
   - Install in actual tank
   - Run for 24 hours
   - Monitor for issues
   - Adjust parameters as needed