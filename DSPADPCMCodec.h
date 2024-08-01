//
// Created by cookieso on 18.07.24.
//
#pragma once

#include <cstdint>

// From Citric-Composer
static int8_t nibbleToSbyte[] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};

inline int8_t GetHighNibble(const uint8_t value) {
    return nibbleToSbyte[value >> 4 & 0xF];
}

inline int8_t GetLowNibble(const uint8_t value) {
    return nibbleToSbyte[value & 0xF];
}

inline short Clamp16(const int value) {
    if (value > 32767) {
        return 32767;
    }
    if (value < -32678) {
        return -32678;
    }
    return static_cast<short>(value);
}

static void DSPADPCMDecode(const uint8_t *src, short *dst, short &yn1, short &yn2, const int16_t coefs[8][2], const uint32_t sampleCount) {
    //Each DSP-ADPCM frame is 8 bytes long. It contains 1 header byte, and 7 sample bytes.

    //Set initial values.
    uint32_t dstIndex = 0;
    uint32_t srcIndex = 0;

    //Until all samples decoded.
    while (dstIndex < sampleCount) {

        //Get the header.
        const uint8_t header = src[srcIndex++];

        //Get scale and co-efficient index.
        const uint16_t scale = 1 << (header & 0xF);
        uint8_t coef_index = header >> 4;
        short coef1 = coefs[coef_index][0];
        short coef2 = coefs[coef_index][1];

        //7 sample bytes per frame.
        for (uint32_t b = 0; b < 7; b++) {

            //Get byte.
            uint8_t byt = src[srcIndex++];

            //2 samples per byte.
            for (uint32_t s = 0; s < 2; s++) {
                int8_t adpcm_nibble = s == 0 ? GetHighNibble(byt) : GetLowNibble(byt);
                short sample = Clamp16(
                        ((adpcm_nibble * scale << 11) + 1024 + (coef1 * yn1 + coef2 * yn2)) >> 11);

                yn2 = yn1;
                yn1 = sample;
                dst[dstIndex++] = sample;

                if (dstIndex >= sampleCount) break;
            }
            if (dstIndex >= sampleCount) break;
        }
    }
}