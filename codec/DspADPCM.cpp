//
// Created by cookieso on 29.11.24.
//

#include <algorithm>
#include <limits>
#include <array>
#include <cmath>
#include <vector>
#include <cstring>
#include <memory>
#include "DspADPCM.h"

static int8_t nibbleToSHalfbyte[] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};

inline int8_t getHighNibble(const uint8_t value) {
    return nibbleToSHalfbyte[value >> 4 & 0xF];
}

inline int8_t getLowNibble(const uint8_t value) {
    return nibbleToSHalfbyte[value & 0xF];
}

inline short clamp16(const int value) {
    if (value > 32767) {
        return 32767;
    }
    if (value < -32678) {
        return -32678;
    }
    return static_cast<short>(value);
}

void InnerProductMerge(std::array<double, 3> &vecOut, const std::array<int16_t, 28>& pcmBuf) {
    for (int i = 0; i <= 2; i++) {
        vecOut[i] = 0.0;
        for (int x = 0; x < 14; x++) {
            vecOut[i] -= pcmBuf[14 + x - i] * pcmBuf[14 + x];
        }
    }
}

void OuterProductMerge(std::array<std::array<double, 3>, 3> &mtxOut, const std::array<int16_t, 28>& pcmBuf) {
    for (int x = 1; x <= 2; x++) {
        for (int y = 1; y <= 2; y++) {
            mtxOut[x][y] = 0.0;
            for (int z = 0; z < 14; z++) {
                mtxOut[x][y] += pcmBuf[14 + z - x] * pcmBuf[14 + z - y];
            }
        }
    }
}

bool AnalyzeRanges(std::array<std::array<double, 3>, 3> &mtx, std::array<int, 3> &vecIdxsOut, std::array<double, 3>& recips) {
    double val, tmp, min, max;

    /* Get greatest distance from zero */
    for (int x = 1; x <= 2; x++) {
        val = std::max(std::abs(mtx[x][1]), std::abs(mtx[x][2]));
        if (val < std::numeric_limits<double>::epsilon()) return true;

        recips[x] = 1.0 / val;
    }

    int maxIndex = 0;
    for (int i = 1; i <= 2; i++) {
        for (int x = 1; x < i; x++) {
            tmp = mtx[x][i];
            for (int y = 1; y < x; y++) {
                tmp -= mtx[x][y] * mtx[y][i];
            }
            mtx[x][i] = tmp;
        }

        val = 0.0;
        for (int x = i; x <= 2; x++) {
            tmp = mtx[x][i];
            for (int y = 1; y < i; y++) {
                tmp -= mtx[x][y] * mtx[y][i];
            }

            mtx[x][i] = tmp;
            tmp = std::abs(tmp) * recips[x];
            if (tmp >= val) {
                val = tmp;
                maxIndex = x;
            }
        }

        if (maxIndex != i) {
            for (int y = 1; y <= 2; y++) {
                tmp = mtx[maxIndex][y];
                mtx[maxIndex][y] = mtx[i][y];
                mtx[i][y] = tmp;
            }
            recips[maxIndex] = recips[i];
        }

        vecIdxsOut[i] = maxIndex;

        if (i != 2) {
            tmp = 1.0 / mtx[i][i];
            for (int x = i + 1; x <= 2; x++)
                mtx[x][i] *= tmp;
        }
    }

    /* Get range */
    min = 1.0e10;
    max = 0.0;
    for (int i = 1; i <= 2; i++) {
        tmp = std::abs(mtx[i][i]);
        if (tmp < min)
            min = tmp;
        if (tmp > max)
            max = tmp;
    }

    return min / max < 1.0e-10;
}

