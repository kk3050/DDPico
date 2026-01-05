# DDP Controller

Standalone Distributed Display Protocol (DDP) controller for RP2040 Pico with dual-core processing.

## Architecture

### Dual-Core Design
- **Core 0 (Main)**: LED updates and packet processing
- **Core 1**: Serial reception and COBS decoding

### Data Flow
```
UDP (PC) → Python Bridge → USB Serial (COBS) → Core 1 → Circular Buffer → Core 0 → LEDs
```

## Components

### DDPProtocol.h
- DDP packet parser
- Validates packet structure
- Extracts pixel data

### CircularBuffer.h
- Thread-safe circular buffer (16KB)
- Mutex-protected for dual-core safety
- Handles variable-length packets

### COBSDecoder.h
- Consistent Overhead Byte Stuffing decoder
- Frames serial data with 0x00 delimiter
- Handles packet boundaries

### DDPController.h
- Main controller class
- Manages dual-core operation
- Applies pixel data to LEDs
- Statistics tracking

## DDP Protocol

### Packet Structure
```
Byte 0:    Flags (0x80 = VER, 0x02 = PUSH)
Byte 1:    Sequence number (0-15)
Byte 2:    Data type (0x01 = RGB)
Byte 3:    Destination ID
Bytes 4-7: Destination ID (32-bit)
Bytes 8-9: Data offset (big-endian)
Bytes 10-11: Data length (big-endian)
Bytes 12+: RGB pixel data
```

### Features
- Up to 480 RGB pixels per packet (1440 bytes)
- Automatic packet splitting for large displays
- Push flag for display synchronization
- Sequence numbers for packet ordering

## Usage

```cpp
#include <DDPController.h>

Orb orb(ORB_PRESET_PICO);
DDPController ddpController(orb);

void setup() {
    Serial.begin(921600);  // High baud rate for throughput
    orb.begin();
    ddpController.begin();  // Launches Core 1
}

void loop() {
    ddpController.update();  // Process packets on Core 0
}
```

## Performance

- **Baud Rate**: 921600 (8x faster than default)
- **Buffer Size**: 16KB (holds ~10 full packets)
- **Throughput**: ~100KB/s sustained
- **Latency**: <10ms typical

## Python Bridge

See `tools/ddp/ddp_serial_bridge.py` for the UDP-to-Serial bridge.

### Features
- Receives DDP packets via UDP (port 4048)
- COBS encoding for serial transmission
- Web dashboard (port 4000)
- Statistics and monitoring
- Auto-detects Pico serial port

### Usage
```bash
python tools/ddp/ddp_serial_bridge.py
```

## Statistics

The controller prints statistics every 5 seconds:
```
[DDP Info] Stats - RX: 1234 | Processed: 1230 | Dropped: 4 | Buffer: 12.5%
```

- **RX**: Packets received from serial
- **Processed**: Packets successfully applied to LEDs
- **Dropped**: Packets dropped (buffer full or invalid)
- **Buffer**: Circular buffer usage percentage

## Troubleshooting

### High Packet Loss
- Increase baud rate (921600 recommended)
- Check USB cable quality
- Reduce packet send rate

### Buffer Overflow
- Increase `DDP_CIRCULAR_BUFFER_SIZE`
- Optimize LED update code
- Reduce packet size

### Serial Errors
- Verify COBS encoding/decoding
- Check for serial port conflicts
- Ensure proper baud rate match
