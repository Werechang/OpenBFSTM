//
// Created by cookieso on 01.08.24.
//

#include <memory>
#include <functional>
#include <iostream>
#include "../BfstmFile.h"
#include "../DSPADPCMCodec.h"

void initLoopDspYn(const BfstmContext &context, std::shared_ptr<int16_t[][2]> &dspYn) {
    for (int i = 0; i < context.streamInfo.channelNum; ++i) {
        auto dsp = get<BfstmDSPADPCMChannelInfo>(context.channelInfos[i]);
        dspYn[i][0] = dsp.loopYn1;
        dspYn[i][1] = dsp.loopYn2;
    }
}

void
initDspYn(const BfstmContext &context, const void *histPtr, uint32_t blockIndex, std::shared_ptr<int16_t[][2]> &dspYn) {
    auto histInfo = static_cast<const BfstmHistoryInfo *>(histPtr);
    for (int i = 0; i < context.streamInfo.channelNum; ++i) {
        auto yn = histInfo[blockIndex * context.streamInfo.channelNum + i];
        dspYn[i][0] = yn.histSample1;
        dspYn[i][1] = yn.histSample2;
    }
}

void initDsp(const BfstmContext &context, std::shared_ptr<int16_t[][8][2]> &coefficients,
             std::shared_ptr<int16_t[][2]> &dspYn) {
    for (int i = 0; i < context.streamInfo.channelNum; ++i) {
        auto dsp = get<BfstmDSPADPCMChannelInfo>(context.channelInfos[i]);
        dspYn[i][0] = dsp.yn1;
        dspYn[i][1] = dsp.yn2;
        for (int j = 0; j < 8; ++j) {
            coefficients[i][j][0] = dsp.coefficients[j][0];
            coefficients[i][j][1] = dsp.coefficients[j][1];
        }
    }
}

/**
 *
 * @param channelNum
 * @param frameCount Number of frames to be played
 * @param startByte The first byte where playback should start
 * @param sampleSize
 * @param thisBlockSizeRaw
 * @param encoding
 * @param offsetDataPtr
 * @param writeFun the function that is responsable to write the data. It takes a pointer to multiple buffers (amount is channel count) and the frame count
 */
int decodeFrameBlockSimple(uint32_t channelNum, uint32_t frameCount,
                           uint32_t thisBlockSizeRaw,
                           const void *offsetDataPtr,
                           const std::function<void(void **, uint32_t)> &writeFun) {
    auto blockData = std::make_unique_for_overwrite<void *[]>(channelNum);
    for (int j = 0; j < channelNum; ++j) {
        blockData[j] =
                reinterpret_cast<void *>(reinterpret_cast<size_t>(offsetDataPtr) + j * thisBlockSizeRaw);
    }
    writeFun(blockData.get(), frameCount);
    return 0;
}

/**
 *
 * @param channelNum
 * @param frameCount Number of frames to be played
 * @param startByte The first byte where playback should start
 * @param thisBlockSizeRaw
 * @param offsetDataPtr
 * @param coefficients The coefficients for dsp-adpcm playback
 * @param dspYn The start yn values for dsp-adpcm playback (must be initialized with the right values corresponding to startFrame)
 * @param writeFun the function that is responsable to write the data. It takes a pointer to multiple buffers (amount is channel count) and the frame count
 * @return
 */
int decodeFrameBlockDSP(uint32_t channelNum, uint32_t frameCount,
                        uint32_t thisBlockSizeRaw,
                        const void *offsetDataPtr, const std::shared_ptr<int16_t[][8][2]> &coefficients,
                        std::shared_ptr<int16_t[][2]> &dspYn,
                        const std::function<void(void **, uint32_t)> &writeFun) {
    auto blockData = std::make_unique_for_overwrite<void *[]>(channelNum);
    std::unique_ptr<int16_t[]> decoded = std::make_unique_for_overwrite<int16_t[]>(frameCount);
    for (int j = 0; j < channelNum; ++j) {
        auto *start = reinterpret_cast<uint8_t *>(reinterpret_cast<size_t>(offsetDataPtr) + j * thisBlockSizeRaw);
        DSPADPCMDecode(start, decoded.get(), dspYn[j][0], dspYn[j][1],
                       coefficients[j], frameCount);
        blockData[j] = decoded.get();
    }
    writeFun(blockData.get(), frameCount);
    return 0;
}
