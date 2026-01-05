#pragma once
#include <Arduino.h>
#include <Orb.h>
#include "DDPProtocol.h"
#include "CircularBuffer.h"
#include "COBSDecoder.h"
#include "BrightnessLimiter.h"
#include <pico/multicore.h>

// Buffer size: 16KB for circular buffer (can hold ~10 full DDP packets)
#define DDP_CIRCULAR_BUFFER_SIZE (16 * 1024)

// Forward declaration
class DDPController;

// Global instance pointer for core1 access (declaration only)
extern DDPController* g_ddpController;

/**
 * DDP Controller - Standalone LED controller for DDP protocol
 * 
 * Architecture:
 * - Core 0: Main loop and LED updates (reads from buffer)
 * - Core 1: Serial receiver (writes to buffer)
 * 
 * This class is designed to be independent from the main Orb effects system
 */
class DDPController {
public:
    DDPController(Orb& orb)
        : orb(orb),
          limiter(orb.numLEDs),
          decoder(2048),
          running(false),
          packetsReceived(0),
          packetsProcessed(0),
          packetsDropped(0),
          lastStatsTime(0) {
        g_ddpController = this;
    }
    
    /**
     * Initialize DDP controller
     * Must be called from Core 0 (main core)
     */
    void begin() {
        Serial.println("[DDPico] [Info] Initializing DDP Controller...");
        
        // Clear buffer
        buffer.clear();
        
        // Reset stats
        packetsReceived = 0;
        packetsProcessed = 0;
        packetsDropped = 0;
        lastStatsTime = millis();
        
        running = true;
        
        // Launch Core 1 for serial reception
        multicore_launch_core1(core1Entry);
        
        Serial.println("[DDPico] [Info] DDP Controller initialized");
        Serial.println("[DDPico] [Info] Core 1: Serial receiver active");
        Serial.println("[DDPico] [Info] Core 0: LED processor ready");
    }
    
    /**
     * Stop DDP controller
     */
    void end() {
        running = false;
        multicore_reset_core1();
        Serial.println("[DDPico] [Info] DDP Controller stopped");
    }
    
    /**
     * Update LED display (call from Core 0 main loop)
     * Processes packets from circular buffer and updates LEDs
     */
    void update() {
         // Check if we have data in buffer
         if (!buffer.available()) {
             return;
         }
         
         // Read packet from buffer
         uint8_t packetData[2048];
         size_t packetLen = buffer.read(packetData, sizeof(packetData));
         
         if (packetLen == 0) {
             return;
         }
         
         // Parse DDP packet
         DDPPacket packet;
         if (!DDPProtocol::parsePacket(packetData, packetLen, packet)) {
             packetsDropped++;
             
             // Log detailed parse failure info with RAW HEX dump
             if (packetsDropped <= 5) {
                 Serial.print("[DDPico] ERROR: Parse failed - Len: ");
                 Serial.print(packetLen);
                 if (packetLen >= 12) {
                     Serial.print(", Flags: 0x");
                     Serial.print(packetData[0], HEX);
                     Serial.print(", Seq: ");
                     Serial.print(packetData[1]);
                     Serial.print(", Type: 0x");
                     Serial.print(packetData[2], HEX);
                     Serial.print(", Dest: ");
                     Serial.print(packetData[3]);
                     Serial.print(", Offset: ");
                     Serial.print(((uint16_t)packetData[8] << 8) | packetData[9]);
                     Serial.print(", DataLen: ");
                     Serial.print(((uint16_t)packetData[10] << 8) | packetData[11]);
                     
                     // Show first 16 bytes in hex
                     Serial.print("\n[DDPico] RAW HEX: ");
                     for (int i = 0; i < min(16, (int)packetLen); i++) {
                         if (packetData[i] < 0x10) Serial.print("0");
                         Serial.print(packetData[i], HEX);
                         Serial.print(" ");
                     }
                 }
                 Serial.println();
             }
             return;
         }
         
         packetsProcessed++;
         
         // Log ALL processed packets for debugging
         Serial.print("[DDPico] ✓ Processing packet #");
         Serial.print(packetsProcessed);
         Serial.print(" - Offset: ");
         Serial.print(packet.dataOffset);
         Serial.print(", Length: ");
         Serial.print(packet.dataLength);
         Serial.print(", Push: ");
         Serial.println(packet.shouldPush() ? "YES" : "NO");
         
         // Apply pixel data to LEDs
         applyPixelData(packet);
         
         // Push to display if requested
         if (packet.shouldPush()) {
             Serial.println("[DDPico] ✓ Calling pixelsShow() to update LEDs");
             orb.pixelsShow();
             Serial.println("[DDPico] ✓ pixelsShow() completed");
         } else {
             Serial.println("[DDPico] ⚠ Push flag NOT set - LEDs not updated");
         }
         
         // Print stats periodically
         if (millis() - lastStatsTime >= 5000) {
             printStats();
             lastStatsTime = millis();
         }
    }
    
