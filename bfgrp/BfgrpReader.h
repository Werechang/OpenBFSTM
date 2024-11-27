//
// Created by cookieso on 01.07.24.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include "BfgrpStructs.h"
#include "../MemoryResource.h"

class BfgrpReader {
public:
    BfgrpReader(const MemoryResource& resource);

    bool readHeader();

    bool readHeaderSections(BfgrpHeader& header);

    bool readInfo();

    void readFileLocationInfo();

    void readGroupItemExtraInfo();
private:
    InMemoryStream m_Stream;
};
