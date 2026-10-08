# DriveSense OBD — ESP32 Firmware

Production-quality, modular, and automotive-safe ESP32 prototype firmware for the **DriveSense OBD** automotive telemetry device.

The device plugs into a vehicle's standard OBD-II port, communicates with the vehicle ECU using **ISO 15765-4 CAN**, collects supported live telemetry parameters, and transmits normalized JSON/Binary data over **Bluetooth Low Energy (BLE)** to the **DriveSense AI Mobile Application**.

---

## 1. System Architecture

```text
VEHICLE ECU
     │
     │ CAN Bus (ISO 15765-4 / 500 kbps)
     ▼
OBD-II PORT (Pins 6: CAN_H, 14: CAN_L, 4/5: GND, 16: 12V V_BATT)
     │
     ▼
MCP2515 CAN CONTROLLER + TJA1050 TRANSCEIVER
     │
     │ SPI Bus (CS: 5, INT: 4, SCK: 18, MISO: 19, MOSI: 23)
     ▼
┌────────────────────────────────────────────────────────┐
│                   ESP32 Firmware                       │
│                                                        │
│  ┌──────────────┐     Queue      ┌──────────────────┐  │
│  │   OBD Task   │ ─────────────> │ Processing Task  │  │
│  │ (CAN/MCP2515 │                │ (Filter/Buffer/  │  │
│  │ or Simulator)│                │  Normalization)  │  │
│  └──────────────┘                └────────┬─────────┘  │
│                                           │ Queue      │
│  ┌──────────────┐                         ▼            │
│  │ System Task  │                ┌──────────────────┐  │
│  │ (Watchdog /  │                │     BLE Task     │  │
│  │  Health)     │                │  (Notify/Cmds)   │  │
│  └──────────────┘                └────────┬─────────┘  │
└───────────────────────────────────────────┼────────────┘
                                            │ BLE (GATT)
                                            ▼
                                DriveSense AI Mobile App
```

---

## 2. Directory Structure

```text
DriveSenseFirmware/
├── platformio.ini              # PlatformIO configuration (esp32dev, Arduino framework)
├── Makefile                    # Native macOS/Linux test build script
├── README.md                   # System documentation & wiring guide
├── src/
│   ├── config/
│   │   ├── HardwareConfig.h    # Centralized GPIO pins and SPI clock settings
│   │   └── AppConfig.h         # Timing, timeouts, BLE UUIDs, simulation toggle
│   ├── can/
│   │   ├── CANConfig.h         # CAN bitrates (250k, 500k, 1M) and status enums
│   │   ├── CANFrame.h          # Standard/extended 8-byte CAN frame struct
│   │   ├── CANManager.h        # MCP2515 controller abstraction & safety guard
│   │   └── CANManager.cpp
│   ├── obd/
│   │   ├── SupportedPIDs.h     # Standard SAE J1979 Mode 01 PID definitions
│   │   ├── OBDPID.h            # PID priority, units, and registry interface
│   │   ├── OBDPID.cpp
│   │   ├── OBDParser.h         # Formula calculations & bitmask decoders
│   │   ├── OBDParser.cpp
│   │   ├── OBDManager.h        # Vehicle detection & adaptive PID discovery
│   │   └── OBDManager.cpp
│   ├── telemetry/
│   │   ├── TelemetryData.h     # Normalized data struct & thread-safe ring buffer
│   │   ├── TelemetryScheduler.h# Priority-based polling interval scheduler
│   │   ├── TelemetryManager.h  # Coordination between OBD and vehicle simulator
│   │   └── TelemetryManager.cpp
│   ├── protocol/
│   │   ├── TelemetrySerializer.h # Abstract serialization interface
│   │   ├── JSONSerializer.h    # JSON serializer with explicit null for missing data
│   │   ├── JSONSerializer.cpp
│   │   ├── BinarySerializer.h  # High-efficiency packed binary protocol extension
│   │   └── BinarySerializer.cpp
│   ├── ble/
│   │   ├── BLECommands.h       # Mobile command definitions
│   │   ├── BLEService.h        # GATT UUIDs
│   │   ├── BLEManager.h        # Peripheral advertising, connection, notifications
│   │   └── BLEManager.cpp
│   ├── system/
│   │   ├── Logger.h            # Structured tagged logging ([BOOT], [CAN], [OBD], [BLE])
│   │   ├── Logger.cpp
│   │   ├── ErrorManager.h      # Error codes and statistics tracking
│   │   ├── SystemMonitor.h     # Task watchdog and memory health supervisor
│   │   └── SystemMonitor.cpp
│   └── main.cpp                # System entry point and FreeRTOS task orchestration
└── test/
    └── test_main.cpp           # Native C++ unit tests (PID math, JSON, buffers, BLE)
```

