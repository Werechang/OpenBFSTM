//
// Created by cookieso on 01.07.24.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include "BfgrpFile.h"
#include "../MemoryResource.h"

class BfgrpReader {
public:
    BfgrpReader(const MemoryResource& resource);

    void readGroupItemLocInfo(int index);

    void readGroupItemExtraInfo();
private:
    InMemoryStream m_Stream;
    BfgrpHeader *m_Header;
    std::optional<SectionInfo> m_Info, m_File, m_InfoEx;
};
