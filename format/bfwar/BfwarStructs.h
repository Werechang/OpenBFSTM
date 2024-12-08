//
// Created by cookieso on 08.12.24.
//

#pragma once

#include <cstdint>
#include <vector>
#include <span>

struct BfwarFile {
    int32_t offset;
    uint32_t size;
};

struct BfwarReadContext {
    std::vector<BfwarFile> fileData;
};

struct BfwarWriteContext {
    std::vector<std::span<const uint8_t>> files;
};