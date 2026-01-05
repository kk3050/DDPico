#pragma once

#include <Arduino.h>

/**
 * Dynamic Brightness Limiter
 *
 * Scales LED brightness based on the number of lit LEDs:
 * - Max brightness (100%) for 4 or fewer lit LEDs
 * - 40% brightness when all LEDs are lit
 * - Scales linearly between these points
 *
 * Uses efficient integer math with bit shifting for real-time performance.
 */
class BrightnessLimiter {
public:
    /**
     * Constructor
     * @param totalLEDs Total number of LEDs in the strip
     * @param maxBrightness Maximum brightness scale (0-255, default 255 for 100%)
     * @param minBrightness Minimum brightness scale (0-255, default 102 for ~40%)
     * @param threshold LED count threshold for max brightness (default 4)
     */
    BrightnessLimiter(uint16_t totalLEDs,
                     uint8_t maxBrightness = 255,
                     uint8_t minBrightness = 102,
                     uint16_t threshold = 4)
        : totalLEDs(totalLEDs),
          maxScale(maxBrightness),
          minScale(minBrightness),
          thresholdCount(threshold) {}

    /**
     * Apply brightness limiting to RGB pixel data
     * @param rgbData Pointer to RGB data (3 bytes per pixel)
     * @param pixelCount Number of pixels in the data
     */
    void limitBrightness(uint8_t* rgbData, size_t pixelCount) {
        // Count lit pixels (any RGB component > 0)
        uint16_t litCount = 0;
        for (size_t i = 0; i < pixelCount; ++i) {
            uint8_t r = rgbData[i * 3];
            uint8_t g = rgbData[i * 3 + 1];
            uint8_t b = rgbData[i * 3 + 2];
            if (r | g | b) {
                ++litCount;
            }
        }

        // Calculate brightness scale
        uint8_t scale;
        if (litCount <= thresholdCount) {
            scale = maxScale;
        } else if (litCount >= totalLEDs) {
            scale = minScale;
        } else {
            // Linear interpolation: scale = max - ((lit - thresh) * (max - min)) / (total - thresh)
            uint16_t range = totalLEDs - thresholdCount;
            uint16_t diff = litCount - thresholdCount;
            uint16_t scaleDiff = maxScale - minScale;
            scale = maxScale - ((diff * scaleDiff) / range);
        }

        // Apply scaling to each pixel 
        for (size_t i = 0; i < pixelCount; ++i) {
            uint8_t* r = &rgbData[i * 3];
            uint8_t* g = &rgbData[i * 3 + 1];
            uint8_t* b = &rgbData[i * 3 + 2];

            *r = (*r * scale) >> 8;
            *g = (*g * scale) >> 8;
            *b = (*b * scale) >> 8;
        }
    }

private:
    uint16_t totalLEDs;
    uint8_t maxScale;
    uint8_t minScale;
    uint16_t thresholdCount;
};