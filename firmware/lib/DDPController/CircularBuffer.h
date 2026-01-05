#pragma once
#include <Arduino.h>
#include <pico/mutex.h>

/**
 * Thread-safe Circular Buffer for dual-core communication
 * Core 0 writes incoming DDP packets
 * Core 1 reads and processes packets
 */
template<size_t BUFFER_SIZE>
class CircularBuffer {
public:
    CircularBuffer() : writeIndex(0), readIndex(0), count(0) {
        mutex_init(&bufferMutex);
    }
    
    /**
     * Write data to buffer (Core 0)
     * @param data Pointer to data
     * @param length Data length
     * @return true if successful, false if buffer full
     */
    bool write(const uint8_t* data, size_t length) {
        if (length == 0 || length > BUFFER_SIZE) {
            return false;
        }
        
        mutex_enter_blocking(&bufferMutex);
        
        // Check if we have space (need length + 2 bytes for size header)
        size_t required = length + 2;
        if (count + required > BUFFER_SIZE) {
            mutex_exit(&bufferMutex);
            return false;  // Buffer full
        }
        
        // Write length header (16-bit big-endian)
        buffer[writeIndex] = (length >> 8) & 0xFF;
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;
        buffer[writeIndex] = length & 0xFF;
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;
        
        // Write data
        for (size_t i = 0; i < length; i++) {
            buffer[writeIndex] = data[i];
            writeIndex = (writeIndex + 1) % BUFFER_SIZE;
        }
        
        count += required;
        
        mutex_exit(&bufferMutex);
        return true;
    }
    
    /**
     * Read data from buffer (Core 1)
     * @param data Output buffer
     * @param maxLength Maximum bytes to read
     * @return Number of bytes read, 0 if empty
     */
    size_t read(uint8_t* data, size_t maxLength) {
        mutex_enter_blocking(&bufferMutex);
        
        // Check if we have data
        if (count < 2) {
            mutex_exit(&bufferMutex);
            return 0;  // Empty
        }
        
        // Read length header
        uint8_t lenHigh = buffer[readIndex];
        readIndex = (readIndex + 1) % BUFFER_SIZE;
        uint8_t lenLow = buffer[readIndex];
        readIndex = (readIndex + 1) % BUFFER_SIZE;
        
        size_t length = ((uint16_t)lenHigh << 8) | lenLow;
        
        // Validate length
        if (length == 0 || length > maxLength || count < length + 2) {
            // Corrupted data, reset buffer
            readIndex = writeIndex;
            count = 0;
            mutex_exit(&bufferMutex);
            return 0;
        }
        
        // Read data
        for (size_t i = 0; i < length; i++) {
            data[i] = buffer[readIndex];
            readIndex = (readIndex + 1) % BUFFER_SIZE;
        }
        
        count -= (length + 2);
        
        mutex_exit(&bufferMutex);
        return length;
    }
    
    /**
     * Check if buffer has data available
     */
    bool available() {
        mutex_enter_blocking(&bufferMutex);
        bool hasData = count >= 2;
        mutex_exit(&bufferMutex);
        return hasData;
    }
    
    /**
     * Get available space in buffer
     */
    size_t availableSpace() {
        mutex_enter_blocking(&bufferMutex);
        size_t space = BUFFER_SIZE - count;
        mutex_exit(&bufferMutex);
        return space;
    }
    
    /**
     * Clear buffer
     */
    void clear() {
        mutex_enter_blocking(&bufferMutex);
        readIndex = 0;
        writeIndex = 0;
        count = 0;
        mutex_exit(&bufferMutex);
    }
    
    /**
     * Get buffer usage statistics
     */
    float getUsagePercent() {
        mutex_enter_blocking(&bufferMutex);
        float usage = (count * 100.0f) / BUFFER_SIZE;
        mutex_exit(&bufferMutex);
        return usage;
    }

private:
    uint8_t buffer[BUFFER_SIZE];
    volatile size_t writeIndex;
    volatile size_t readIndex;
    volatile size_t count;
    mutex_t bufferMutex;
};