void BidirectionalFilter(std::array<std::array<double, 3>, 3> &mtx, const std::array<int, 3> &vecIdxs, std::array<double, 3> &vecOut) {
    double tmp;

    for (int i = 1, x = 0; i <= 2; i++) {
        int index = vecIdxs[i];
        tmp = vecOut[index];
        vecOut[index] = vecOut[i];
        if (x != 0)
            for (int y = x; y <= i - 1; y++)
                tmp -= vecOut[y] * mtx[i][y];
            // ReSharper disable once CompareOfFloatsByEqualityOperator
        else if (tmp != 0.0)
            x = i;
        vecOut[i] = tmp;
    }

    for (int i = 2; i > 0; i--) {
        tmp = vecOut[i];
        for (int y = i + 1; y <= 2; y++)
            tmp -= vecOut[y] * mtx[i][y];
        vecOut[i] = tmp / mtx[i][i];
    }

    vecOut[0] = 1.0;
}

bool QuadraticMerge(std::array<double, 3> &inOutVec) {
    double v2 = inOutVec[2];
    double tmp = 1.0 - (v2 * v2);

    // ReSharper disable once CompareOfFloatsByEqualityOperator
    if (tmp == 0.0)
        return true;

    double v0 = (inOutVec[0] - (v2 * v2)) / tmp;
    double v1 = (inOutVec[1] - (inOutVec[1] * v2)) / tmp;

    inOutVec[0] = v0;
    inOutVec[1] = v1;

    return std::abs(v1) > 1.0;
}

void FinishRecord(std::array<double, 3> &inR, std::vector<std::array<double, 3>> &outR, int row) {
    for (int z = 1; z <= 2; z++) {
        if (inR[z] >= 1.0)
            inR[z] = 0.9999999999;
        else if (inR[z] <= -1.0)
            inR[z] = -0.9999999999;
    }
    outR[row][0] = 1.0;
    outR[row][1] = (inR[2] * inR[1]) + inR[1];
    outR[row][2] = inR[2];
}

void FinishRecord(std::array<double, 3> &inR, std::array<double, 3> &outR) {
    for (int z = 1; z <= 2; z++) {
        if (inR[z] >= 1.0)
            inR[z] = 0.9999999999;
        else if (inR[z] <= -1.0)
            inR[z] = -0.9999999999;
    }
    outR[0] = 1.0;
    outR[1] = (inR[2] * inR[1]) + inR[1];
    outR[2] = inR[2];
}

void MatrixFilter(std::vector<std::array<double, 3>> &src, int row, std::array<double, 3> &dst, std::array<std::array<double, 3>, 3> &mtx) {
    mtx[2][0] = 1.0;
    for (int i = 1; i <= 2; i++)
        mtx[2][i] = -src[row][i];

    for (int i = 2; i > 0; i--) {
        double val = 1.0 - (mtx[i][i] * mtx[i][i]);
        for (int y = 1; y <= i; y++)
            mtx[i - 1][y] = ((mtx[i][i] * mtx[i][y]) + mtx[i][y]) / val;
    }

    dst[0] = 1.0;
    for (int i = 1; i <= 2; i++) {
        dst[i] = 0.0;
        for (int y = 1; y <= i; y++)
            dst[i] += mtx[i][y] * dst[i - y];
    }
}

void MergeFinishRecord(std::array<double, 3> &src, std::array<double, 3> &dst) {
    std::array<double, 3> tmp{};
    double val = src[0];

    dst[0] = 1.0;
    for (int i = 1; i <= 2; i++) {
        double v2 = 0.0;
        for (int y = 1; y < i; y++)
            v2 += dst[y] * src[i - y];

        if (val > 0.0)
            dst[i] = -(v2 + src[i]) / val;
        else
            dst[i] = 0.0;

        tmp[i] = dst[i];

        for (int y = 1; y < i; y++)
            dst[y] += dst[i] * dst[i - y];

        val *= 1.0 - (dst[i] * dst[i]);
    }

    FinishRecord(tmp, dst);
}

