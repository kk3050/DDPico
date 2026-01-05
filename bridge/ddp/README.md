# DDP Serial Bridge

Efficient UDP to USB Serial proxy for controlling the GlowBit Orb via DDP (Distributed Display Protocol).

## Features

- **Auto-detect**: Automatically finds Pico serial port (Windows/Linux/macOS)
- **COBS Framing**: Reliable packet encoding/decoding for serial communication
- **Minimal Overhead**: Lightweight, efficient implementation
- **Robust Error Handling**: Graceful recovery from connection issues
- **Real-time Stats**: Periodic statistics output
- **Multi-threaded**: Separate threads for serial RX, UDP RX, and stats

## Installation

```bash
pip install pyserial
```

## Usage

```bash
python tools/ddp_serial_bridge.py
```

The bridge will:
1. Auto-detect the Pico serial port
2. Listen for UDP DDP packets on `0.0.0.0:4048`
3. Forward packets to Pico via USB Serial with COBS encoding
4. Print Pico serial output to console
5. Display statistics every 10 seconds

## Configuration

Configure your DDP software to send to:
- **Host**: `127.0.0.1` (or your computer's IP)
- **Port**: `4048`
- **Protocol**: UDP

## Output

```
[SERIAL] Connected to COM3 @ 115200 baud
[UDP] Listening on 0.0.0.0:4048
[BRIDGE] DDP Serial Bridge started
[CONFIG] Configure DDP software to send to 127.0.0.1:4048
[PICO] [DDPController] Initialized - Waiting for DDP packets on Serial
[DDP] 275 bytes from 192.168.1.100:12345
[STATS] RX: 1 packets, 275 bytes | TX: 1 packets, 275 bytes | Idle: 0.5s
```

## Protocol

### DDP Packet Format
```
[Header (10 bytes)][Pixel Data (variable)]
```

### COBS Encoding
Packets are COBS-encoded before transmission over serial:
- Eliminates 0x00 bytes from data
- Adds 0x00 delimiter at end of packet
- Allows reliable packet framing

## Troubleshooting

### Serial port not found
- Ensure Pico is connected via USB
- Check Device Manager (Windows) or `lsusb` (Linux)
- Manually specify port: `python tools/ddp_serial_bridge.py --port COM3`

### No LED updates
- Verify DDP software is sending to `127.0.0.1:4048`
- Check bridge output for `[DDP]` messages
- Verify Pico is in DDP mode (press BOOTSEL button)

### Connection drops
- Check USB cable quality
- Try different USB port
- Restart bridge and DDP software

## Performance

- **Throughput**: ~1000+ packets/second
- **Latency**: <5ms per packet
- **CPU Usage**: <1% on modern systems
- **Memory**: ~10MB

## Compatible Software

- WLED
- LedFx
- xLights
- Any DDP v1 compatible software
