//
// Created by cookieso on 27.11.24.
//

#pragma once


#include "../MemoryResource.h"
#include "BfsarStructs.h"

class BfsarReader {
public:
    BfsarReader(const MemoryResource& resource);

    bool readSar();

    bool readHeader();

    bool readHeaderSections(BfsarHeader& header);

    bool readStrg();

    bool readLut();

    std::optional<BfsarStringEntry> readStrTbl();
private:
    InMemoryStream m_Stream;
};
