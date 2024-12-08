//
// Created by cookieso on 07.12.24.
//

#pragma once


#include "../../MemoryResource.h"

class BfstpReader {
public:
    explicit BfstpReader(const MemoryResource& resource);

    bool wasReadSuccess();
private:
    bool readHeader();

    bool readInfo();

    bool readRegion();

    bool readData();
private:
    InMemoryStream m_Stream;

};
