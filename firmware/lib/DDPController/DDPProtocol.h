#pragma once
#include <Arduino.h>

// DDP Protocol Constants
#define DDP_HEADER_SIZE 10
#define DDP_MAX_PACKET_SIZE 1440  // Max data per packet (480 RGB pixels)
#define DDP_ID_DEFAULT 1
#define DDP_ID_BROADCAST 0

// DDP Flags (byte 0)
#define DDP_FLAG_VER_MASK   0xC0  // Version mask (bits 7-6)
#define DDP_FLAG_VER1       0x40  // Version 1 (bits 7-6 = 01)
#define DDP_FLAG_TIMECODE   0x10  // Timecode present (bit 4)
#define DDP_FLAG_STORAGE    0x08  // Storage type (bit 3)
#define DDP_FLAG_REPLY      0x04  // Reply flag (bit 2)
#define DDP_FLAG_QUERY      0x02  // Query packet (bit 1)
#define DDP_FLAG_PUSH       0x01  // Push to display (bit 0)

// DDP Data Types (byte 2)
#define DDP_TYPE_RGB        0x01  // RGB data

/**
 * DDP Packet Structure (10-byte header)
 *
 * Byte 0: Flags
 * Byte 1: Sequence (0-15) + reserved
 * Byte 2: Data type (0x00 or 0x01 = RGB)
 * Byte 3: Destination ID
 * Bytes 4-7: Data offset (32-bit big-endian)
 * Bytes 8-9: Data length (16-bit big-endian)
 * Bytes 10+: Pixel data
 */
struct DDPPacket {
    uint8_t flags;
    uint8_t sequence;
    uint8_t dataType;
    uint8_t destId;
    uint32_t dataOffset;    // 32-bit offset (bytes 4-7)
    uint16_t dataLength;    // 16-bit length (bytes 8-9)
    const uint8_t* data;
    
    bool isValid() const {
        // Check version is 1 (bits 7-6 should be 01 = 0x40)
        // Accept dataType 0x00 (undefined, used by xLights) or 0x01 (RGB)
        return ((flags & DDP_FLAG_VER_MASK) == DDP_FLAG_VER1) &&
               (dataType == 0x00 || dataType == DDP_TYPE_RGB) &&
               dataLength > 0 &&
               dataLength <= DDP_MAX_PACKET_SIZE;
    }
    
    bool shouldPush() const {
        return flags & DDP_FLAG_PUSH;
    }
};

/**
 * DDP Protocol Parser
 * Parses raw DDP packets from byte stream
 */
class DDPProtocol {
public:
    /**
     * Parse DDP packet from buffer
     * @param buffer Raw packet data
     * @param length Buffer length
     * @param packet Output packet structure
     * @return true if packet is valid
     */
    static bool parsePacket(const uint8_t* buffer, size_t length, DDPPacket& packet) {
        if (length < DDP_HEADER_SIZE) {
            return false;
        }
        
        // Parse header
        packet.flags = buffer[0];
        packet.sequence = buffer[1] & 0x0F;
        packet.dataType = buffer[2];
        packet.destId = buffer[3];
        
        // Parse 32-bit data offset (bytes 4-7, big-endian)
        packet.dataOffset = ((uint32_t)buffer[4] << 24) |
                           ((uint32_t)buffer[5] << 16) |
                           ((uint32_t)buffer[6] << 8) |
                           buffer[7];
        
        // Parse 16-bit data length (bytes 8-9, big-endian)
        packet.dataLength = ((uint16_t)buffer[8] << 8) | buffer[9];
        
        // Validate packet
        if (!packet.isValid()) {
            return false;
        }
        
        // Check if we have enough data
        if (length < DDP_HEADER_SIZE + packet.dataLength) {
            return false;
        }
        
        // Point to pixel data (starts at byte 10)
        packet.data = buffer + 10;
        
        return true;
    }
    
    /**
     * Calculate number of pixels in packet
     */
    static uint16_t getPixelCount(const DDPPacket& packet) {
        return packet.dataLength / 3;  // RGB = 3 bytes per pixel
    }
};
