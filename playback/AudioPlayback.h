//
// Created by cookieso on 6/24/24.
//

#pragma once

#include <thread>
#include <memory>
#include <condition_variable>
#include "../format/bfstm/BfstmFile.h"

/**
 * This class is thread safe! It is recommended to call play() on a different thread.
 */
class AudioPlayback {
public:
    virtual void startDevice() const = 0;

    virtual void writeData(void **bufs, size_t frames) const = 0;

    /**
     * This method plays audio until stop() is called or until the audio playback is done (if it doesn't prepareLoop).
     * @param context
     * @param resource
     */
    void play(const BfstmContext &context, const void *resource);

    virtual void seek(const BfstmContext &context, const void *histPtr, uint32_t block);

    /**
     * Important! No check if this track has regions!
     */
    void incRegion();

    /**
     *
     * @param channelIndex No oob check! DIY
     */
    void setChannel(uint32_t channelIndex) {
        m_ChannelIndex = channelIndex;
    }

    virtual void stop() {
        m_ShouldStop = true;
        m_Paused = false;
        m_Paused.notify_all();
    }

    virtual void pause(bool enable) {
        m_Paused = enable;
        m_Paused.notify_all();
    }

    bool isPaused() {
        return m_Paused;
    }

    /**
     * @return The amount of frames stored in the audio device waiting to be played
     */
    virtual uint32_t getDelayFrames() = 0;

protected:
    /**
     * Only call this from the audio thread!
     */
    virtual void join() = 0;
    std::atomic_bool m_ShouldStop = false;
    std::atomic_bool m_Paused = false;
private:
    void prepareLoop(const BfstmContext &context);

    std::shared_ptr<int16_t[][8][2]> m_Coefficients;
    std::shared_ptr<int16_t[][2]> m_Yn;
    std::mutex m_WriteAudio;
    std::atomic_uint32_t m_NextBlock = 0;
    std::atomic_uint32_t m_RegionIdx = -1;
    std::atomic_uint32_t m_RegStartSample;
    std::atomic_uint32_t m_RegEndSample;
    std::atomic_uint32_t m_ChannelIndex = 0;
};
