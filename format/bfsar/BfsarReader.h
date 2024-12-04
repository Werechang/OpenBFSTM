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

    std::optional<std::vector<int32_t>> readInfo();

    SoundInfo readSoundInfo();

    SoundGroupInfo readSoundGroupInfo();

    BankInfo readBankInfo();

    WaveArchiveInfo readWaveArchiveInfo();

    bool readGroupInfo();

    bool readPlayerInfo();

    int32_t readFileInfo();

    bool readSoundArchivePlayerInfo();

    std::vector<uint32_t> readInfoRef(uint16_t requiredType);

    bool readFile(const std::vector<int32_t> &fileOffsets);

    std::optional<BfsarStringEntry> readStrTbl();
private:
    InMemoryStream m_Stream;
};
