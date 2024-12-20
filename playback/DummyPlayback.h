//
// Created by cookieso on 08.12.24.
//

#pragma once


#include "AudioPlayback.h"

class DummyPlayback : public AudioPlayback {
public:
    void startDevice() const override {}

    void writeData(void **bufs, size_t frames) const override {}

    void seek(const BfstmContext &context, const void *histPtr, uint32_t block) override {}

    void stop() override {
        AudioPlayback::stop();
    }

    void pause(bool enable) override {
        AudioPlayback::pause(enable);
    }

    uint32_t getDelayFrames() override {
        return 0xffffffff;
    }

protected:
    void join() override {}
};
