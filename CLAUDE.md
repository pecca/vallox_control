# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Vallox Control is an IoT home automation system for monitoring and controlling a Vallox 150 SE MLV ventilation unit. The system consists of multiple layers:

- **C firmware** (`c/`) - Multi-threaded Raspberry Pi application that interfaces with the Vallox device via RS485/DIGIT protocol
- **Python services** (`python/`) - Data routing and sensor interfaces (InfluxDB integration)
- **Node.js API** (`vallox-control-api/`) - Express REST API for web/mobile clients
- **React web app** (`web-app/`) - Modern UI with Redux state management and WebSocket communication
- **Hardware drivers** (`BMP280_driver/`) - Bosch pressure/humidity sensor support

## Build & Development Commands

### C Firmware
```bash
cd c/
make                    # Compiles to vallox_ctrl executable
                       # Links: -lm (math), -lpthread (threading), -lbcm2835 (GPIO/I2C)
```

### Node.js API
```bash
cd vallox-control-api/
npm install
npm start              # Starts Express server on port 9000 (configurable via .env)
```

### React Web App
```bash
cd web-app/
yarn install
yarn start             # Starts both React dev server AND WebSocket server (port 3005)
                      # Uses concurrently to run: react-scripts start + node ./server/server.js
yarn build             # Production build
yarn test              # Run tests with jest
```

## Architecture & Communication Flow

### Multi-Layer Communication Stack
```
Vallox HVAC Device
    ↕ (RS485/DIGIT Protocol)
C Firmware (vallox_ctrl) - 5 Pthreads
    ↕ (UDP port 8056, JSON payloads)
Python Router (vallox_router.py)
    ↕ (TCP/UDP)
Node.js API (Express on port 9000)
    ↕ (HTTP REST + WebSocket)
React Web App
    ↕ (WebSocket port 3005)
Browser Client
```

### C Firmware Threading Model
The firmware runs 5 concurrent pthreads (see [c/main.c](c/main.c)):
1. **DS18B20 thread** - Temperature sensor reading (1-Wire)
2. **Digit receive thread** - RS485 protocol reception
3. **Digit update thread** - Protocol processing (5-second cycle)
4. **UDP server thread** - Network communication (port 8056)
5. **Control logic thread** - Main control loop for relays, resistors, defrost

All threads must be thread-safe. Shared state requires proper mutex/synchronization.

### API Communication Protocol

**REST API Endpoints** ([vallox-control-api/routes/vallox_control.js](vallox-control-api/routes/vallox_control.js)):
```
GET /api/vallox?action=get&type={type}&token={token}
GET /api/vallox?action=set&type={type}&variable={var}&value={val}&token={token}
```

**UDP Message Format** (JSON):
```javascript
// GET request
{ id: 0, get: "digit_vars" }

// SET request
{ id: 0, set: { digit_vars: { FAN_SPEED: 3 } } }
```

**UDP Timeout**: 10 seconds before "device not responding" error

### WebSocket Real-Time Updates
The web app's WebSocket server ([web-app/server/server.js](web-app/server/server.js)) bridges UDP and WebSocket:
- Listens on port 3005
- Forwards UDP messages from firmware to browser clients
- Frontend polls every 3 seconds for variable updates

## Configuration

### Environment Variables
File: [vallox-control-api/.env](vallox-control-api/.env)
```
PORT=9000                                    # API server port
VALLOX_CONTROL_PORT=8056                    # UDP port for device communication
VALLOX_CONTROL_IP=pekanraspi.duckdns.org   # Device hostname/IP
TOKEN=<optional_auth_token>                 # API authentication token
```

**Important**: The `.env` file is gitignored. Create it locally based on your deployment environment.

## State Management & Data Flow

### Redux Architecture (Web App)
- **Ducks pattern**: Redux logic in `src/ducks/` (control_vars.js, digit_vars.js)
- **Middleware**: redux-promise-middleware, redux-thunk for async operations
- **Immutability**: Uses Immutable.js for state management
- **Real-time updates**: WebSocket service polls variables every 3 seconds

### Key React Components
- `src/containers/App.js` - Main container component
- `src/components/ReadVars.js` - Display monitored variables
- `src/components/WriteVars.js` - Control interface for setting variables
- `src/components/Leds.js` - LED status indicators
- `src/services/vallox_ctrl_api.js` - WebSocket client

