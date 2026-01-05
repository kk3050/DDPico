#pragma once
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Orb preset configurations
#define ORB_PRESET_PICO 0

/**
 * Orb - Simple LED controller wrapper for NeoPixel strips
 * Provides a simple interface for controlling WS2812B LED strips
 */
class Orb {
public:
    /**
     * Constructor
     * @param preset Preset configuration (ORB_PRESET_PICO)
     * @param numLEDs Number of LEDs in the strip
     * @param pin GPIO pin for LED data
     */
    Orb(uint8_t preset = ORB_PRESET_PICO, uint16_t numLEDs = 100, uint8_t pin = 16)
        : numLEDs(numLEDs), pin(pin), brightness(255) {
        // Create NeoPixel object (GRB for WS2812B)
        pixels = new Adafruit_NeoPixel(numLEDs, pin, NEO_GRB + NEO_KHZ800);
    }
    
    /**
     * Initialize the LED strip
     */
    void begin() {
        if (pixels) {
            pixels->begin();
            pixels->setBrightness(brightness);
            pixels->show(); // Initialize all pixels to 'off'
            Serial.println("[Orb Info] LED strip initialized");
            Serial.print("[Orb Info] Number of LEDs: ");
            Serial.println(numLEDs);
            Serial.print("[Orb Info] Data pin: ");
            Serial.println(pin);
        }
    }
    
    /**
     * Set a single pixel color
     * @param index Pixel index (0-based)
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    void pixelSet(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
        if (pixels && index < numLEDs) {
            pixels->setPixelColor(index, pixels->Color(r, g, b));
        }
    }
    
    /**
     * Update the LED strip display
     * Call this after setting pixel colors to show changes
     */
    void pixelsShow() {
        if (pixels) {
            pixels->show();
        }
    }
    
    /**
     * Clear all pixels (set to black)
     */
    void clear() {
        if (pixels) {
            pixels->clear();
            pixels->show();
        }
    }
    
    /**
     * Set global brightness
     * @param brightness Brightness value (0-255)
     */
    void setBrightness(uint8_t brightness) {
        this->brightness = brightness;
        if (pixels) {
            pixels->setBrightness(brightness);
        }
    }
    
    /**
     * Get pixel color
     * @param index Pixel index
     * @return 32-bit color value
     */
    uint32_t getPixelColor(uint16_t index) {
        if (pixels && index < numLEDs) {
            return pixels->getPixelColor(index);
        }
        return 0;
    }
    
    /**
     * Fill all pixels with a color
     * @param r Red value (0-255)
     * @param g Green value (0-255)
     * @param b Blue value (0-255)
     */
    void fill(uint8_t r, uint8_t g, uint8_t b) {
        if (pixels) {
            pixels->fill(pixels->Color(r, g, b));
        }
    }
    
    /**
     * Destructor
     */
    ~Orb() {
        if (pixels) {
            delete pixels;
        }
    }
    
    // Public members
    uint16_t numLEDs;
    uint8_t pin;
    
private:
    Adafruit_NeoPixel* pixels;
    uint8_t brightness;
};
