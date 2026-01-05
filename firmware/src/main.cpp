/**
 * DDPico - DDP to Pico LED Controller
 * 
 * Basic firmware for receiving DDP (Distributed Display Protocol) packets
 * over USB Serial and controlling WS2812B LED strips on Raspberry Pi Pico.
 * 
 * Features:
 * - Dual-core processing (Core 0: LED updates, Core 1: Serial reception)
 * - COBS framing for reliable serial communication
 * - Thread-safe circular buffer for packet handling
 * - Support for up to 480 RGB pixels per DDP packet
 * - High-speed serial (921600 baud)
 * 
 * 
 * Usage:
 * 1. Upload this firmware to Pico
 * 2. Run ddp_serial_bridge.py on PC to forward DDP packets
 * 3. Configure xLights to send DDP to bridge IP:4048
 */

#include <Arduino.h>
#include <Orb.h>
#include <DDPController.h>

// ============================================================================
// Configuration
// ============================================================================

// LED Channel Configurations (predefined pins for each channel)
// Up to 8 channels mapped to Pico GPIOs (DDP destination IDs 1-8)
const LEDChannel channelConfigs[] = {
    {43, 16, nullptr, nullptr},  // Channel 1: 43 LEDs on GP16 (default strip)
    {50, 17, nullptr, nullptr},  // Channel 2: 50 LEDs on GP17
    {50, 18, nullptr, nullptr},  // Channel 3: 50 LEDs on GP18
    {50, 19, nullptr, nullptr},  // Channel 4: 50 LEDs on GP19
    {50, 13, nullptr, nullptr},  // Channel 5: 50 LEDs on GP13
    {50, 12, nullptr, nullptr},  // Channel 6: 50 LEDs on GP12
    {50, 11, nullptr, nullptr},  // Channel 7: 50 LEDs on GP11
    {50, 10, nullptr, nullptr}   // Channel 8: 50 LEDs on GP10
};
const uint8_t numChannels = sizeof(channelConfigs) / sizeof(channelConfigs[0]);

// Serial Configuration
#define SERIAL_BAUD 921600  // High baud rate for throughput (8x faster than default)

// ============================================================================
// Global Objects
// ============================================================================

// Create DDP controller with multiple channels
DDPController ddpController(channelConfigs, numChannels);

// ============================================================================
// Setup
// ============================================================================

void setup() {
    // Initialize serial communication
    Serial.begin(SERIAL_BAUD);
    
    // Wait a moment for serial to stabilize
    delay(1000);
    
    // Print startup banner
    Serial.println();
    Serial.println("[DDPico] ========================================");
    Serial.println("[DDPico]   DDP to Pico LED Controller");
    Serial.println("[DDPico] ========================================");
    Serial.println();
    Serial.println("[DDPico] [Info] Firmware version: 1.0.0");
    Serial.print("[DDPico] [Info] Serial baud rate: ");
    Serial.println(SERIAL_BAUD);
    Serial.println("[DDPico] [Info] CPU frequency: 133 MHz");
    Serial.println();
    
    // Initialize LED channels (done in DDPController.begin())

    // Show startup animation (quick color test on all channels)
    Serial.println("[DDPico] [Info] Running LED test on all channels...");

    for (uint8_t ch = 0; ch < ddpController.getNumChannels(); ch++) {
        const LEDChannel* channels = ddpController.getChannels();
        if (channels[ch].orb) {
            Serial.print("[DDPico] [Info] Testing Channel ");
            Serial.println(ch + 1);

            Orb* orb = channels[ch].orb;

            // Red
            orb->fill(255, 0, 0);
            orb->pixelsShow();
            delay(100);

            // Green
            orb->fill(0, 255, 0);
            orb->pixelsShow();
            delay(100);

            // Blue
            orb->fill(0, 0, 255);
            orb->pixelsShow();
            delay(100);

            // Clear
            orb->clear();
        }
    }
    
    Serial.println("[DDPico] [Info] LED test complete");
    Serial.println();
    
    // Initialize DDP controller (launches Core 1)
    Serial.println("[DDPico] [Info] Starting DDP controller...");
    ddpController.begin();
    Serial.println();
    
    Serial.println("[DDPico] ========================================");
    Serial.println("[DDPico]   System Ready - Waiting for DDP data");
    Serial.println("[DDPico] ========================================");
    Serial.println();
    Serial.println("[DDPico] [Info] Listening for COBS-encoded DDP packets on USB Serial");
    Serial.println("[DDPico] [Info] Use ddp_serial_bridge.py to forward UDP packets");
    Serial.println();
}

// ============================================================================
// Main Loop (Core 0)
// ============================================================================

void loop() {
    // Process DDP packets from circular buffer
    // This reads packets received by Core 1 and updates the LEDs
    ddpController.update();
    
    // Small yield to prevent watchdog issues
    yield();
}