---

## 3. Hardware Configuration & Wiring

Pin assignments are centralized in [`src/config/HardwareConfig.h`](file:///Users/nandanankushgaikwad/Desktop/car%20monitor%20/src/config/HardwareConfig.h):

| MCP2515 Pin | ESP32 GPIO | Description |
|-------------|------------|-------------|
| **CS**      | `GPIO 5`   | SPI Chip Select (SS) |
| **INT**     | `GPIO 4`   | Active-low receive interrupt |
| **SCK**     | `GPIO 18`  | SPI Clock |
| **SO / MISO**| `GPIO 19` | SPI Master In / Slave Out |
| **SI / MOSI**| `GPIO 23` | SPI Master Out / Slave In |
| **VCC**     | 5V / 3.3V  | MCP2515 supply |
| **GND**     | GND        | Ground |

### OBD-II 16-Pin Connector Pinout

| OBD-II Pin | Function | Connection |
|------------|----------|------------|
| **Pin 6**  | CAN High (`CAN_H`) | MCP2515 / TJA1050 `CANH` |
| **Pin 14** | CAN Low (`CAN_L`)  | MCP2515 / TJA1050 `CANL` |
| **Pin 4**  | Chassis Ground     | ESP32 & Circuit Common GND |
| **Pin 5**  | Signal Ground      | ESP32 & Circuit Common GND |
| **Pin 16** | 12V Battery Power  | Automotive 12V->5V Buck Converter |

---

## 4. Automotive Safety Principles

The firmware strictly operates in **Read-Only Diagnostic Mode**:
* Only standard ISO 15765-4 diagnostic requests (CAN IDs `0x7DF`, `0x7E0`-`0x7E7`) are allowed.
* Arbitrary frame transmission, actuators control, ECU flash programming, or DTC clearing commands are strictly blocked at the `CANManager` and `OBDManager` layers.
* Never attempts to control throttle, braking, transmission, or safety-critical modules.

---

## 5. Supported Telemetry Parameters & Formulas

| Sensor Parameter | Mode | PID | Standard Conversion Formula | Unit | Polling Priority |
|------------------|------|-----|-----------------------------|------|------------------|
| **Engine RPM** | `01` | `0C` | `((A * 256) + B) / 4` | RPM | **HIGH** (100 ms) |
| **Vehicle Speed** | `01` | `0D` | `A` | km/h | **HIGH** (100 ms) |
| **Throttle Position** | `01` | `11` | `(A * 100) / 255` | % | **HIGH** (100 ms) |
| **Engine Load** | `01` | `04` | `(A * 100) / 255` | % | **MEDIUM** (500 ms) |
| **Mass Air Flow (MAF)** | `01` | `10` | `((A * 256) + B) / 100` | g/s | **MEDIUM** (500 ms) |
| **Coolant Temperature** | `01` | `05` | `A - 40` | °C | **LOW** (5000 ms) |
| **Intake Air Temp (IAT)** | `01` | `0F` | `A - 40` | °C | **LOW** (5000 ms) |
| **Fuel Level** | `01` | `2F` | `(A * 100) / 255` | % | **LOW** (5000 ms) |

### Adaptive PID Discovery
On vehicle connection, the device queries PID `0x00` (and `0x20`) to obtain the vehicle's supported PIDs bitmask. Only parameters explicitly supported by the vehicle's ECU are polled, preventing bus congestion and ECU timeouts.

---

## 6. Bluetooth Low Energy (BLE) Interface

* **Device Name**: `DriveSense_OBD`
* **Service UUID**: `9b20b22a-5a91-4c60-84a1-0e1d52033000`

### GATT Characteristics

| Characteristic | UUID | Properties | Purpose |
|----------------|------|------------|---------|
| **Telemetry**  | `...3001` | `READ`, `NOTIFY` | Emits live vehicle telemetry stream |
| **Command**    | `...3002` | `WRITE` | Receives commands from mobile application |
| **Status**     | `...3003` | `READ`, `NOTIFY` | Emits command responses and system status |

### Live Telemetry Stream (JSON)
Unavailable sensors are explicitly emitted as `null` rather than zero:

```json
{
  "timestamp": 1725700000,
  "speed": 62.4,
  "rpm": 2450,
  "engine_load": 48.2,
  "throttle_position": 32.5,
  "coolant_temperature": 88.0,
  "intake_air_temperature": 31.0,
  "mass_air_flow": 8.40,
  "fuel_level": null
}
```

### Mobile Application Commands

Write any of the following to the **Command Characteristic**:

1. **Start Telemetry Streaming**:
   `START_TELEMETRY`
   *Response*: `{"status":"TELEMETRY_STARTED","rate_ms":200}`

2. **Stop Telemetry Streaming**:
   `STOP_TELEMETRY`
   *Response*: `{"status":"TELEMETRY_STOPPED"}`

3. **Get Device Information**:
   `GET_DEVICE_INFO`
   *Response*:
   ```json
   {
     "device_name": "DriveSense OBD",
     "firmware_version": "1.0.0",
     "hardware_version": "1.0",
     "bluetooth_status": "CONNECTED",
     "obd_status": "CONNECTED"
   }
   ```

4. **Get Supported PIDs**:
   `GET_SUPPORTED_PIDS`
   *Response*: `{"supported_mask_01_20":"0xBE1FA813","supported_mask_21_40":"0x80002001"}`

5. **Get Connection Status**:
   `GET_CONNECTION_STATUS`
   *Response*: `{"bluetooth":"CONNECTED","obd":"CONNECTED","telemetry":"ACTIVE","simulation":true}`

6. **Toggle Simulation Mode**:
   `SET_SIMULATION_MODE false` (or `true`)
   *Response*: `{"simulation_mode":false}`

---

## 7. Development & Simulation Mode

By default, `SIMULATION_MODE = true` in [`src/config/AppConfig.h`](file:///Users/nandanankushgaikwad/Desktop/car%20monitor%20/src/config/AppConfig.h).

When enabled:
* Physical CAN hardware is **not required**.
* An integrated realistic automotive physics engine generates smooth driving cycles across four phases:
  - **Idle**: 800 RPM, 0 km/h, 0% throttle
  - **Acceleration**: RPM climbs to 3400, Speed to 65 km/h, Throttle ~45%
  - **Cruising**: RPM shifts down to 2000, steady 65–72 km/h
  - **Deceleration**: Throttle 0%, vehicle slows back down to idle
  - **Thermal & Fuel**: Coolant warms to 90°C, fuel gently depletes.
* Mobile app developers can connect over BLE and test the live charts, trip logging, and command parsing without needing a car.

---

## 8. Compilation & Verification

### Running Unit Tests Natively (macOS / Linux)
The codebase includes a native test runner that validates PID conversion formulas, ISO-TP parsing, SAE J1979 bitmasks, JSON null serialization, ring buffer overflow semantics, driving physics simulation, and BLE commands:

```bash
make test
```

### Building with PlatformIO
Connect your ESP32 board via USB:

```bash
pio run -t upload
pio device monitor -b 115200
```
