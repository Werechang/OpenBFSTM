#include <fstream>
#include <iostream>
#include <vector>
#include <ostream>
#include <functional>
#include <filesystem>

#include "BfstmFile.h"
#include "InMemoryResource.h"
#include "playback/ALSAPlayback.h"
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

/**
 *
 * @param channelNum
 * @param frameCount Number of frames to be played
 * @param startFrame The first frame to be played
 * @param sampleSize
 * @param blockSizeRaw
 * @param encoding
 * @param offsetDataPtr
 * @param coefficients The coefficients for dsp-adpcm playback
 * @param dspYn The start yn values for dsp-adpcm playback
 * @param writeFun
 */
void decodeFrameBlock(uint32_t channelNum, uint32_t frameCount, uint32_t startFrame, uint32_t sampleSize,
                      uint32_t blockSizeRaw, SoundEncoding encoding,
                      const void *offsetDataPtr, std::optional<std::vector<int16_t[8][2]>> &coefficients,
                      std::optional<std::vector<std::array<int16_t, 2>>> &dspYn,
                      const std::function<void(void **, uint32_t)> &writeFun) {
    auto blockData = std::make_unique_for_overwrite<void *[]>(channelNum);
    std::unique_ptr<int16_t[]> decoded = std::make_unique_for_overwrite<int16_t[]>(frameCount);
    for (int j = 0; j < channelNum; ++j) {
        if (encoding != SoundEncoding::DSP_ADPCM) {
            blockData[j] =
                    reinterpret_cast<void *>(reinterpret_cast<size_t>(offsetDataPtr) + j * blockSizeRaw +
                                             sampleSize * startFrame);
        } else {
            auto *start = reinterpret_cast<uint8_t *>(reinterpret_cast<size_t>(offsetDataPtr) + j * blockSizeRaw +
                                                      sampleSize * startFrame);
            DSPADPCMDecode(start, decoded.get(), dspYn.value()[j][0], dspYn.value()[j][1],
                           coefficients.value()[j], frameCount);
            blockData[j] = decoded.get();
        }
    }
    writeFun(blockData.get(), frameCount);
}

void decodeBlocks(const BfstmContext &context, const InMemoryResource &resource,
                  const std::function<void(void **, uint32_t)> &writeFun) {
    uint32_t channelNum = context.streamInfo.channelNum;
    uint32_t blockSizeBytes = context.streamInfo.blockSizeBytes;
    uint32_t lastBlockSizeBytesRaw = context.streamInfo.lastBlockSizeBytesRaw;
    uint32_t sampleSize = context.streamInfo.blockSizeSamples / context.streamInfo.blockSizeBytes;

    auto *dataPtr = resource.getAsPtrUnsafe(
            context.header.dataSection->offset + 0x8 + context.streamInfo.sampleDataOffset);
    auto blockData = std::make_unique_for_overwrite<void *[]>(channelNum);

    std::optional<std::vector<int16_t[8][2]>> coefficients;
    std::optional<std::vector<std::array<int16_t, 2>>> dspYn;
    if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
        dspYn = std::vector<std::array<int16_t, 2>>(channelNum);
        coefficients = std::vector<int16_t[8][2]>(channelNum);
        for (int i = 0; i < channelNum; ++i) {
            auto dsp = get<BfstmDSPADPCMChannelInfo>(context.channelInfos[i]);
            dspYn.value()[i][0] = dsp.yn1;
            dspYn.value()[i][1] = dsp.yn2;
            for (int j = 0; j < 8; ++j) {
                coefficients.value()[i][j][0] = dsp.coefficients[j][0];
                coefficients.value()[i][j][1] = dsp.coefficients[j][1];
            }
        }
    }
    std::unique_ptr<int16_t[]> decoded = std::make_unique_for_overwrite<int16_t[]>(
            context.streamInfo.blockSizeSamples);
    for (int i = 0; i < context.streamInfo.blockCountPerChannel; ++i) {
        bool isLast = i + 1 == context.streamInfo.blockCountPerChannel;
        uint32_t blockSampleCount = isLast
                                    ? context.streamInfo.lastBlockSizeSamples
                                    : context.streamInfo.blockSizeSamples;
        uint32_t off = i * channelNum * blockSizeBytes;
        uint32_t blockSize = isLast
                             ? lastBlockSizeBytesRaw
                             : blockSizeBytes;
        decodeFrameBlock(channelNum, blockSampleCount, 0, sampleSize, blockSize,
                         context.streamInfo.soundEncoding,
                         reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off), coefficients, dspYn,
                         writeFun);
    }
    // Loop Logic
    if (!context.streamInfo.isLoop) return;
    auto frameCount = static_cast<double>(
            (context.streamInfo.blockCountPerChannel - 1) * context.streamInfo.blockSizeSamples +
            context.streamInfo.lastBlockSizeSamples);
    auto loopStartBlock =
            static_cast<uint32_t>(context.streamInfo.loopStart / frameCount * context.streamInfo.blockCountPerChannel);
    uint32_t startSampleInBlock = context.streamInfo.loopStart % context.streamInfo.blockSizeSamples;
    auto loopEndBlock =
            static_cast<uint32_t>(context.streamInfo.loopEnd / frameCount * context.streamInfo.blockCountPerChannel);
    if (loopEndBlock == context.streamInfo.blockCountPerChannel)
        loopEndBlock = context.streamInfo.blockCountPerChannel - 1;
    uint32_t endSampleInBlock = context.streamInfo.loopEnd % context.streamInfo.blockSizeSamples;
    if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
        for (int i = 0; i < channelNum; ++i) {
            auto dsp = get<BfstmDSPADPCMChannelInfo>(context.channelInfos[i]);
            dspYn.value()[i][0] = dsp.loopYn1;
            dspYn.value()[i][1] = dsp.loopYn2;
        }
    }
    for (int n = 0; n < 8; ++n) {
        for (uint32_t i = loopStartBlock; i <= loopEndBlock; ++i) {
            bool isLast = i + 1 == context.streamInfo.blockCountPerChannel;
            bool isLoopStart = i == loopStartBlock;
            bool isLoopEnd = i == loopEndBlock;
            uint32_t off = i * channelNum * blockSizeBytes + (isLoopStart ? startSampleInBlock * sampleSize : 0);
            decodeFrameBlock(channelNum, (isLoopEnd ? endSampleInBlock : context.streamInfo.blockSizeSamples) -
                                         (isLoopStart ? startSampleInBlock : 0), isLoopStart ? startSampleInBlock : 0,
                             sampleSize, isLast ? lastBlockSizeBytesRaw : blockSizeBytes,
                             context.streamInfo.soundEncoding,
                             reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off), coefficients,
                             dspYn,
                             writeFun);
        }
    }
}

