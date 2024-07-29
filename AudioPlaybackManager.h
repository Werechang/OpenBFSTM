//
// Created by cookieso on 6/24/24.
//

#pragma once

#include <string>
#include <vector>

extern "C" {
#include <alsa/asoundlib.h>
}

class AudioPlaybackManager {
public:
    AudioPlaybackManager(const std::string &deviceName, snd_pcm_format_t format, unsigned int rate,
                         unsigned int channelCount);

    ~AudioPlaybackManager();

    void startDevice() const;

    void writeData(void **bufs, size_t frames) const;

    void setSWParams();

    static std::vector<std::string> getDevices();

private:
    void setHWParams(snd_pcm_format_t format, unsigned int rate, unsigned int channelCount);

    snd_pcm_t *m_PlaybackHandle{};
    int err;
};
