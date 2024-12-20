//
// Created by cookieso on 27.11.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfsarStructs.h"

class BfsarReader {
public:
    explicit BfsarReader(const MemoryResource &resource);

    bool wasReadSuccess() {
        return m_Context.has_value();
    }

    const BfsarContext& getContext() {
        return m_Context.value();
    }
private:
    std::optional<BfsarContext> readHeader();

    std::optional<BfsarContext> readHeaderSections();

    std::optional<std::vector<std::string>> readStrg();

    bool readLut(const std::vector<std::string>& strTable);

    void printLutEntry(uint32_t baseOff, const std::string &prefix, bool isLeft, const std::vector<std::string>& strTable);

    std::optional<BfsarContext> readInfo(const std::vector<std::string> &stringTable);

    std::optional<BfsarSound> readSoundInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData);

    std::optional<BfsarStreamSound> readStreamSoundInfo();

    BfsarWaveSound readWaveSoundInfo();

    BfsarSequenceSound readSequenceSoundInfo();

    BfsarSound3D readSound3DInfo(uint32_t startOff);

    BfsarSoundGroup
    readSoundGroupInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData);

    BfsarBank readBankInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData);

    BfsarWaveArchive readWaveArchiveInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData);

    std::vector<uint32_t> readBankIdTable();

    BfsarGroup readGroupInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData);

    BfsarPlayer readPlayerInfo(const std::vector<std::string> &stringTable);

    std::optional<BfsarFileInfo> readFileInfo();

    BfsarSoundArchivePlayer readSoundArchivePlayerInfo();

    std::vector<uint32_t> readInfoRef(uint16_t requiredType);

    uint32_t readFile();

    std::optional<BfsarStringEntry> readStrTbl();

    static bool hasFlag(uint32_t flags, uint8_t index);

private:
    InMemoryStream m_Stream;
    std::optional<BfsarContext> m_Context;
    uint32_t m_FileOffset = 0;
};
