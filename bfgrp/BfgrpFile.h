//
// Created by cookieso on 6/27/24.
//

#pragma once
#include <cstdint>
#include "../BfFile.h"

struct BfgrpDepEntry {
    uint32_t id;
    uint32_t loadFlags;
};

struct BfgrpInfoEx {
    uint32_t magic;
    uint32_t size;
    uint32_t tableNum;
    BfgrpDepEntry entries[];
};

struct BfgrpFileEntry {
    uint32_t id;
    int32_t offset;
};

struct BfgrpInfo {
    uint32_t magic;
    uint32_t size;
    uint32_t tableNum;
    BfgrpFileEntry entries[];
};

struct BfgrpHeader {
    uint32_t magic;
    uint16_t bom;
    uint16_t headerSize;
    uint32_t version;
    uint32_t fileSize;
    uint16_t sectionNum;
    uint16_t _pad0;
    SectionInfo sectionInfo[];
};
