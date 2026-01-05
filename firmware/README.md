# Firmware

This directory contains the PlatformIO project for the Raspberry Pi Pico firmware.

## Overview

The firmware implements a DDP controller that receives DDP packets over USB Serial (with COBS framing) and drives WS2812B LEDs. It uses dual-core processing:

- **Core 0**: Main loop, LED updates
- **Core 1**: Serial reception, packet processing

## Files

- `src/main.cpp` - Main firmware entry point
- `lib/DDPController/` - DDP protocol handling library
- `lib/Orb/` - LED strip control library
- `platformio.ini` - PlatformIO configuration

## Building and Flashing

1. Install PlatformIO
2. Open this directory in PlatformIO
3. Build and upload to Pico

## Configuration

Edit `src/main.cpp` to configure:
- Number of LEDs (`NUM_LEDS`)
- LED pin (`LED_PIN`)
- Serial baud rate (`SERIAL_BAUD`)

## Dependencies

- Arduino framework for Pico
- Custom libraries: DDPController, Orb