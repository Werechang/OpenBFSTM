//
// Created by cookieso on 29.07.24.
//

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

    uint32_t getDelayFrames() override;

    static std::vector<std::string> getDevices();
protected:
    void join() override;
private:
    void setHWParams(snd_pcm_format_t format, unsigned int rate, unsigned int channelCount);

    snd_pcm_t *m_PlaybackHandle{};
    int err;
};
