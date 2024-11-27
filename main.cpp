#include <fstream>
#include <iostream>
#include <ostream>
#include <functional>
#include <filesystem>

#include "bfstm/BfstmFile.h"
#include "MemoryResource.h"
#include "playback/ALSAPlayback.h"
#include "DSPADPCMCodec.h"
#include "Window.h"
#include "bfstm/BfstmReader.h"

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
            auto context = BfstmReader(resource).m_Context;
            ++used[context.streamInfo.regionNum];
        }
    }
    for (const auto &i: used) {
        std::cout << std::hex << i.first << " amount: " << std::dec << i.second << '\n';
    }
    std::cout << std::endl;
    exit(0);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cout << "Please specify a file to play." << std::endl;
        return 0;
    }
    MemoryResource outMem(0x400);
    OutMemoryStream outBfstm{outMem};
    BfstmWriteInfo outInfo{SoundEncoding::DSP_ADPCM, 2, true, 48000, 0, 1000};
    writeBfstm(outBfstm, outInfo);
    outMem.writeToFile("Exported.bfstm");
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
    BfstmReader bfstmReader(resource);
    if (!bfstmReader.success) {
        std::cerr << "Context could not be loaded." << std::endl;
        return -1;
    }
    auto context = bfstmReader.m_Context;

    if (!context.regionInfos.empty()) {
        std::cout << "This audio file contains regions (" << context.regionInfos.size()
                  << " in total). Enter 'r' to increment the region counter." << std::endl;
    }

    std::cout << "Length: " << (context.streamInfo.blockSizeSamples * (context.streamInfo.blockCountPerChannel - 1) +
                                context.streamInfo.lastBlockSizeSamples) / context.streamInfo.sampleRate << "s"
              << std::endl;
    std::cout << "Channel count: " << static_cast<uint32_t>(context.streamInfo.channelNum) << std::endl;

    if (context.streamInfo.isLoop) {
        std::cout << "Loop Start: " << context.streamInfo.loopStart << std::endl;
        std::cout << "Loop End: " << context.streamInfo.loopEnd << std::endl;
        std::cout << "This audio file loops. Write 's' to stop." << std::endl;
    }

    auto dataPtr = resource.getAsPtrUnsafe(
            context.header.dataSection->offset + 0x8 + context.streamInfo.sampleDataOffset);
    auto histPtr = resource.getAsPtrUnsafe(context.header.seekSection->offset + 0x8);

    ALSAPlayback audio{argc > 2 ? argv[2] : std::string("pipewire"), getFormat(context.streamInfo.soundEncoding),
                       context.streamInfo.sampleRate,
                       context.streamInfo.channelNum < 2 ? 1u : 2u};
    audio.startDevice();
    std::thread audioThread([&]() {
        audio.play(context, dataPtr);
    });

    Window window{};

    while (!window.shouldClose()) {
        window.preDraw();
        window.draw(audio, context);
        window.afterDraw();
        /*
        char c;
        std::cin >> c;
        switch (c) {
            case 's':
                audio.stop();
                audioThread.join();
                return 0;
            case 'p':
                isPaused = !isPaused;
                audio.pause(isPaused);
                break;
            case 'g':
                uint32_t block;
                std::cin >> block;
                if (block >= context->streamInfo.blockCountPerChannel) {
                    std::cout << "Block num too big!" << std::endl;
                    break;
                }
                audio.seek(context.value(), histPtr, block);
                break;
            case 'r':
                if (context->regionInfos.empty()) {
                    std::cout << "This track does not have regions!" << std::endl;
                    break;
                }
                audio.incRegion();
                break;
            case 'c':
                uint32_t channel;
                std::cin >> channel;
                if (context->streamInfo.channelNum <= 2) {
                    std::cout << "Channel index can't be specified since this track doesn't have enough channels."
                              << std::endl;
                } else if (channel * 2 > context->streamInfo.channelNum - 2) {
                    std::cout << "Channel index too big! Max index is: " << (context->streamInfo.channelNum - 2) / 2
                              << std::endl;
                } else {
                    audio.setChannel(channel * 2);
                }
                break;
            default:
                continue;
        }*/
    }
    audio.stop();
    audioThread.join();
}
