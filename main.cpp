#include <fstream>
#include <iostream>
#include <vector>
#include <ostream>
#include <functional>

#include "BfstmFile.h"
#include "InMemoryResource.h"
#include "AudioPlaybackManager.h"
#include "DSPADPCMCodec.h"

/*        D | E
 * BFSAR  - | -
 * BFGRP  - | -
 * BFSTM  X | -
 */

snd_pcm_format_t getFormat(const SoundEncoding encoding) {
    switch (encoding) {
        case SoundEncoding::PCM8:
            return SND_PCM_FORMAT_S8;
        case SoundEncoding::PCM16:
        case SoundEncoding::DSP_ADPCM:
            return SND_PCM_FORMAT_S16_LE;
        case SoundEncoding::IMA_ADPCM:
            return SND_PCM_FORMAT_IMA_ADPCM;
        default:
            return SND_PCM_FORMAT_UNKNOWN;
    }
}

void decodeBlocks(const BfstmContext &context, const InMemoryResource &resource, const std::function<void(void**, uint32_t)>& writeFun) {
    uint32_t channelNum = context.streamInfo.channelNum;
    auto *dataPtr = resource.getAsPtrUnsafe(
            context.header.dataSection->offset + 0x8 + context.streamInfo.sampleDataOffset);
    auto blockData = std::make_unique_for_overwrite<void *[]>(channelNum);
    auto dspYn = std::vector<std::array<int16_t, 2>>(channelNum);
    if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
        for (int i = 0; i < channelNum; ++i) {
            auto dsp = get<BfstmDSPADPCMChannelInfo>(context.channelInfos[i]);
            dspYn[i][0] = dsp.yn1;
            dspYn[i][1] = dsp.yn2;
        }
    }
    std::unique_ptr<int16_t[]> decoded = std::make_unique_for_overwrite<int16_t[]>(
            context.streamInfo.blockSizeSamples);
    for (int i = 0; i < context.streamInfo.blockCountPerChannel; ++i) {
        bool isLast = i + 1 == context.streamInfo.blockCountPerChannel;
        uint32_t blockSampleCount = isLast
                                    ? context.streamInfo.lastBlockSizeSamples
                                    : context.streamInfo.blockSizeSamples;
        uint32_t off = i * channelNum * context.streamInfo.blockSizeBytes;
        for (int j = 0; j < channelNum; ++j) {
            if (context.streamInfo.soundEncoding != SoundEncoding::DSP_ADPCM) {
                blockData[j] =
                        reinterpret_cast<void *>(reinterpret_cast<size_t>(dataPtr) + off + j * (isLast
                                                                                                ? context.streamInfo.lastBlockSizeBytesRaw
                                                                                                : context.streamInfo.blockSizeBytes));
            } else {
                auto *start = reinterpret_cast<uint8_t *>(reinterpret_cast<size_t>(dataPtr) + off + j * (isLast
                                                                                                         ? context.streamInfo.lastBlockSizeBytesRaw
                                                                                                         : context.streamInfo.blockSizeBytes));
                DSPADPCMDecode(start, decoded.get(), dspYn[j][0], dspYn[j][1],
                               get<BfstmDSPADPCMChannelInfo>(context.channelInfos[j]).coefficients, blockSampleCount);
                blockData[j] = decoded.get();
            }
        }
        writeFun(blockData.get(), blockSampleCount);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Please specify a file to play." << std::endl;
        return 0;
    }
    std::ifstream in{
            argv[1],
            std::ios::binary
    };
    if (!in) {
        std::cout << "File is invalid." << std::endl;
        return -1;
    }
    InMemoryResource resource{in};
    auto context = readBfstm(resource);
    if (!context) {
        std::cerr << "Context could not be loaded." << std::endl;
        return -1;
    }

    if (context->streamInfo.isLoop) {
        std::cout << "Loop Start: " << context->streamInfo.loopStart << std::endl;
        std::cout << "Loop End: " << context->streamInfo.loopEnd << std::endl;
    }
    std::cout << "Length: " << (context->streamInfo.blockSizeSamples * (context->streamInfo.blockCountPerChannel - 1) +
                                context->streamInfo.lastBlockSizeSamples) / context->streamInfo.sampleRate << "s" << std::endl;

    uint32_t channelNum = context->streamInfo.channelNum;
    auto deviceName = "pipewire";
    if (argc > 2) {
        deviceName = argv[2];
    }
    auto devices = AudioPlaybackManager::getDevices();
    bool hasDevice = false;
    for (const auto &device: devices) {
        if (device == deviceName) hasDevice = true;
    }
    if (!hasDevice) {
        std::cerr << "Playback Device " << deviceName <<
                  " not found! Specify a device with the third argument.\nAvailable devices:\n";
        for (const auto &device: devices) {
            std::cerr << device << '\n';
        }
        return 0;
    }
    AudioPlaybackManager audio{deviceName, getFormat(context->streamInfo.soundEncoding), context->streamInfo.sampleRate,
                               channelNum};
    audio.startDevice();
    decodeBlocks(context.value(), resource, [&](void **data, uint32_t frames){
        audio.writeData(data, frames);
    });
    return 0;
}
