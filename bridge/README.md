# Bridge

This directory contains the Python UDP-to-Serial bridge and web dashboard.

## Overview

The bridge forwards DDP packets from UDP (typically from lighting software) to the Pico via USB Serial using COBS framing. It includes a web interface for monitoring and configuration.

## Files

- `ddp/ddp_serial_bridge.py` - Main bridge script
- `ddp/web/` - Web dashboard files
- `ddp/start_bridge.bat` - Windows batch file to start the bridge
- `ddp/XLIGHTS_TESTING_GUIDE.md` - Testing guide for xLights integration

## Running the Bridge

```bash
python ddp/ddp_serial_bridge.py
```

Or use the batch file on Windows.

## Web Dashboard

Access at http://localhost:4000 after starting the bridge.

Features:
- Real-time statistics
- Log viewer
- Tweening controls
- Connection status

## Configuration

The bridge auto-detects the Pico serial port. Manual configuration available in the script.

## Requirements

- Python 3.6+
- pyserial
- websocket (for web interface)