# Vallox Control API

Express/Node.js REST API for monitoring and controlling a Vallox 150 SE MLV ventilation unit. This API acts as a gateway between web/mobile clients and the Vallox firmware (via UDP).

## Prerequisites

- Node.js (v12+ recommended)
- Access to the Vallox control device (firmware running on Raspberry Pi)

## Installation

```bash
cd vallox-control-api
npm install
```

## Configuration

Create a `.env` file in the root directory:

```env
PORT=9000
VALLOX_CONTROL_PORT=8056
VALLOX_CONTROL_IP=pekanraspi.duckdns.org
TOKEN=your_secret_token
```

- `PORT`: The port this API server listens on.
- `VALLOX_CONTROL_PORT`: The UDP port the Vallox firmware is listening on.
- `VALLOX_CONTROL_IP`: The IP address or hostname of the device running the firmware.
- `TOKEN`: Authentication token required for API requests.

## Usage

Start the server:

```bash
npm start
```

## API Endpoints

All endpoints require a `token` query parameter.

### GET /api/vallox

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `action` | `string` | Must be `get`. |
| `type` | `string` | The variable type to retrieve (e.g., `digit_vars`). |
| `token` | `string` | Authentication token. |

**Example:**
`GET /api/vallox?action=get&type=digit_vars&token=your_token`

### SET /api/vallox

| Parameter | Type | Description |
| :--- | :--- | :--- |
| `action` | `string` | Must be `set`. |
| `type` | `string` | The variable type to set (e.g., `digit_vars`). |
| `variable` | `string` | The specific variable name (e.g., `FAN_SPEED`). |
| `value` | `number` | The value to set. |
| `token` | `string` | Authentication token. |

**Example:**
`GET /api/vallox?action=set&type=digit_vars&variable=FAN_SPEED&value=3&token=your_token`
