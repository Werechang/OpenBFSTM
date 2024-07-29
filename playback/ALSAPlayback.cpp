//
// Created by cookieso on 29.07.24.
//

#include <iostream>
#include "ALSAPlayback.h"

ALSAPlayback::ALSAPlayback(const std::string &deviceName, const snd_pcm_format_t format, const unsigned int rate,
                             const unsigned int channelCount) {
    if ((err = snd_pcm_open(&m_PlaybackHandle, deviceName.c_str(), SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        std::cout << "Cannot open audio device!\n" << snd_strerror(err) << std::endl;
        return;
    }
    setHWParams(format, rate, channelCount);
}

ALSAPlayback::~ALSAPlayback() {
    snd_pcm_drain(m_PlaybackHandle);
    snd_pcm_close(m_PlaybackHandle);
}

void ALSAPlayback::setHWParams(const snd_pcm_format_t format, unsigned int rate,
                                const unsigned int channelCount) {
    snd_pcm_hw_params_t *hw_params;
    if ((err = snd_pcm_hw_params_malloc(&hw_params))) {
        std::cout << "Cannot allocate hardware parameters!\n" << snd_strerror(err) << std::endl;
        return;
    }
    if ((err = snd_pcm_hw_params_any(m_PlaybackHandle, hw_params)) < 0) {
        std::cout << "Cannot initialize hardware parameters!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    if ((err = snd_pcm_hw_params_set_access(m_PlaybackHandle, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED)) < 0) {
        std::cout << "Cannot set access type!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    if ((err = snd_pcm_hw_params_set_format(m_PlaybackHandle, hw_params, format)) < 0) {
        std::cout << "Cannot set format!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    if ((err = snd_pcm_hw_params_set_rate_near(m_PlaybackHandle, hw_params, &rate, nullptr)) < 0) {
        std::cout << "Cannot set rate to " << rate << "!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    if ((err = snd_pcm_hw_params_set_channels(m_PlaybackHandle, hw_params, channelCount)) < 0) {
        std::cout << "Cannot set channel count to " << channelCount << "!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    if ((err = snd_pcm_hw_params(m_PlaybackHandle, hw_params)) < 0) {
        std::cout << "Cannot set hardware parameters!\n" << snd_strerror(err) << std::endl;
        snd_pcm_hw_params_free(hw_params);
        return;
    }
    snd_pcm_hw_params_free(hw_params);
}

void ALSAPlayback::startDevice() const {
    snd_pcm_start(m_PlaybackHandle);
}

void ALSAPlayback::writeData(void **bufs, const size_t frames) const {
    if (snd_pcm_writen(m_PlaybackHandle, bufs, frames) == -EPIPE) {
        snd_pcm_prepare(m_PlaybackHandle);
    }
}

std::vector<std::string> ALSAPlayback::getDevices() {
    char **hints = nullptr;
    snd_device_name_hint(-1, "pcm", reinterpret_cast<void ***>(&hints));
    char **it = hints;
    std::vector<std::string> deviceNames;
    while (*it != nullptr) {
        if (char *name = snd_device_name_get_hint(*it, "NAME")) {
            deviceNames.emplace_back(name);
            free(name);
        }
        ++it;
    }
    snd_device_name_free_hint(reinterpret_cast<void **>(hints));
    return deviceNames;
}
