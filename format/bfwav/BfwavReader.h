//
// Created by cookieso on 08.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfwavStructs.h"

class BfwavReader {
public:
    explicit BfwavReader(const MemoryResource& resource);

    bool wasReadSuccess() {
        return m_Context.has_value();
    }

    BfwavReadContext getContext() {
        return m_Context.value();
    }
private:
    std::optional<BfwavReadContext> readHeader();

    std::optional<BfwavReadContext> readHeaderSections();

    std::optional<BfwavReadContext> readInfo();

    uint32_t readData();
private:
    InMemoryStream m_Stream;
    std::optional<BfwavReadContext> m_Context;
    uint32_t m_DataOffset = 0;
};
