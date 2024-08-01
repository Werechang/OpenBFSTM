#include <fstream>
#include <iostream>
#include <ostream>
#include <functional>
#include <filesystem>

#include "BfstmFile.h"
#include "MemoryResource.h"
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

enum class Options {
    INFILE,
    VOLUME,
    LOOP_NUM,
    DEVICE
};

void parseOption(char option, char *value) {
    switch (option) {

    }
}

void iterateAll() {
    const std::filesystem::path path{
            "/home/cookieso/switch/Games/RomFS/SUPER MARIO ODYSSEY v0 (0100000000010000)/SoundData/stream/"};
    std::unordered_map<uint32_t, int32_t> used{};
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
            MemoryResource resource{in};
            auto context = readBfstm(resource);
            if (!context) {
                std::cerr << "Context could not be loaded." << std::endl;
                continue;
            }
            if (context->streamInfo.soundEncoding != SoundEncoding::DSP_ADPCM) {
                std::cout << context->streamInfo.soundEncoding << " in: " << dir_entry.path() << std::endl;
            }
            ++used[context->streamInfo.loopStart % context->streamInfo.blockSizeSamples];
        }
    }
    for (const auto &i: used) {
        std::cout << std::hex << i.first << " : " << i.second << ' ';
    }
    std::cout << std::endl;
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
    MemoryResource resource{in};
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

    if (context->streamInfo.isLoop) {
        std::cout << "This audio file loops. Write 's' to stop." << std::endl;
    }

    auto *dataPtr = resource.getAsPtrUnsafe(
            context->header.dataSection->offset + 0x8 + context->streamInfo.sampleDataOffset);
    auto histPtr = resource.getAsPtrUnsafe(context->header.seekSection->offset + 0x8);

    ALSAPlayback audio{deviceName, getFormat(context->streamInfo.soundEncoding), context->streamInfo.sampleRate,
                       context->streamInfo.channelNum};
    audio.startDevice();
    std::thread audioThread([&](){
        audio.play(context.value(), dataPtr);
    });

    while (true) {
        char c;
        std::cin >> c;
        switch (c) {
            case 's': audio.stop();
                audioThread.join();
                return 0;
            case 'p': audio.pause(true);
                break;
            case 'c': audio.pause(false);
                break;
            case 'r':
                uint32_t block;
                std::cin >> block;
                if (block >= context->streamInfo.blockCountPerChannel) {
                    std::cout << "Block num too big!" << std::endl;
                    break;
                }
                audio.seek(context.value(), histPtr, block);
                break;
            default:
                continue;
        }
    }
}
