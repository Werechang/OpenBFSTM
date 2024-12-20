#include <fstream>
#include <iostream>
#include <ostream>
#include <functional>
#include <filesystem>
#include <utility>
#include <stack>

#include "MemoryResource.h"
#include "playback/ALSAPlayback.h"
#include "Window.h"
#include "format/bfsar/BfsarReader.h"
#include "format/bfstm/BfstmReader.h"
#include "format/bfgrp/BfgrpReader.h"
#include "format/bfgrp/BfgrpWriter.h"
#include "format/bfwar/BfwarReader.h"
#include "format/bfwar/BfwarWriter.h"
#include "playback/DummyPlayback.h"
#include "format/bfwav/BfwavReader.h"

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
            "/home/cookieso/OdysseyModding/bfsar/"};
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
            std::cout << "Reading " << dir_entry.path() << std::endl;
            MemoryResource resource{in};
            BfsarReader reader(resource);
        }
    }
    for (const auto &i: used) {
        std::cout << std::hex << i.first << " amount: " << std::dec << i.second << '\n';
    }
    std::cout << std::endl;
    exit(0);
}

void testOne() {
    std::ifstream in{
            "/home/cookieso/OdysseyModding/bfsar/AtmosBirdsInsects.bfsar",
            std::ios::binary
    };
    if (!in) {
        std::cout << "File is invalid." << std::endl;
    } else {
        std::cout << "Reading... " << std::endl;
        MemoryResource resource{in};
        BfsarReader reader(resource);

    }
    exit(0);
}

int main(int argc, char **argv) {
    iterateAll();
    testOne();

    std::ifstream in{
            argv[1],
            std::ios::binary
    };
    if (!in) {
        std::cout << "File is invalid." << std::endl;
        return -1;
    }
    MemoryResource resource{in};


    DummyPlayback audio{};
    audio.startDevice();
    std::thread audioThread([&]() {
        //audio.play(context, dataPtr);
    });

    Window window{};

    while (!window.shouldClose()) {
        window.preDraw();
        window.draw(audio);
        window.afterDraw();
    }
    audio.stop();
    audioThread.join();
}
