//
// Created by cookieso on 01.07.24.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include "BfgrpFile.h"

class BfgrpReader {
public:
    BfgrpReader(uint8_t *data, size_t size);

    void readGroupItemLocInfo(int index);
private:
    uint8_t *m_Data;
    size_t m_Size;
    BfgrpHeader *m_Header;
    std::optional<SectionInfo> m_Info, m_File, m_InfoEx;
};
