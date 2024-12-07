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

    BfgrpReadContext getContext() {
        return m_Context.value();
    }

    std::span<const uint8_t> getFileData(uint32_t offset, uint32_t size);

private:
    std::optional<BfgrpReadContext> readHeader();

    std::optional<BfgrpReadContext> readHeaderSections();

    std::optional<std::vector<BfgrpFileEntry>> readInfo();

    uint32_t readFile(const std::vector<BfgrpFileEntry>& entries);

    std::optional<BfgrpFileEntry> readFileLocationInfo();

    std::optional<std::vector<BfgrpDepEntry>> readGroupItemExtraInfo();
private:
    InMemoryStream m_Stream;
    std::optional<BfgrpReadContext> m_Context;
    uint32_t m_FileOffset = 0;
};
