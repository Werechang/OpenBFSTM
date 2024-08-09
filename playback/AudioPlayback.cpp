//
// Created by cookieso on 01.08.24.
//
#include "AudioPlayback.h"
#include "../MemoryResource.h"
#include "PlaybackFunctions.h"

void AudioPlayback::play(const BfstmContext &context, const void *dataPtr) {
    if (context.streamInfo.isLoop) {
        if (context.streamInfo.loopStart % context.streamInfo.blockSizeSamples != 0) {
            std::cerr << "Loop start is in the middle of a block!" << std::endl;
            return;
        }
        auto loopEndBlock = context.streamInfo.sampleCount / context.streamInfo.blockSizeSamples;
        if (loopEndBlock + 1 != context.streamInfo.blockCountPerChannel) {
            std::cerr << "Loop end is not in the last block!" << std::endl;
            return;
        }
        if (context.streamInfo.sampleCount % context.streamInfo.blockSizeSamples !=
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

    if (!context.regionInfos.empty()) {
        incRegion();
        m_RegEndSample = context.regionInfos[0].first.endSample - 1;
    }

    auto writeFun = [this](void **data, uint32_t frames) {
        writeData(data, frames);
    };

    uint32_t sampleSize = context.streamInfo.soundEncoding == SoundEncoding::PCM16 ? 2 : 1;
    uint32_t maxChannels = context.streamInfo.channelNum < 2 ? 1 : 2;

    while (true) {
        m_Paused.wait(true);
        if (m_ShouldStop) break;
        m_WriteAudio.lock();
        uint32_t i = m_NextBlock;
        bool isLast = i + 1 == context.streamInfo.blockCountPerChannel;
        uint32_t off = i * context.streamInfo.channelNum * context.streamInfo.blockSizeBytes;
        uint32_t frameCount = isLast ? context.streamInfo.lastBlockSizeSamples
                                     : context.streamInfo.blockSizeSamples;
        uint32_t thisBlockSize = isLast ? context.streamInfo.lastBlockSizeBytesRaw
                                        : context.streamInfo.blockSizeBytes;
        uint32_t startSample = i * context.streamInfo.blockSizeSamples;
        uint32_t endSample = startSample + frameCount;
        uint32_t startSampleInBlock = 0;
        if (m_RegionIdx != -1 && startSample <= m_RegStartSample && endSample >= m_RegStartSample) {
            startSampleInBlock = m_RegStartSample - startSample;
            if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
                m_WriteAudio.unlock();
                initRegionDspYn(context, m_RegionIdx, m_Yn);
                m_WriteAudio.lock();
            }
        }

        bool isRegionEnd = false;
        if (m_RegionIdx != -1 && startSample <= m_RegEndSample && endSample >= m_RegEndSample) {
            frameCount = m_RegEndSample - startSample + 1;
            isRegionEnd = true;
        }
        frameCount -= startSampleInBlock;

        //std::cout << m_NextBlock << ": Start sample: " << startSampleInBlock << " End Sample: " << frameCount - 1 + startSampleInBlock << std::endl;

        if (context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM) {
            decodeFrameBlockDSP(maxChannels, m_ChannelIndex, frameCount, startSampleInBlock, thisBlockSize,
                                reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off),
                                m_Coefficients, m_Yn,
                                writeFun);
        } else {
            decodeFrameBlockSimple(maxChannels, m_ChannelIndex, frameCount, startSampleInBlock / sampleSize,
                                   thisBlockSize,
                                   reinterpret_cast<const void *>(reinterpret_cast<size_t>(dataPtr) + off),
                                   writeFun);
        }

        ++m_NextBlock;
        if (isRegionEnd) {
            const auto &reg = context.regionInfos[m_RegionIdx].first;
            uint32_t predicted = m_NextBlock;
            m_NextBlock = reg.startSample / context.streamInfo.blockSizeSamples;
            if (m_NextBlock >= predicted || (m_NextBlock == predicted - 1 && frameCount != context.streamInfo.blockSizeSamples)) {
                m_RegStartSample = reg.startSample;
                m_RegEndSample = reg.endSample - 1;
            }
            m_WriteAudio.unlock();
            initRegionDspYn(context, m_RegionIdx, m_Yn);
        } else if (m_NextBlock == context.streamInfo.blockCountPerChannel) {
            m_WriteAudio.unlock();
            if (context.streamInfo.isLoop) {
                prepareLoop(context);
            } else {
                m_ShouldStop = true;
            }
        } else {
            m_WriteAudio.unlock();
        }

        double storedFrames = static_cast<int32_t>(getDelayFrames()) - static_cast<int32_t>(context.streamInfo.blockSizeSamples);
        if (storedFrames >= 0) {
            std::this_thread::sleep_for(std::chrono::duration<double>((storedFrames) / context.streamInfo.sampleRate));
        }
    }
    join();
    std::cout << "Audio playback finished." << std::endl;
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

void AudioPlayback::incRegion() {
    std::lock_guard<std::mutex> guard{m_WriteAudio};
    ++m_RegionIdx;
}