double ContrastVectors(std::array<double, 3> &source1, std::vector<std::array<double, 3>> &source2, int row) {
    double val = (source2[row][2] * source2[row][1] + -source2[row][1]) / (1.0 - source2[row][2] * source2[row][2]);
    double val1 = (source1[0] * source1[0]) + (source1[1] * source1[1]) + (source1[2] * source1[2]);
    double val2 = (source1[0] * source1[1]) + (source1[1] * source1[2]);
    double val3 = source1[0] * source1[2];
    return val1 + (2.0 * val * val2) + (2.0 * (-source2[row][1] * val + -source2[row][2]) * val3);
}

void FilterRecords(std::array<std::array<double, 3>, 8> &vecBest, int exp, std::vector<std::array<double, 3>> &records, int recordCount) {
    std::array<std::array<double, 3>, 8> bufferList{};

    std::array<std::array<double, 3>, 3> mtx{};

    std::array<int, 8> buffer1{};
    std::array<double, 3> buffer2{};

    for (int x = 0; x < 2; x++) {
        for (int y = 0; y < exp; y++) {
            buffer1[y] = 0;
            for (int i = 0; i <= 2; i++)
                bufferList[y][i] = 0.0;
        }
        for (int z = 0; z < recordCount; z++) {
            int index = 0;
            double value = 1.0e30;
            for (int i = 0; i < exp; i++) {
                double tempVal = ContrastVectors(vecBest[i], records, z);
                if (tempVal < value) {
                    value = tempVal;
                    index = i;
                }
            }
            buffer1[index]++;
            MatrixFilter(records, z, buffer2, mtx);
            for (int i = 0; i <= 2; i++)
                bufferList[index][i] += buffer2[i];
        }

        for (int i = 0; i < exp; i++)
            if (buffer1[i] > 0)
                for (int y = 0; y <= 2; y++)
                    bufferList[i][y] /= buffer1[i];

        for (int i = 0; i < exp; i++)
            MergeFinishRecord(bufferList[i], vecBest[i]);
    }
}

namespace dspadpcm {
    void decode(const uint8_t *src, short *dst, short &yn1, short &yn2, const int16_t (*coefs)[2], uint32_t sampleCount,
                uint32_t startSample) {
        //Each DSP-ADPCM group is 8 bytes long. It contains 1 header byte, and 7 sample bytes. so 8 bytes are 7 samples

        uint32_t startHeaderIndex = startSample / 7 * 8;

        //Set initial values.
        uint32_t dstIndex = 0;
        uint32_t srcIndex = startHeaderIndex;
        uint32_t remainingNotPlayed = startSample % 7;

        //Until all samples decoded.
        while (dstIndex < sampleCount) {

            //Get the header.
            const uint8_t header = src[srcIndex++];

            //Get scale and co-efficient index.
            const uint16_t scale = 1 << (header & 0xF);
            uint8_t coef_index = header >> 4;
            short coef1 = coefs[coef_index][0];
            short coef2 = coefs[coef_index][1];

            //7 sample bytes per header.
            for (uint32_t b = 0; b < 7; b++) {

                uint8_t byt = src[srcIndex++];

                // 1 byte to 1 sample
                for (uint32_t s = 0; s < 2; s++) {
                    int8_t adpcm_nibble = s == 0 ? getHighNibble(byt) : getLowNibble(byt);
                    short sample = clamp16(
                            ((adpcm_nibble * scale << 11) + 1024 + (coef1 * yn1 + coef2 * yn2)) >> 11);

                    yn2 = yn1;
                    yn1 = sample;
                    if (remainingNotPlayed > 0) --remainingNotPlayed;
                    else
                        dst[dstIndex++] = sample;

                    if (dstIndex >= sampleCount) break;
                }
                if (dstIndex >= sampleCount) break;
            }
        }
    }