## Hardware Integration

### Raspberry Pi Dependencies
The C firmware requires the **bcm2835 library** for GPIO/I2C/SPI access:
- Install from: http://www.airspayce.com/mikem/bcm2835/
- Used for relay control, sensor reading, and RS485 communication

### Sensor Interfaces
- **DS18B20**: 1-Wire temperature sensors (RH1, outside, inside, exhaust, incoming)
- **BMP280**: I2C/SPI pressure and humidity sensor
- **RS485**: DIGIT protocol communication with Vallox unit

### Key Hardware Control Modules
- [c/relay_control.c](c/relay_control.c) - GPIO relay switching
- [c/defrost_resistor.c](c/defrost_resistor.c) - Defrost heating control
- [c/pre_heating_resistor.c](c/pre_heating_resistor.c) - Pre-heating management
- [c/post_heating_counter.c](c/post_heating_counter.c) - Post-heating logic
- [c/digit_protocol.c](c/digit_protocol.c) - Vallox DIGIT protocol implementation

## Code Organization

### C Firmware Structure
- **Protocol**: `digit_protocol.c/h`, `rs485.c/h`
- **Hardware**: `DS18B20.c/h`, `RPI.c/h`, `relay_control.c/h`
- **Control**: `ctrl_logic.c/h`, `*_resistor.c/h`
- **Communication**: `udp-server.c/h`, `json_codecs.c/h`
- **Utilities**: `temperature_conversion.c/h`, `jsmn.c/h` (JSON parser)

### API Structure
- `app.js` - Express application setup (body-parser, cookie-parser)
- `bin/www` - HTTP server startup, loads .env
- `routes/vallox_control.js` - All API endpoint logic

### Web App Structure
- `src/containers/` - Redux-connected smart components
- `src/components/` - Presentational components
- `src/ducks/` - Redux modules (actions, reducers, selectors)
- `src/services/` - External service interfaces (WebSocket API)
- `src/utils/` - Redux store configuration
- `server/` - WebSocket server for real-time communication

## Important Technical Details

### JSON Parsing in C
Uses **jsmn** (Jasmine) - a minimalist JSON parser:
- Token-based parsing (no DOM)
- Located in `c/jsmn.c/h`
- Wrapper in `c/json_codecs.c/h` for encoding/decoding messages

### Thread Safety
All shared data structures in the C firmware must be protected:
- Use pthread mutexes for shared state
- Be especially careful with DIGIT protocol buffers and control variables
- Temperature readings are asynchronously updated by DS18B20 thread

### Control Loop Timing
- **DIGIT update thread**: 5-second cycle for RS485 communication
- **Frontend polling**: 3-second WebSocket variable requests
- **UDP timeout**: 10 seconds before API returns error

### Legacy Code
- `php/vallox.php` - Legacy PHP interface (25KB), uses TCP sockets
- Maintained for backward compatibility but not actively developed

## Development Workflow

When making changes that span multiple layers:

1. **Firmware changes** (C): Rebuild with `make` in `c/` directory
2. **API changes** (Node.js): Restart `npm start` in `vallox-control-api/`
3. **Frontend changes** (React): Hot reload via `yarn start` in `web-app/`

For full-stack features, test the complete communication chain:
- Verify UDP messages between firmware and API (use Wireshark or tcpdump)
- Check WebSocket messages in browser DevTools (Network → WS)
- Monitor Redux state changes with Redux DevTools

## Testing

- **Frontend tests**: `yarn test` in `web-app/` (Jest + react-scripts)
- **No formal test suite** for C firmware or Node.js API currently
- Manual testing via API endpoints and web interface

## Common Pitfalls

- **WebSocket port conflicts**: Port 3005 must be free for real-time updates
- **UDP timeouts**: Device must be reachable at configured IP/port within 10s
- **Thread synchronization**: Race conditions in C firmware can cause erratic behavior
- **bcm2835 library**: Must be installed system-wide for C compilation
- **Buffer deprecation**: Node.js `Buffer` constructor is deprecated (see [web-app/server/server.js:23](web-app/server/server.js#L23))
