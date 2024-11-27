//
// Created by cookieso on 6/27/24.
//

#pragma once
#include <cstdint>
#include "MemoryResource.h"

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

ReferenceEntry readReferenceEntry(InMemoryStream &stream);

SectionInfo readSectionInfo(InMemoryStream &stream);
