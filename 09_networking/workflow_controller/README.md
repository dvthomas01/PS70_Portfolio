# Workflow Controller

A **Seeed XIAO ESP32-C3** desk controller that connects to your Mac over **Wi‑Fi**. **Three buttons** send chord commands over **WebSocket** to a Python server that runs **macOS** actions; a **potentiometer** adjusts **system output volume** via AppleScript.

---

## Features

| Input | Behavior |
|--------|-----------|
| **3× buttons** (D5, D1, D2) | Debounced chords with a short intent window; see mapping below |
| **Potentiometer** (D0, 3.3 V) | Maps to **macOS system output volume** (0–100%) |
| **Serial** (115200) | Periodic Wi‑Fi status; `buttons:` lines when the debounced mask changes |

---

## Repository layout

| Path | Purpose |
|------|---------|
| `firmware/workflow_controller/` | Arduino sketch (`.ino`, `.cpp`, `.h`) |
| `firmware/workflow_controller/secrets.h` | Created locally from the example; Wi‑Fi and server host settings |
| `firmware/workflow_controller/secrets.h.example` | Template for `secrets.h` |
| `host/` | Python WebSocket server and action handlers |

---

## Firmware setup (Arduino IDE)

1. Add **esp32** board support (Espressif) and select **XIAO_ESP32C3**.
2. **Library Manager**: install **WebSockets** (Links2004 / Markus Sattler).  
   Install **Adafruit NeoPixel** only if you enable NeoPixel mode in `led_feedback.cpp`.
3. Copy `secrets.h.example` → `secrets.h` and set `WIFI_SSID`, `WIFI_PASSWORD`, and `WEBSOCKET_HOST` (the machine running `server.py`, same LAN as the board).
4. Open `workflow_controller.ino`, compile, and upload.

### Wiring

- **Buttons**: one side to **D5 / D1 / D2**, other to **GND** (internal pull-ups).
- **Pot**: ends to **3V3** and **GND**, wiper to **D0** (ESP32 GPIO is **3.3 V** max).

### Button → macOS action

| Buttons | Chord ID | Action |
|---------|----------|--------|
| 1 | `b1` | Open **ChatGPT** in Google Chrome |
| 2 | `b2` | Open **Visual Studio Code** |
| 3 | `b3` | Open **Autodesk Fusion** |
| 1 + 2 | `b1b2` | Open **Spotify** (web) in Chrome |
| 2 + 3 | `b2b3` | Open **YouTube** in Chrome |

URLs use `open -a "Google Chrome"` so automation prompts match the app that runs the server.

Chord timing is set in `config.h` (`kButtonDebounceMs`, `kIntentCollectionMs`).

### Pot calibration (`config.h`)

- `kPotAdcAtMin` / `kPotAdcAtMax` — raw ADC at full CCW / CW (after averaging); firmware maps this to **0–4095** for the host.
- `kPotAnalogSampleCount` — number of ADC samples averaged per reading.

### LEDs

Stock **XIAO ESP32-C3** has no onboard NeoPixel. Optional: external LED on **D10** — set `WORKFLOW_HW_DIGITAL_LED` in `led_feedback.cpp`.

---

## macOS host

Requires **Python 3.10+**.

```bash
cd host
python3 -m venv .venv
source .venv/bin/activate   # Windows: .venv\Scripts\activate
pip install -r requirements.txt
python server.py
```

Default listen address: **`0.0.0.0:8765`** (must match the port in `secrets.h`).

### Optional environment

| Variable | Purpose |
|----------|---------|
| `WORKFLOW_POT_ADC_MIN` / `WORKFLOW_POT_ADC_MAX` | Adjust host-side mapping if the firmware range is not full-scale |
| `WORKFLOW_VOLUME_VERIFY=1` | Extra logging for AppleScript volume read-back (troubleshooting) |

### Permissions

macOS may prompt for **Automation** / scripting when `osascript` runs (volume) and when opening Chrome or apps from Python.

---

## WebSocket messages

JSON text frames. Each object includes `schema_version` (same value as `kSchemaVersion` in firmware and `protocol.py`), plus:

| `type` | Fields |
|--------|--------|
| `button_chord` | `chord`: `b1`, `b2`, `b3`, `b1b2`, or `b2b3` |
| `potentiometer` | `value`: integer **0–4095** (after firmware ADC mapping) |

---

## Logging

- **Server**: INFO for startup and each chord action; warnings for malformed input or failed subprocesses; set loggers to DEBUG for WebSocket connection details.
- **ESP Serial**: Wi‑Fi status on an interval; button lines only when the debounced state changes.
