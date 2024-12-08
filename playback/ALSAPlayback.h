//
// Created by cookieso on 29.07.24.
//

#if __has_include(<alsa/asoundlib.h>)

#pragma once

#include "AudioPlayback.h"

extern "C" {
#include <alsa/asoundlib.h>
}

class ALSAPlayback : public AudioPlayback {
public:
    ALSAPlayback(const std::string &deviceName, snd_pcm_format_t format, unsigned int rate,
                 unsigned int channelCount);

    ~ALSAPlayback();

    void startDevice() const override;

    void writeData(void **bufs, size_t frames) const override;

    void stop() override;

    void pause(bool enable) override;

    void seek(const BfstmContext &context, const void *histPtr, uint32_t block) override;

    void join() override;

    uint32_t getDelayFrames() override;

    static std::vector<std::string> getDevices();

protected:
private:
    void setHWParams(snd_pcm_format_t format, unsigned int rate, unsigned int channelCount);

    snd_pcm_t *m_PlaybackHandle{};
    int err;
};

#endif