//
// Created by cookieso on 29.11.24.
//

#pragma once

#include <cstdint>

namespace dspadpcm {
    // From citric composer https://github.com/Gota7/Citric-Composer/blob/master/Citric%20Composer/Citric%20Composer/Low%20Level/Stream%20Audio/DspAdpcmDecode.cs
    void decode(const uint8_t *src, short *dst, short &yn1, short &yn2, const int16_t coefs[8][2],
                uint32_t sampleCount, uint32_t startSample);

    // From VG Audio https://github.com/Thealexbarney/VGAudio/blob/master/src/VGAudio/Codecs/GcAdpcm/GcAdpcmCoefficients.cs
    std::array<int16_t, 16> calculateCoefficients(const int16_t *pcm16samples, uint32_t sampleCount);

    // From VG Audio https://github.com/Thealexbarney/VGAudio/blob/master/src/VGAudio/Codecs/GcAdpcm/GcAdpcmEncoder.cs
    void encode();
};
