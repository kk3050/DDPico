# DDPico - DDP to Pico LED Controller

DDPico is a complete solution for controlling WS2812B LED strips on Raspberry Pi Pico using the Distributed Display Protocol (DDP). It consists of firmware for the Pico microcontroller and a Python bridge for forwarding DDP packets from lighting software like xLights.

## Features

- **High-Performance Firmware**: Dual-core Pico firmware with COBS framing for reliable serial communication
- **Python Bridge**: UDP to USB Serial proxy with real-time web dashboard
- **DDP Protocol Support**: Full DDP v1 implementation with pixel data handling
- **Brightness Limiting**: Hardware-safe brightness control to prevent overdriving LEDs
- **Web Dashboard**: Monitor bridge status and logs in real-time

## Components

- [`firmware/`](./firmware/) - PlatformIO project for Raspberry Pi Pico
- [`bridge/`](./bridge/) - Python serial bridge and web interface
- [`docs/`](./docs/) - Protocol documentation and guides

## Quick Start

1. **Flash Firmware**: Upload the firmware to your Pico using PlatformIO
2. **Run Bridge**: Execute `python ddp_serial_bridge.py` on your PC
3. **Configure Software**: Point xLights or other DDP software to `127.0.0.1:4048`
4. **Monitor**: Open http://localhost:4000 for the web dashboard

## Requirements

- Raspberry Pi Pico
- WS2812B LED strip
- Python 3.6+ with `pyserial` and `websocket` libraries
- PlatformIO for firmware development

## License

This project is licensed under a custom license that allows free personal and non-commercial use. See [LICENSE](./LICENSE) for details. Commercial use requires explicit permission.

## Contributing

Contributions welcome! Please see [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.