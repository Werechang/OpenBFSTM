//
// Created by cookieso on 6/24/24.
//

#pragma once

#include <thread>
#include <memory>
#include <condition_variable>
#include "../BfstmFile.h"

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
    void play(const BfstmContext &context, MemoryResource &resource);

    virtual void seek(const BfstmContext &context, const void *histPtr, uint32_t block);

    virtual void stop() {
        m_ShouldStop = true;
        m_Paused = false;
        m_Paused.notify_all();
    }

    virtual void pause(bool enable) {
        m_Paused = enable;
        m_Paused.notify_all();
    }

    /**
     * @return The amount of frames stored in the audio device waiting to be played
     */
    virtual uint32_t getDelayFrames() = 0;
protected:
    virtual void join() = 0;

    std::atomic_bool m_ShouldStop = false;
    std::atomic_bool m_Paused = false;
private:
    void prepareLoop(const BfstmContext &context);

    std::shared_ptr<int16_t[][8][2]> m_Coefficients;
    std::shared_ptr<int16_t[][2]> m_Yn;
    std::mutex m_WriteAudio;
    std::atomic_uint32_t m_NextBlock = 0;
};
