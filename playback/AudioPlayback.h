//
// Created by cookieso on 6/24/24.
//

#pragma once

#include "../BfstmFile.h"

class AudioPlayback {
public:
    virtual void startDevice() const = 0;

    virtual void writeData(void **bufs, size_t frames) const = 0;
};
