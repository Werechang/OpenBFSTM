//
// Created by cookieso on 29.07.24.
//

#pragma once

#include "AudioPlayback.h"
extern "C" {
//#include <pipewire-0.3/pipewire/pipewire.h>
}
class PipeWirePlayback : public AudioPlayback {
public:
    PipeWirePlayback(const std::string &deviceName, int format, unsigned int rate,
                     unsigned int channelCount);

    void startDevice() const override;

    void writeData(void **bufs, size_t frames) const override;
};
