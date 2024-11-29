//
// Created by cookieso on 27.11.24.
//

#pragma once


#include "BfstmFile.h"
#include "../../MemoryResource.h"

class BfstmReader {
public:
    BfstmReader(const MemoryResource &resource);

    bool readBfstm();

    bool readHeader();

    void readStreamInfo();

    BfstmContext m_Context;
    bool success = true;
private:
    InMemoryStream m_Stream;
};
