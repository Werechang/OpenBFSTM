//
// Created by cookieso on 08.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfwarStructs.h"

class BfwarWriter {
public:
    BfwarWriter(MemoryResource& resource, const BfwarWriteContext &context);
private:
    void writeHeader(const BfwarWriteContext &context);

    uint32_t writeInfo(const BfwarWriteContext &context);

    uint32_t writeFile(const BfwarWriteContext &context);

    OutMemoryStream m_Stream;
};