    /**
     * Get statistics
     */
    void getStats(uint32_t& rx, uint32_t& processed, uint32_t& dropped) {
        rx = packetsReceived;
        processed = packetsProcessed;
        dropped = packetsDropped;
    }
    
    /**
     * Core 1 main loop - receives serial data and writes to buffer
     * Called from static entry point
     */
    void core1Loop() {
        uint32_t lastAckTime = 0;
        uint32_t lastAckCount = 0;
        
        while (running) {
            // Check for serial data
            while (Serial.available() > 0) {
                uint8_t byte = Serial.read();
                
                // Process byte through COBS decoder
                if (decoder.processByte(byte)) {
                    // Complete frame decoded
                    const uint8_t* frame = decoder.getFrame();
                    size_t frameLen = decoder.getFrameLength();
                    
                    // Write to circular buffer
                    if (buffer.write(frame, frameLen)) {
                        packetsReceived++;
                        
                        // Send acknowledgment for first few packets
                        if (packetsReceived <= 5) {
                            Serial.print("[DDPico] ACK: Packet #");
                            Serial.print(packetsReceived);
                            Serial.print(" received (");
                            Serial.print(frameLen);
                            Serial.println(" bytes)");
                        }
                    } else {
                        packetsDropped++;
                        // Log buffer overflow
                        Serial.println("[DDPico] WARN: Buffer full - packet dropped");
                    }
                }
            }
            
            // Send periodic acknowledgment every 100 packets
            uint32_t currentTime = millis();
            if (packetsReceived > lastAckCount && (currentTime - lastAckTime) >= 1000) {
                uint32_t newPackets = packetsReceived - lastAckCount;
                Serial.print("[DDPico] ACK: ");
                Serial.print(newPackets);
                Serial.print(" packets received in last ");
                Serial.print((currentTime - lastAckTime) / 1000.0, 1);
                Serial.println("s");
                lastAckTime = currentTime;
                lastAckCount = packetsReceived;
            }
            
            // Small delay to prevent tight loop
            tight_loop_contents();
        }
    }

private:
    /**
     * Core 1 entry point (serial receiver)
     */
    static void core1Entry() {
        if (g_ddpController) {
            g_ddpController->core1Loop();
        }
    }
    
    /**
     * Apply DDP pixel data to LEDs
     */
    void applyPixelData(const DDPPacket& packet) {
         uint16_t pixelCount = DDPProtocol::getPixelCount(packet);
         uint16_t startPixel = packet.dataOffset / 3;  // Offset is in bytes, convert to pixels
         
         // Log ALL pixel applications for debugging
         Serial.print("[DDPico] Applying pixels - Start: ");
         Serial.print(startPixel);
         Serial.print(", Count: ");
         Serial.print(pixelCount);
         Serial.print(", Total LEDs: ");
         Serial.println(orb.numLEDs);
         
         // Bounds check
         if (startPixel >= orb.numLEDs) {
             Serial.print("[DDPico] WARN: Start pixel ");
             Serial.print(startPixel);
             Serial.print(" >= LED count ");
             Serial.println(orb.numLEDs);
             return;
         }
         
         // Limit to available LEDs
         if (startPixel + pixelCount > orb.numLEDs) {
             pixelCount = orb.numLEDs - startPixel;
         }
         
         // Apply pixel data
         // Apply brightness limiting
          limiter.limitBrightness((uint8_t*)packet.data, pixelCount);

          const uint8_t* data = packet.data;
         for (uint16_t i = 0; i < pixelCount; i++) {
             uint16_t pixelIndex = startPixel + i;
             uint8_t r = data[i * 3];
             uint8_t g = data[i * 3 + 1];
             uint8_t b = data[i * 3 + 2];
             
             orb.pixelSet(pixelIndex, r, g, b);
             
             // Log first pixel of first packet
             if (packetsProcessed == 1 && i == 0) {
                 Serial.print("[DDPico] First pixel RGB: (");
                 Serial.print(r);
                 Serial.print(", ");
                 Serial.print(g);
                 Serial.print(", ");
                 Serial.print(b);
                 Serial.println(")");
             }
         }
    }
    
    /**
     * Print statistics to serial
     */
    void printStats() {
        float bufferUsage = buffer.getUsagePercent();
        
        Serial.print("[DDPico] Stats - RX: ");
        Serial.print(packetsReceived);
        Serial.print(" | Processed: ");
        Serial.print(packetsProcessed);
        Serial.print(" | Dropped: ");
        Serial.print(packetsDropped);
        Serial.print(" | Buffer: ");
        Serial.print(bufferUsage, 1);
        Serial.println("%");
    }
    
    Orb& orb;
    BrightnessLimiter limiter;
    CircularBuffer<DDP_CIRCULAR_BUFFER_SIZE> buffer;
    COBSDecoder decoder;
    
    volatile bool running;
    volatile uint32_t packetsReceived;
    volatile uint32_t packetsProcessed;
    volatile uint32_t packetsDropped;
    uint32_t lastStatsTime;
};
