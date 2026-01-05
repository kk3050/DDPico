#pragma once
#include <Arduino.h>

/**
 * COBS (Consistent Overhead Byte Stuffing) Decoder
 * Decodes COBS-encoded frames from serial stream
 * Frame format: [COBS encoded data] 0x00
 */
class COBSDecoder {
public:
    COBSDecoder(size_t maxFrameSize = 2048)
        : maxFrameSize(maxFrameSize), framePos(0), decodedLength(0), state(STATE_WAITING) {
        frameBuffer = new uint8_t[maxFrameSize];
        decodeBuffer = new uint8_t[maxFrameSize];
    }
    
    ~COBSDecoder() {
        delete[] frameBuffer;
        delete[] decodeBuffer;
    }
    
    /**
     * Process incoming byte
     * @param byte Input byte
     * @return true if complete frame decoded
     */
    bool processByte(uint8_t byte) {
        if (byte == 0x00) {
            // Frame delimiter - decode if we have data
            if (framePos > 0) {
                size_t decoded = decode(frameBuffer, framePos, decodeBuffer, maxFrameSize);
                if (decoded > 0) {
                    decodedLength = decoded;
                    framePos = 0;
                    return true;
                }
            }
            framePos = 0;
            return false;
        }
        
        // Accumulate frame data
        if (framePos < maxFrameSize) {
            frameBuffer[framePos++] = byte;
        } else {
            // Frame too large, reset
            framePos = 0;
        }
        
        return false;
    }
    
    /**
     * Get decoded frame data
     */
    const uint8_t* getFrame() const {
        return decodeBuffer;
    }
    
    /**
     * Get decoded frame length
     */
    size_t getFrameLength() const {
        return decodedLength;
    }
    
    /**
     * Reset decoder state
     */
    void reset() {
        framePos = 0;
        decodedLength = 0;
        state = STATE_WAITING;
    }

private:
    enum State {
        STATE_WAITING,
        STATE_RECEIVING
    };
    
    /**
     * COBS decode implementation
     * @param input Encoded data
     * @param inputLen Input length
     * @param output Output buffer
     * @param outputMax Maximum output size
     * @return Decoded length, 0 on error
     */
    size_t decode(const uint8_t* input, size_t inputLen, uint8_t* output, size_t outputMax) {
        if (inputLen == 0) {
            return 0;
        }
        
        size_t inPos = 0;
        size_t outPos = 0;
        
        while (inPos < inputLen) {
            uint8_t code = input[inPos++];
            
            if (code == 0) {
                break;  // End of frame
            }
            
            // Copy (code - 1) bytes
            for (uint8_t i = 1; i < code && inPos < inputLen; i++) {
                if (outPos >= outputMax) {
                    return 0;  // Output buffer overflow
                }
                output[outPos++] = input[inPos++];
            }
            
            // Add zero byte if not at end and code < 0xFF
            if (code < 0xFF && inPos < inputLen) {
                if (outPos >= outputMax) {
                    return 0;
                }
                output[outPos++] = 0;
            }
        }
        
        return outPos;
    }
    
    uint8_t* frameBuffer;
    uint8_t* decodeBuffer;
    size_t maxFrameSize;
    size_t framePos;
    size_t decodedLength;
    State state;
};
