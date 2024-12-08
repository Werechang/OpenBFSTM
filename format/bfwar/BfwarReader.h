//
// Created by cookieso on 08.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfwarStructs.h"

class BfwarReader {
public:
    explicit BfwarReader(const MemoryResource& resource);

    bool wasReadSuccess() {
        return m_Context.has_value();
    }

    BfwarReadContext getContext() {
        return m_Context.value();
    }

    std::span<const uint8_t> getFileData(uint32_t offset, uint32_t size);
private:
    std::optional<BfwarReadContext> readHeader();

    std::optional<BfwarReadContext> readHeaderSections();

    std::optional<std::vector<BfwarFile>> readInfo();

    uint32_t readFile();
private:
    InMemoryStream m_Stream;
    std::optional<BfwarReadContext> m_Context;
    uint32_t m_FileOffset = 0;
};
