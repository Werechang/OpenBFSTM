//
// Created by cookieso on 03.12.24.
//

#pragma once


#include "../../MemoryResource.h"

class BfsarWriter {
public:
    explicit BfsarWriter(MemoryResource &resource);

    void writeHeader();

    void writeStrg();
private:
    OutMemoryStream m_Stream;
};
