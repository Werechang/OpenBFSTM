//
// Created by cookieso on 6/27/24.
//

#pragma once
#include <cstdint>

struct SectionInfo {
    uint16_t flag;
    int32_t offset;
    uint32_t size;
};

struct ReferenceEntry {
    uint16_t flag;
    int32_t offset;
};

struct ReferenceTable {
    uint32_t refCount;
    ReferenceEntry entries[];
};


