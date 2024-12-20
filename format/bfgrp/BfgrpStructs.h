//
// Created by cookieso on 6/27/24.
//

#pragma once
#include <cstdint>
#include "../../BfFile.h"

struct BfgrpDepEntry {
    uint32_t id;
    uint32_t loadFlags;
};

struct BfgrpNestedFile {
    uint32_t fileIndex;
    std::span<const uint8_t> file;
};

struct BfgrpContext {
    std::vector<BfgrpNestedFile> files{};
    std::vector<BfgrpDepEntry> dependencies{};
};
