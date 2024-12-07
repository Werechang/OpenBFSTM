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

struct BfgrpFileEntry {
    uint32_t fileIndex;
    int32_t offset;
    uint32_t size;
};

struct BfgrpNestedFile {
    uint32_t fileIndex;
    std::span<uint8_t> file;
};

struct BfgrpWriteContext {
    std::vector<BfgrpNestedFile> files{};
    std::vector<BfgrpDepEntry> dependencies{};
};

struct BfgrpReadContext {
    std::vector<BfgrpFileEntry> fileData{};
    std::vector<BfgrpDepEntry> dependencies{};
};
