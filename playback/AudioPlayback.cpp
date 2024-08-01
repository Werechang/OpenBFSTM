//
// Created by cookieso on 01.08.24.
//
#include "AudioPlayback.h"
#include "../MemoryResource.h"
#include "PlaybackFunctions.h"

void AudioPlayback::play(const BfstmContext &context, MemoryResource &resource) {
    auto *dataPtr = resource.getAsPtrUnsafe(
            context.header.dataSection->offset + 0x8 + context.streamInfo.sampleDataOffset);

    if (context.streamInfo.isLoop) {
        if (context.streamInfo.loopStart % context.streamInfo.blockSizeSamples != 0) {
            std::cerr << "Loop start is in the middle of a block!" << std::endl;
            return;
        }
        auto loopEndBlock = context.streamInfo.loopEnd / context.streamInfo.blockSizeSamples;
        if (loopEndBlock + 1 != context.streamInfo.blockCountPerChannel) {
            std::cerr << "Loop end is not in the last block!" << std::endl;
            return;
        }
        if (context.streamInfo.loopEnd % context.streamInfo.blockSizeSamples !=
            context.streamInfo.lastBlockSizeSamples) {
            std::cerr << "Loop end is in the middle of a block!" << std::endl;
            return;
        }
    }

    if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
        m_Coefficients = std::make_unique_for_overwrite<int16_t[][8][2]>(context.streamInfo.channelNum);
        m_Yn = std::make_unique_for_overwrite<int16_t[][2]>(context.streamInfo.channelNum);
        initDsp(context, m_Coefficients, m_Yn);
    }
    auto writeFun = [this](void **data, uint32_t frames) {
        writeData(data, frames);
    };

    while (true) {
        m_Paused.wait(true);
        if (m_ShouldStop) break;
        m_WriteAudio.lock();
        uint32_t i = m_NextBlock;
        bool isLast = i + 1 == context.streamInfo.blockCountPerChannel;
        uint32_t off = i * context.streamInfo.channelNum * context.streamInfo.blockSizeBytes;
        if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
            decodeFrameBlockDSP(context.streamInfo.channelNum,
                                (isLast ? context.streamInfo.lastBlockSizeSamples
                                        : context.streamInfo.blockSizeSamples),
                                isLast ? context.streamInfo.lastBlockSizeBytesRaw
                                       : context.streamInfo.blockSizeBytes,
                                reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off), m_Coefficients,
                                m_Yn,
                                writeFun);
        } else {
            decodeFrameBlockSimple(context.streamInfo.channelNum,
                                   (isLast ? context.streamInfo.lastBlockSizeSamples
                                           : context.streamInfo.blockSizeSamples),
                                   isLast ? context.streamInfo.lastBlockSizeBytesRaw
                                          : context.streamInfo.blockSizeBytes,
                                   reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off),
                                   writeFun);
        }

        ++m_NextBlock;
        if (m_NextBlock == context.streamInfo.blockCountPerChannel) {
            m_WriteAudio.unlock();
            if (context.streamInfo.isLoop) {
                prepareLoop(context);
            } else {
                m_ShouldStop = true;
            }
        } else {
            m_WriteAudio.unlock();
        }

        double storedFrames = getDelayFrames() - context.streamInfo.blockSizeSamples;
        if (storedFrames >= 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>((storedFrames) / context.streamInfo.sampleRate));
        }
    }
    join();
}

void AudioPlayback::prepareLoop(const BfstmContext &context) {
    std::lock_guard<std::mutex> guard{m_WriteAudio};
    m_NextBlock = context.streamInfo.loopStart / context.streamInfo.blockSizeSamples;
    initLoopDspYn(context, m_Yn);
}

void AudioPlayback::seek(const BfstmContext &context, const void *histPtr, uint32_t block) {
    std::lock_guard<std::mutex> guard{m_WriteAudio};
    m_NextBlock = block;
    initDspYn(context, histPtr, block, m_Yn);
}

