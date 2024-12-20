//
// Created by cookieso on 08.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfwsdStructs.h"

class BfwsdReader {
public:
    explicit BfwsdReader(const MemoryResource& resource);

    bool wasReadSuccess() {
        return m_Context.has_value();
    }

    BfwsdReadContext getContext() {
        return m_Context.value();
    }
private:
    std::optional<BfwsdReadContext> readHeader();

    std::optional<BfwsdReadContext> readHeaderSections();

    std::optional<BfwsdReadContext> readInfo();

    uint32_t readData();
private:
    InMemoryStream m_Stream;
    std::optional<BfwsdReadContext> m_Context;
    uint32_t m_DataOffset = 0;
};
