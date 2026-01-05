# DDP (Distributed Display Protocol) Documentation

## Overview

DDP (Distributed Display Protocol) is a modern protocol designed specifically for controlling distributed LED lighting displays over Ethernet/UDP. Unlike Art-Net and E1.31 (which transport legacy DMX protocol over Ethernet), DDP was designed from the ground up for large-scale LED displays with high efficiency, synchronization, and large packet sizes.

**Key Advantages over DMX-based protocols:**
- Higher update rates (not limited by DMX's 40 FPS maximum)
- Larger packet sizes (up to 1440 bytes of data per packet)
- Built-in synchronization mechanisms
- Simpler protocol structure
- No multicast overhead

---

## Protocol Specifications

### Transport Layer
- **Protocol**: UDP (User Datagram Protocol)
- **Default Port**: 4048
- **Direction**: Controller → Display (receivers listen on port 4048)
- **Source Port**: Any (controllers can send from any source port)

### Packet Structure

#### Header Format
All DDP packets have a **10-byte or 14-byte header** followed by optional data payload.

**Standard 10-byte Header:**

```
Byte 0: Flags
  Bits 7-6: Version (VV) - Protocol version (01 = version 1 = 0x40)
  Bit 5: Reserved
  Bit 4: TIMECODE flag - Timecode field present (0x10)
  Bit 3: STORAGE flag - Store data to non-volatile memory (0x08)
  Bit 2: REPLY flag - Response to query (0x04)
  Bit 1: QUERY flag - Request status/configuration (0x02)
  Bit 0: PUSH flag - Signals display to update/latch data (0x01)

Byte 1: Sequence Number (0-15, wraps around)
  - 4-bit nibble used for duplicate detection
  - Ignored if zero
  - Allows receiver to ignore duplicate packets received back-to-back

Byte 2: Data Type
  Bits 7: C - Customer defined (0=standard, 1=custom)
  Bit 6: R - Reserved (should be 0)
  Bits 5-3: TTT - Data type
    000 = undefined
    001 = RGB (most common)
    010 = HSL
    011 = RGBW
    100 = grayscale
  Bits 2-0: SSS - Size in bits per pixel element
    0=undefined, 1=1bit, 2=4bit, 3=8bit, 4=16bit, 5=24bit, 6=32bit

Bytes 3-6: Destination ID (32-bit, big-endian)
  - Identifies target display/device
  - 0 = broadcast to all devices
  - 1 = default device ID

Bytes 7-10: Data Offset (32-bit, big-endian)
  - Starting channel/pixel offset for this packet's data
  - Allows splitting large displays across multiple packets
  - Byte offset into the display buffer
```

**Extended 14-byte Header** (when additional fields needed):
- Bytes 11-14: Additional control fields (implementation specific)

---

## Packet Sizes and Data Capacity

### Maximum Data Size
- **Optimal packet size**: 1440 bytes of data per packet
- **Reason**: Fits within standard Ethernet MTU (1500 bytes) including headers
  - Ethernet frame: 1500 bytes max
  - IP header: ~20 bytes
  - UDP header: 8 bytes
  - DDP header: 10 bytes
  - Remaining for data: ~1440 bytes

### LED Capacity per Packet

**RGB LEDs (3 bytes per pixel):**
- 1440 bytes ÷ 3 = **480 RGB pixels per packet**

**RGBW LEDs (4 bytes per pixel):**
- 1440 bytes ÷ 4 = **360 RGBW pixels per packet**

**For larger displays:**
- Multiple packets are sent with different data offsets
- Example: 1000 RGB LEDs = 3000 bytes
  - Packet 1: Offset 0, 1440 bytes (480 pixels)
  - Packet 2: Offset 1440, 1440 bytes (480 pixels)
  - Packet 3: Offset 2880, 120 bytes (40 pixels)
  - Final packet has PUSH flag set

---

## Channel Ordering and Addressing

### RGB Channel Order
The most common data type is RGB with 8-bit per channel (data type byte = 0x0B for RGB888).

**Standard RGB ordering:**
```
Pixel 0: [R0, G0, B0]
Pixel 1: [R1, G1, B1]
Pixel 2: [R2, G2, B2]
...
```

**Supported color orders** (implementation dependent):
- RGB (Red, Green, Blue)
- GRB (Green, Red, Blue) - common for WS2812B
- RGBW (Red, Green, Blue, White)
- HSL (Hue, Saturation, Lightness)
- Grayscale

### Addressing Model
- **Data Offset**: Specifies the starting byte position in the display buffer
- **Channel numbering**: Starts at 0
- **Calculation**: For pixel N in RGB: offset = N × 3

**Example for 1000 RGB pixels:**
- Total channels: 1000 pixels × 3 = 3000 channels
- Pixel 0: Channels 0-2 (offset 0)
- Pixel 100: Channels 300-302 (offset 300)
- Pixel 500: Channels 1500-1502 (offset 1500)

---

## Update Rates and Timing

### Frame Rates
DDP supports much higher frame rates than DMX-based protocols:

**Common frame rates in xLights:**
- **40 FPS** (25ms per frame) - High quality, smooth animations
- **20 FPS** (50ms per frame) - Standard quality, lower bandwidth
- **10 FPS** (100ms per frame) - Low quality, minimal bandwidth

**Frame rate limitations:**
- Network bandwidth
- Number of pixels per controller port
- Controller processing capability
- WiFi vs Ethernet connection

**xLights specific:**
- More than 800 pixels on a port typically limits to 20 FPS
- Under 800 pixels can achieve 40 FPS
- Ethernet connections generally support higher rates than WiFi

### Timing Behavior

**UDP Characteristics:**
- **Non-blocking**: Sender doesn't wait for acknowledgment
- **No retransmission**: Lost packets are simply dropped
- **Low latency**: Minimal protocol overhead
- **Packet loss impact**: ≤1 frame (next frame overwrites)

**Synchronization:**
- **PUSH flag**: Signals display to update/latch the frame
- **Single device**: PUSH flag set on last packet of frame
- **Multiple devices**: 
  - All devices receive their data packets
  - Broadcast packet with PUSH flag synchronizes all displays
  - Prevents visible tearing or timing issues across displays

---

## Common Usage with xLights

### Configuration in xLights

**Controller Setup:**
1. Add Ethernet controller
2. Select "DDP" as protocol
3. Configure:
   - IP address of target device
   - Starting channel
   - Number of channels
   - **Channels per packet**: 1440 (default, recommended)

**Sequence Settings:**
- Frame timing: 25ms (40 FPS) or 50ms (20 FPS)
- Network output: DDP protocol selected
- Channel mapping: 3 channels per RGB pixel

### Data Flow
```
xLights Sequence (40 FPS)
    ↓
Every 25ms: Generate frame data
    ↓
Split into 1440-byte packets
    ↓
Add DDP headers (sequence, offset, flags)
    ↓
Send via UDP to port 4048
    ↓
Set PUSH flag on last packet
    ↓
Display device receives and updates LEDs
```

### Network Considerations

**Bandwidth Calculation:**
- Example: 500 RGB pixels at 40 FPS
  - Data per frame: 500 × 3 = 1500 bytes
  - Packets per frame: 2 (1440 + 60 bytes)
  - Overhead per packet: ~38 bytes (Ethernet + IP + UDP + DDP headers)
  - Total per frame: 1500 + (2 × 38) = 1576 bytes
  - Bandwidth: 1576 × 40 = 63,040 bytes/sec ≈ 504 Kbps

**Best Practices:**
- Use wired Ethernet for large displays or high frame rates
- WiFi suitable for smaller displays (<500 pixels) at 20-40 FPS
- Minimize network hops between controller and displays
- Use dedicated network for large installations
- Consider VLAN isolation for reliability

---

## UDP Behavior and Timing Expectations

### Packet Delivery
- **Best-effort delivery**: No guaranteed delivery
- **No ordering guarantee**: Packets may arrive out of order
- **No flow control**: Sender can overwhelm receiver
- **Duplicate detection**: Sequence number helps identify duplicates

### Expected Behavior

**Normal Operation:**
- Packets arrive in order, minimal loss
- Display updates smoothly at target frame rate
- Latency: typically <5ms on local network

**Network Congestion:**
- Some packets may be dropped
- Results in occasional frame skips
- Next frame corrects any errors
- Visual impact: brief flicker or stutter

**Packet Loss Handling:**
- Receiver uses last valid data for missing packets
- Sequence number helps detect gaps
- PUSH flag ensures frame boundaries are respected
- Typically imperceptible at high frame rates

### Timing Expectations

**Latency:**
- Local network: 1-5ms
- WiFi: 5-20ms (variable)
- Processing time: <1ms for most controllers

**Jitter:**
- UDP provides no timing guarantees
- Application must handle timing
- PUSH flag provides synchronization point
- Frame rate determined by sender timing

**Synchronization:**
- Multiple displays synchronized via broadcast PUSH
- All displays latch data simultaneously
- Critical for large installations
- Prevents visible timing differences

---

## Implementation Notes

### Receiver Implementation
```c
// Pseudo-code for DDP receiver
while (true) {
    packet = receive_udp(port_4048);
    
    // Parse header
    flags = packet[0];
    sequence = packet[1] & 0x0F;
    data_type = packet[2];
    dest_id = read_uint32_be(packet[3:6]);
    offset = read_uint32_be(packet[7:10]);
    
    // Check if packet is for us
    if (dest_id == 0 || dest_id == my_id) {
        // Detect duplicates
        if (sequence != 0 && sequence == last_sequence) {
            continue; // Skip duplicate
        }
        last_sequence = sequence;
        
        // Copy data to buffer
        data = packet[10:];
        memcpy(led_buffer + offset, data, len(data));
        
        // Update display if PUSH flag set
        if (flags & 0x01) {  // PUSH flag is bit 0
            update_leds(led_buffer);
        }
    }
}
```

### Sender Implementation
```c
// Pseudo-code for DDP sender
void send_frame(pixels[], num_pixels) {
    data_size = num_pixels * 3; // RGB
    offset = 0;
    sequence = (sequence + 1) & 0x0F;
    
    while (offset < data_size) {
        chunk_size = min(1440, data_size - offset);
        is_last = (offset + chunk_size >= data_size);
        
        // Build header
        header[0] = 0x40; // Version 1 (bits 7-6 = 01)
        if (is_last) header[0] |= 0x01; // Set PUSH flag (bit 0)
        header[1] = sequence;
        header[2] = 0x0B; // RGB, 8-bit
        write_uint32_be(header[3:6], dest_id);
        write_uint32_be(header[7:10], offset);
        
        // Send packet
        packet = header + pixels[offset:offset+chunk_size];
        send_udp(dest_ip, 4048, packet);
        
        offset += chunk_size;
    }
}
```

---

## Comparison with Other Protocols

| Feature | DDP | Art-Net | E1.31 (sACN) |
|---------|-----|---------|--------------|
| **Max Frame Rate** | No limit (40+ FPS) | 40 FPS (DMX limit) | 40 FPS (DMX limit) |
| **Data per Packet** | 1440 bytes | 512 bytes (DMX universe) | 512 bytes (DMX universe) |
| **Synchronization** | Built-in PUSH flag | Separate sync packets | Separate sync packets |
| **Overhead** | Low (10 bytes) | Medium | Medium |
| **Multicast** | Optional | Common | Required |
| **Complexity** | Simple | Medium | Complex |
| **LED Focus** | Yes | No (fixture control) | No (fixture control) |

---

## References and Resources

- **Official Specification**: http://www.3waylabs.com/ddp/
- **WLED Implementation**: https://kno.wled.ge/interfaces/ddp/
- **xLights Manual**: https://manual.xlights.org/
- **OpenRGB Implementation**: GitLab merge request #2867
- **LedFx Documentation**: https://docs.ledfx.app/

---

## Summary

DDP is a purpose-built protocol for LED display control that offers:
- ✅ Simple 10-byte header structure
- ✅ Large 1440-byte data packets (480 RGB pixels)
- ✅ High frame rates (40+ FPS capable)
- ✅ Built-in synchronization via PUSH flag
- ✅ Efficient UDP transport on port 4048
- ✅ Flexible addressing with data offset
- ✅ Multiple color formats (RGB, RGBW, HSL, etc.)
- ✅ Widely supported in xLights, WLED, and other LED control software

The protocol's design makes it ideal for real-time LED animation over WiFi or Ethernet, with minimal latency and overhead compared to legacy DMX-based protocols.
