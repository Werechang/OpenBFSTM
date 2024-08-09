//
// Created by cookieso on 01.08.24.
//

#pragma once

#include <memory>
#include <functional>

class BfstmContext;

void initLoopDspYn(const BfstmContext &context, std::shared_ptr<int16_t[][2]> &dspYn);

void
initDspYn(const BfstmContext &context, const void *histPtr, uint32_t blockIndex, std::shared_ptr<int16_t[][2]> &dspYn);

void initRegionDspYn(const BfstmContext &context, uint32_t regionIndex, std::shared_ptr<int16_t[][2]> &dspYn);

void initDsp(const BfstmContext &context, std::shared_ptr<int16_t[][8][2]> &coefficients,
             std::shared_ptr<int16_t[][2]> &dspYn);

int decodeFrameBlockSimple(uint32_t channelNum, uint32_t startChannel, uint32_t frameCount, uint32_t startByte,
                           uint32_t thisBlockSizeRaw,
                           const void *offsetDataPtr,
                           const std::function<void(void **, uint32_t)> &writeFun);

int decodeFrameBlockDSP(uint32_t channelNum, uint32_t startChannel, uint32_t frameCount, uint32_t startSample,
                        uint32_t thisBlockSizeRaw,
                        const void *offsetDataPtr, const std::shared_ptr<int16_t[][8][2]> &coefficients,
                        std::shared_ptr<int16_t[][2]> &dspYn,
                        const std::function<void(void **, uint32_t)> &writeFun);
