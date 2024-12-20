//
// Created by cookieso on 08.12.24.
//

#pragma once

#include "../bfstm/BfstmFile.h"

struct BfwavLoopInfo {
    uint32_t loopStartSample;
    uint32_t alignedLoopStartSample;
};

struct BfwavReadContext {
    SoundEncoding format;
    uint32_t sampleRate;
    uint32_t channelNum;
    std::optional<BfwavLoopInfo> loopInfo;
    uint32_t sampleCount;
    std::vector<uint32_t> channelDataOffsets;
    std::vector<BfstmDSPADPCMChannelInfo> dspAdpcmChannelInfo;
};