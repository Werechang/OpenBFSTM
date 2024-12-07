//
// Created by cookieso on 06.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfgrpStructs.h"

class BfgrpWriter {
public:
    BfgrpWriter(MemoryResource& resource, const BfgrpWriteContext &context);
private:
    void writeHeader(const BfgrpWriteContext &context);

    uint32_t writeInfo(const BfgrpWriteContext &context);

    uint32_t writeFile(const BfgrpWriteContext &context);

    uint32_t writeInfx(const BfgrpWriteContext &context);

private:
    OutMemoryStream m_Stream;
};
