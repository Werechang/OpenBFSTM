//
// Created by cookieso on 27.11.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfsarStructs.h"

class BfsarReader {
public:
    BfsarReader(const MemoryResource& resource);

    bool readSar();

    bool readHeader();

    bool readHeaderSections(BfsarHeader& header);

    bool readStrg();

    bool readLut();

    void printLutEntry(uint32_t baseOff, const std::string& prefix, bool isLeft);

    bool readInfo();

    bool readSoundInfo();

    bool readSoundGroupInfo();

    bool readBankInfo();

    std::vector<uint32_t> readInfoRef(uint16_t requiredType);

    bool readFile();

    std::optional<BfsarStringEntry> readStrTbl();
private:
    InMemoryStream m_Stream;
};
