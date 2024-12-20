//
// Created by cookieso on 01.07.24.
//

#pragma once

#include <cstdint>
#include <cstddef>
#include "BfgrpStructs.h"
#include "../../MemoryResource.h"

class BfgrpReader {
public:
    explicit BfgrpReader(const MemoryResource& resource);

    bool wasReadSuccess() {
        return m_Context.has_value();
    }

    BfgrpContext getContext() {
        return m_Context.value();
    }

private:
    std::optional<BfgrpContext> readHeader();

    std::optional<BfgrpContext> readHeaderSections();

    std::optional<std::vector<BfgrpNestedFile>> readInfo();

    uint32_t readFile();

    std::optional<BfgrpNestedFile> readFileLocationInfo();

    std::optional<std::vector<BfgrpDepEntry>> readGroupItemExtraInfo();
private:
    InMemoryStream m_Stream;
    std::optional<BfgrpContext> m_Context;
    uint32_t m_FileOffset = 0;
};
