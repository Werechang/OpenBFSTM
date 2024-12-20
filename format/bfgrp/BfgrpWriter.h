//
// Created by cookieso on 06.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfgrpStructs.h"

class BfgrpWriter {
public:
    BfgrpWriter(MemoryResource& resource, const BfgrpContext &context);
private:
    void writeHeader(const BfgrpContext &context);

    uint32_t writeInfo(const BfgrpContext &context);

    uint32_t writeFile(const BfgrpContext &context);

    uint32_t writeInfx(const BfgrpContext &context);

private:
    OutMemoryStream m_Stream;
};