enum class Options {
    OUTFILE,
    VOLUME,
};

void parseOption(char option, char *value) {
    switch (option) {

    }
}

void iterateAll() {
    const std::filesystem::path path{"/home/cookieso/switch/Games/RomFS/SUPER MARIO ODYSSEY v0 (0100000000010000)/SoundData/stream/"};
    uint32_t minLen = 999999;
    std::filesystem::path minFile;
    for (auto const &dir_entry: std::filesystem::directory_iterator{path}) {
        if (dir_entry.is_regular_file()) {
            std::ifstream in{
                    dir_entry.path(),
                    std::ios::binary
            };
            if (!in) {
                std::cout << "File is invalid." << std::endl;
                continue;
            }
            InMemoryResource resource{in};
            auto context = readBfstm(resource);
            if (!context) {
                std::cerr << "Context could not be loaded." << std::endl;
                continue;
            }
            if (context->streamInfo.soundEncoding != SoundEncoding::DSP_ADPCM) {
                std::cout << context->streamInfo.soundEncoding << " in: " << dir_entry.path() << std::endl;
            }
            if (!context->streamInfo.isLoop) continue;
            uint32_t len = (context->streamInfo.blockSizeSamples * (context->streamInfo.blockCountPerChannel - 1) +
                            context->streamInfo.lastBlockSizeSamples) / context->streamInfo.sampleRate;
            if (len < minLen) {
                minLen = len;
                minFile = dir_entry.path();
            }
        }
    }
    std::cout << "Min len: " << minLen << "s in: " << minFile << std::endl;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Please specify a file to play." << std::endl;
        return 0;
    }
    iterateAll();

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
                                context->streamInfo.lastBlockSizeSamples) / context->streamInfo.sampleRate << "s"
              << std::endl;

    auto deviceName = "pipewire";
    if (argc > 2) {
        deviceName = argv[2];
    }
    auto devices = ALSAPlayback::getDevices();
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

    ALSAPlayback audio{deviceName, getFormat(context->streamInfo.soundEncoding), context->streamInfo.sampleRate,
                       context->streamInfo.channelNum};
    audio.startDevice();
    decodeBlocks(context.value(), resource, [&](void **data, uint32_t frames) {
        audio.writeData(data, frames);
    });
    return 0;
}