    std::array<int16_t, 16> calculateCoefficients(const int16_t *pcm16samples, uint32_t sampleCount) {
        uint32_t frameCount = (sampleCount + 13) / 14;

        std::array<int16_t, 28> pcmHistBuffer{};

        std::array<int16_t, 16> coefs{};

        std::array<double, 3> vec1{};
        std::array<double, 3> vec2{};
        std::array<double, 3> buffer{};

        std::array<std::array<double, 3>, 3> mtx{};

        std::array<int, 3> vecIdxs{};

        std::vector<std::array<double, 3>> records;
        records.reserve(frameCount * 2);

        int recordCount = 0;

        std::array<std::array<double, 3>, 8> vecBest{};

        /* Iterate though one frame at a time */
        for (int sample = 0, remaining = sampleCount; sample < sampleCount; sample += 14, remaining -= 14) {
            std::fill_n(&pcmHistBuffer[14], 14, 0);
            memcpy(&pcmHistBuffer[14], &pcm16samples[sample], std::min(14, remaining));

            InnerProductMerge(vec1, pcmHistBuffer);
            if (std::abs(vec1[0]) > 10.0) {
                OuterProductMerge(mtx, pcmHistBuffer);
                if (!AnalyzeRanges(mtx, vecIdxs, buffer)) {
                    BidirectionalFilter(mtx, vecIdxs, vec1);
                    if (!QuadraticMerge(vec1)) {
                        FinishRecord(vec1, records, recordCount);
                        recordCount++;
                    }
                }
            }

            memcpy(&pcmHistBuffer[0], &pcmHistBuffer[14], 14);
        }

        vec1[0] = 1.0;
        vec1[1] = 0.0;
        vec1[2] = 0.0;

        for (int z = 0; z < recordCount; z++) {
            MatrixFilter(records, z, vecBest[0], mtx);
            for (int y = 1; y <= 2; y++)
                vec1[y] += vecBest[0][y];
        }
        for (int y = 1; y <= 2; y++)
            vec1[y] /= recordCount;

        MergeFinishRecord(vec1, vecBest[0]);


        int exp = 1;
        for (int w = 0; w < 3;) {
            vec2[0] = 0.0;
            vec2[1] = -1.0;
            vec2[2] = 0.0;
            for (int i = 0; i < exp; i++)
                for (int y = 0; y <= 2; y++)
                    vecBest[exp + i][y] = (0.01 * vec2[y]) + vecBest[i][y];
            ++w;
            exp = 1 << w;
            FilterRecords(vecBest, exp, records, recordCount);
        }

        /* Write output */
        for (int z = 0; z < 8; z++) {
            double d;
            d = -vecBest[z][1] * 2048.0;
            coefs[z * 2] = clamp16(std::round(d));

            d = -vecBest[z][2] * 2048.0;
            coefs[z * 2 + 1] = clamp16(std::round(d));
        }
        return coefs;
    }

    /*std::unique_ptr<uint8_t> Encode(const int16_t *pcm, uint32_t sampleCount, const std::array<int16_t, 16> &coefficients) {
        var adpcm = new byte[SampleCountToByteCount(sampleCount)];

        var pcmBuffer = new short[2 + SamplesPerFrame];
        var adpcmBuffer = new byte[BytesPerFrame];

        pcmBuffer[0] = config.History2;
        pcmBuffer[1] = config.History1;

        int frameCount = sampleCount.DivideByRoundUp(SamplesPerFrame);
        var buffers = new AdpcmEncodeBuffers();

        for (int frame = 0; frame < frameCount; frame++)
        {
            int samplesToCopy = Math.Min(sampleCount - frame * SamplesPerFrame, SamplesPerFrame);
            Array.Copy(pcm, frame * SamplesPerFrame, pcmBuffer, 2, samplesToCopy);
            Array.Clear(pcmBuffer, 2 + samplesToCopy, SamplesPerFrame - samplesToCopy);

            DspEncodeFrame(pcmBuffer, SamplesPerFrame, adpcmBuffer, coefs, buffers);

            Array.Copy(adpcmBuffer, 0, adpcm, frame * BytesPerFrame, SampleCountToByteCount(samplesToCopy));

            pcmBuffer[0] = pcmBuffer[14];
            pcmBuffer[1] = pcmBuffer[15];
            config.Progress?.ReportAdd(1);
        }

        return adpcm;
    }*/
}
