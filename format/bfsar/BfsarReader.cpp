//
// Created by cookieso on 27.11.24.
//

#include <bitset>
#include "BfsarReader.h"
#include "../../BfFile.h"

BfsarReader::BfsarReader(const MemoryResource &resource) : m_Stream(resource) {
    m_Context = readHeader();
}

std::optional<BfsarReadContext> BfsarReader::readHeader() {
    uint32_t magic = m_Stream.readU32();
    uint16_t bom = m_Stream.readU16();
    if (bom != 0xFEFF) {
        if (bom != 0xFFFE) return std::nullopt;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        magic = __builtin_bswap32(magic);
    }
    uint16_t headerSize = m_Stream.readU16();
    uint32_t version = m_Stream.readU32();
    if (version != 0x020400 && version != 0x020200)
        std::cout << "Warning: BFSAR version might not be supported. (0x" << std::hex << version << std::dec
                  << ')'
                  << std::endl;
    uint32_t fileSize = m_Stream.readU32();

    return readHeaderSections();
}


std::optional<BfsarReadContext> BfsarReader::readHeaderSections() {
    uint16_t sectionNum = m_Stream.readU16();
    m_Stream.skip(2);

    std::optional<SectionInfo> strgSection, infoSection, fileSection;
    for (int i = 0; i < sectionNum; ++i) {
        switch (auto section = readSectionInfo(m_Stream); section.flag) {
            case 0x2000:
                strgSection = section;
                break;
            case 0x2001:
                infoSection = section;
                break;
            case 0x2002:
                fileSection = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << std::endl;
                return std::nullopt;
        }
    }
    m_Stream.seek(strgSection->offset);
    auto stringTable = readStrg();
    if (!stringTable) return std::nullopt;
    m_Stream.seek(infoSection->offset);
    auto context = readInfo(stringTable.value());
    m_Stream.seek(fileSection->offset);
    m_FileOffset = readFile();

    return context;
}

std::optional<std::vector<std::string>> BfsarReader::readStrg() {
    uint32_t magic = m_Stream.readU32();
    uint32_t sectionSize = m_Stream.readU32();
    uint32_t strgOff = m_Stream.tell();
    if (m_Stream.readU16() != 0x2400) {
        std::cerr << "First entry in string section is not string table!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t strTblOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2401) {
        std::cerr << "Second entry in string section is not lookup table!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t lutOff = m_Stream.readS32();

    m_Stream.seek(strgOff + strTblOff);
    uint32_t entryCount = m_Stream.readU32();
    std::vector<BfsarStringEntry> stringEntries;
    for (int i = 0; i < entryCount; ++i) {
        auto entry = readStrTbl();
        if (entry.has_value()) {
            stringEntries.emplace_back(entry.value());
        }
    }
    std::vector<std::string> stringTable{};
    for (auto entry: stringEntries) {
        m_Stream.seek(strgOff + strTblOff + entry.offset);
        std::string entryName;
        for (int i = 0; i < entry.size - 1; ++i) {
            entryName += m_Stream.readS8();
        }
        stringTable.emplace_back(entryName);
    }
    //m_Stream.seek(strgOff + lutOff);
    //readLut();
    return stringTable;
}


std::optional<BfsarStringEntry> BfsarReader::readStrTbl() {
    if (m_Stream.readU16() != 0x1f01) {
        std::cerr << "String Table magic incorrect!" << std::endl;
        return {};
    }
    m_Stream.skip(2);
    int32_t offset = m_Stream.readS32();
    uint32_t size = m_Stream.readU32();

    return BfsarStringEntry{offset, size};
}

std::string getTypeAsString(uint16_t type) {
    switch (type) {
        case 0:
            return "Null";
        case 1:
            return "Audio";
        case 2:
            return "Sequence";
        case 3:
            return "Bank";
        case 4:
            return "Player";
        case 5:
            return "Wave Arc";
        case 6:
            return "Group";
        default:
            return "Unk";
    }
}

void BfsarReader::printLutEntry(uint32_t baseOff, const std::string &prefix, bool isLeft) {
    uint16_t isLeaf = m_Stream.readU8();
    m_Stream.skip(1);
    uint16_t compareFunc = m_Stream.readU16();
    uint32_t leftChild = m_Stream.readU32();
    uint32_t rightChild = m_Stream.readU32();
    uint32_t strTblIdx = m_Stream.readU32();
    uint32_t itemId = m_Stream.readU32();
    std::cout << prefix << (isLeft ? "├──" : "└──");
    if (itemId != 0xffffffffu) {
        uint32_t type = itemId >> 24;
        // Audio/Prefetch+Sound: BgmData.bfsar
        // Sequence/Sound Group: BgmData.bfsar
        // Bank: FootNote.bfsar, AtmosCityDay.bfsar, AtmosClash.bfsar, AtmosLava.bfsar
        // Player: BgmData.bfsar
        // Wave Archive:
        // Group: BgmData.bfsar
        std::cout << getTypeAsString(type);
        std::cout << " (str index: " << std::hex << strTblIdx << ", id: " << itemId
                  << ")" << std::endl;
    } else {
        std::cout << std::dec << (compareFunc >> 3) << "*" << (~compareFunc & 7) << std::endl;
    }
    if (!isLeaf) {
        m_Stream.seek(baseOff + leftChild * 0x5 * 0x4);
        printLutEntry(baseOff, prefix + (isLeft ? "│   " : "    "), true);
        m_Stream.seek(baseOff + rightChild * 0x5 * 0x4);
        printLutEntry(baseOff, prefix + (isLeft ? "│   " : "    "), false);
    } else if (itemId == 0xffffffffu) {
        std::cerr << "Leaf node has item id!" << std::endl;
    }
}

bool BfsarReader::readLut() {
    uint32_t rootIndex = m_Stream.readU32();
    uint32_t entryCount = m_Stream.readU32();
    if (rootIndex == 0xffffffff) {
        return false;
    }
    uint32_t lutStartOff = m_Stream.tell();
    m_Stream.skip(rootIndex * 0x5 * 0x4);
    printLutEntry(lutStartOff, "", false);
    return true;
}

std::optional<BfsarReadContext> BfsarReader::readInfo(const std::vector<std::string> &stringTable) {
    uint32_t magic = m_Stream.readU32();
    uint32_t size = m_Stream.readU32();
    uint32_t infoOff = m_Stream.tell();
    if (m_Stream.readU16() != 0x2100) {
        std::cerr << "First entry in info section is not sound!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t soundOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2104) {
        std::cerr << "Second entry in info section is not sound group!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t soundGroupOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2101) {
        std::cerr << "Third entry in info section is not bank!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t bankOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2103) {
        std::cerr << "Fourth entry in info section is not wave archive!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t waveArcOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2105) {
        std::cerr << "Fifth entry in info section is not group info!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t groupInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2102) {
        std::cerr << "Sixth entry in info section is not player info!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t playerInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2106) {
        std::cerr << "Seventh entry in info section is not file info!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t fileInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x220B) {
        std::cerr << "Eight entry in info section is not sound archive player info!" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    int32_t soundArcPlayerInfoOff = m_Stream.readS32();

    std::vector<BfsarFileInfo> fileData{};
    m_Stream.seek(fileInfoOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x220a)) {
        m_Stream.seek(fileInfoOff + infoOff + infoRef);
        auto result = readFileInfo();
        if (!result) return std::nullopt;
        fileData.emplace_back(result.value());
    }

    BfsarReadContext context{};

    m_Stream.seek(soundOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2200)) {
        m_Stream.seek(soundOff + infoOff + infoRef);
        auto res = readSoundInfo(stringTable, fileData);
        if (!res) return std::nullopt;
        context.sounds.emplace_back(res.value());
    }

    m_Stream.seek(soundGroupOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2204)) {
        m_Stream.seek(soundGroupOff + infoOff + infoRef);
        context.soundGroups.emplace_back(readSoundGroupInfo(stringTable, fileData));
    }

    m_Stream.seek(bankOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2206)) {
        m_Stream.seek(bankOff + infoOff + infoRef);
        context.banks.emplace_back(readBankInfo(stringTable, fileData));
    }

    m_Stream.seek(waveArcOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2207)) {
        m_Stream.seek(waveArcOff + infoOff + infoRef);
        context.waveArchives.emplace_back(readWaveArchiveInfo(stringTable, fileData));
    }

    m_Stream.seek(groupInfoOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2208)) {
        m_Stream.seek(groupInfoOff + infoOff + infoRef);
        context.groups.emplace_back(readGroupInfo(stringTable, fileData));
    }

    m_Stream.seek(playerInfoOff + infoOff);
    for (uint32_t infoRef: readInfoRef(0x2209)) {
        m_Stream.seek(playerInfoOff + infoOff + infoRef);
        context.players.emplace_back(readPlayerInfo(stringTable));
    }

    m_Stream.seek(soundArcPlayerInfoOff + infoOff);
    readSoundArchivePlayerInfo();

    return context;
}

uint32_t BfsarReader::readFile() {
    uint32_t magic = m_Stream.readU32();
    uint32_t size = m_Stream.readU32();
    return m_Stream.tell();
}

std::vector<uint32_t> BfsarReader::readInfoRef(uint16_t requiredType) {
    uint32_t entryCount = m_Stream.readU32();
    std::vector<uint32_t> entryOffsets{};
    for (int i = 0; i < entryCount; ++i) {
        uint16_t type = m_Stream.readU16();
        if (type != requiredType) {
            std::cerr << std::hex << "Entry type is " << type << " but required was " << requiredType << std::endl;
            m_Stream.skip(6);
            continue;
        }
        m_Stream.skip(2);
        entryOffsets.emplace_back(m_Stream.readS32());
    }
    return entryOffsets;
}

std::optional<BfsarSound>
BfsarReader::readSoundInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData) {
    BfsarSound sound{};
    uint32_t startOff = m_Stream.tell();
    sound.fileData = fileData[m_Stream.readU32()];
    sound.playerId = m_Stream.readU32();
    sound.initialVolume = m_Stream.readU8();
    sound.remoteFilter = m_Stream.readU8();
    m_Stream.skip(2);
    uint32_t soundType = m_Stream.readU16();
    m_Stream.skip(2);
    uint32_t infoOffset = m_Stream.readU32();

    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        sound.name = stringTable[m_Stream.readU32()];
    }
    if (hasFlag(flags, 1)) {
        BfsarPan pan{};
        pan.mode = m_Stream.readU8();
        pan.curve = m_Stream.readU8();
        m_Stream.skip(2);
        sound.panInfo = pan;
    }
    if (hasFlag(flags, 2)) {
        BfsarActorPlayer actorPlayer{};
        actorPlayer.playerPriority = m_Stream.readU8();
        actorPlayer.actorPlayerId = m_Stream.readU8();
        m_Stream.skip(2);
        sound.actorPlayerInfo = actorPlayer;
    }
    if (hasFlag(flags, 3)) {
        BfsarSinglePlay singlePlay{};
        singlePlay.type = m_Stream.readU16();
        singlePlay.effectiveDuration = m_Stream.readU16();
        sound.singlePlayInfo = singlePlay;
    }
    uint32_t sum = 0;
    for (int i = 4; i < 8; ++i) {
        if (hasFlag(flags, i)) sum += 4;
    }
    m_Stream.skip(sum);
    if (hasFlag(flags, 8)) {
        sound.sound3DInfo = readSound3DInfo(startOff);
    }
    sum = 0;
    for (int i = 9; i < 17; ++i) {
        if (hasFlag(flags, i)) sum += 4;
    }
    m_Stream.skip(sum);
    if (hasFlag(flags, 17)) {
        sound.isFrontBypass = m_Stream.readU32();
    }

    m_Stream.seek(infoOffset + startOff);

    switch (soundType) {
        case 0x2201: {
            auto strRet = readStreamSoundInfo();
            if (!strRet) return std::nullopt;
            sound.subInfo = strRet.value();
            break;
        }
        case 0x2202: {
            sound.subInfo = readWaveSoundInfo();
            break;
        }
        case 0x2203: {
            sound.subInfo = readSequenceSoundInfo();
            break;
        }
        default:
            std::cerr << "Unknown sound type " << std::hex << soundType << std::endl;
            return std::nullopt;
    }

    return sound;
}

std::optional<BfsarStreamSound> BfsarReader::readStreamSoundInfo() {
    // TODO check with bfstm/bfstp reader
    BfsarStreamSound streamSound{};
    uint32_t start = m_Stream.tell();
    streamSound.validTracks = m_Stream.readU16();
    streamSound.channelCount = m_Stream.readU16();
    uint16_t titFlag = m_Stream.readU16();
    m_Stream.skip(2);
    int32_t titOff = m_Stream.readS32();
    auto unkFloat = std::bit_cast<float>(m_Stream.readU32());
    uint16_t svFlag = m_Stream.readU16();
    m_Stream.skip(2);
    int32_t svOff = m_Stream.readS32();
    uint16_t sseFlag = m_Stream.readU16();
    m_Stream.skip(2);
    int32_t sseOff = m_Stream.readS32();
    streamSound.unk = m_Stream.readU32();

    m_Stream.seek(start + titOff);
    uint32_t titEntryNum = m_Stream.readU32();
    std::vector<int32_t> tiOffsets{};
    for (int i = 0; i < titEntryNum; ++i) {
        ReferenceEntry entry = readReferenceEntry(m_Stream);
        if (entry.flag != 0x220e) {
            std::cerr << "Track Info Table reference flag invalid! " << entry.flag << std::endl;
            return std::nullopt;
        }
        tiOffsets.emplace_back(entry.offset);
    }
    for (int32_t tiOffset : tiOffsets) {
        m_Stream.seek(start + titOff + tiOffset);
        uint32_t f1 = m_Stream.readU8();
        uint32_t f2 = m_Stream.readU8();
        uint32_t f3 = m_Stream.readU8();
        uint32_t f4 = m_Stream.readU8();
        uint16_t tciFlag = m_Stream.readU16();
        m_Stream.skip(2);
        uint32_t tciOff = m_Stream.readS32();
        uint16_t svcFlag = m_Stream.readU16();
        m_Stream.skip(2);
        uint32_t svcOff = m_Stream.readS32();

        m_Stream.seek(start + titOff + tiOffset + tciOff);
        uint32_t ciEntryNum = m_Stream.readU32();
        uint32_t c0 = m_Stream.readU8();
        // c1 is 0 when channelCount == ciEntryNum == 1
        uint32_t c1 = m_Stream.readU8();
    }

    return streamSound;
}

BfsarWaveSound BfsarReader::readWaveSoundInfo() {
    BfsarWaveSound waveSound{};
    waveSound.archiveId = m_Stream.readU32();
    uint32_t unk1 = m_Stream.readU32();
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        BfsarPrioInfo prioInfo{};
        prioInfo.channelPrio = m_Stream.readU8();
        prioInfo.isReleasePrioFix = m_Stream.readU8();
        waveSound.prioInfo = prioInfo;
    }
    return waveSound;
}

BfsarSequenceSound BfsarReader::readSequenceSoundInfo() {
    BfsarSequenceSound sequenceSound{};
    uint32_t start = m_Stream.tell();
    uint32_t biFlag = m_Stream.readU16();
    m_Stream.skip(2);
    uint32_t bankIdOff = m_Stream.readU32();
    sequenceSound.validTracks = m_Stream.readU32();
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        sequenceSound.startOffset = m_Stream.readU32();
    }
    if (hasFlag(flags, 1)) {
        BfsarPrioInfo prioInfo{};
        prioInfo.channelPrio = m_Stream.readU8();
        prioInfo.isReleasePrioFix = m_Stream.readU8();
        m_Stream.skip(2);
        sequenceSound.prioInfo = prioInfo;
    }

    m_Stream.seek(start + bankIdOff);
    sequenceSound.bankIds = readBankIdTable();
    return sequenceSound;
}

BfsarSound3D BfsarReader::readSound3DInfo(uint32_t startOff) {
    int32_t sound3dOff = m_Stream.readS32();
    uint32_t mem = m_Stream.tell();
    m_Stream.seek(startOff + sound3dOff);
    BfsarSound3D sound3D{};
    sound3D.flags = m_Stream.readU32();
    sound3D.unkFloat = std::bit_cast<float>(m_Stream.readU32());
    sound3D.unkBool0 = m_Stream.readU8();
    sound3D.unkBool1 = m_Stream.readU8();

    m_Stream.seek(mem);
    return sound3D;
}

BfsarSoundGroup BfsarReader::readSoundGroupInfo(const std::vector<std::string> &stringTable,
                                                const std::vector<BfsarFileInfo> &fileData) {
    BfsarSoundGroup group{};
    uint32_t startOff = m_Stream.tell();
    group.startId = m_Stream.readU32();
    group.endId = m_Stream.readU32();
    uint16_t ftFlag = m_Stream.readU16();
    m_Stream.skip(2);
    uint32_t ftOff = m_Stream.readU32();
    uint16_t warTFlag = m_Stream.readU16();
    m_Stream.skip(2);
    uint32_t warTOff = m_Stream.readU32();
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        group.name = stringTable[m_Stream.readU32()];
    }
    return group;
}

BfsarBank
BfsarReader::readBankInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData) {
    BfsarBank bank{};
    uint32_t startOff = m_Stream.tell();
    bank.fileData = fileData[m_Stream.readU32()];
    uint32_t waFlag = m_Stream.readU16();
    m_Stream.skip(2);
    uint32_t waOff = m_Stream.readU32();
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        bank.name = stringTable[m_Stream.readU32()];
    }

    return bank;
}

BfsarWaveArchive BfsarReader::readWaveArchiveInfo(const std::vector<std::string> &stringTable,
                                                  const std::vector<BfsarFileInfo> &fileData) {
    BfsarWaveArchive waveArchive{};
    waveArchive.fileData = fileData[m_Stream.readU32()];
    // TODO check with bfwar reader
    uint32_t fileNum = m_Stream.readU32();

    uint32_t flags = m_Stream.readU32();

    if (hasFlag(flags, 0)) {
        waveArchive.name = stringTable[m_Stream.readU32()];
    }
    if (hasFlag(flags, 1)) {
        waveArchive.waveCount = m_Stream.readU32();
    }

    return waveArchive;
}

BfsarGroup
BfsarReader::readGroupInfo(const std::vector<std::string> &stringTable, const std::vector<BfsarFileInfo> &fileData) {
    BfsarGroup group{};
    uint32_t fileEntry = m_Stream.readU32();
    if (fileEntry != -1) {
        group.fileData = fileData[fileEntry];
    }
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        group.name = stringTable[m_Stream.readU32()];
    }
    return group;
}

BfsarPlayer BfsarReader::readPlayerInfo(const std::vector<std::string> &stringTable) {
    BfsarPlayer player{};
    player.playableSoundLimit = m_Stream.readU32();
    uint32_t flags = m_Stream.readU32();
    if (hasFlag(flags, 0)) {
        player.name = stringTable[m_Stream.readU32()];
    }
    if (hasFlag(flags, 1)) {
        player.playerHeapSize = m_Stream.readU32();
    }
    return player;
}

std::optional<BfsarFileInfo> BfsarReader::readFileInfo() {
    BfsarFileInfo fileInfo{};
    uint32_t start = m_Stream.tell();
    uint16_t locType = m_Stream.readU16();
    m_Stream.skip(2);
    // Always 0xc?
    int32_t offset = m_Stream.readS32();
    m_Stream.seek(start + offset);

    if (locType == 0x220d) {
        std::string str{};
        char chr = m_Stream.readS8();
        while (chr != 0) {
            str += chr;
            chr = m_Stream.readS8();
        }
        fileInfo.info = BfsarExternalFile{str};
    } else if (locType == 0x220c) {
        uint16_t flag = m_Stream.readU16();
        m_Stream.skip(2);
        int32_t fileOff = m_Stream.readS32();
        uint32_t fileSize = m_Stream.readU32();
        uint32_t gtFlag = m_Stream.readU16();
        m_Stream.skip(2);
        int32_t groupTableOff = m_Stream.readS32();

        fileInfo.info = BfsarInternalFile{fileOff, fileSize};
    } else {
        std::cerr << "Location type unknown! " << locType << std::endl;
        return std::nullopt;
    }
    return fileInfo;
}

BfsarSoundArchivePlayer BfsarReader::readSoundArchivePlayerInfo() {
    BfsarSoundArchivePlayer archivePlayer{};
    archivePlayer.sequenceLimit = m_Stream.readU16();
    archivePlayer.sequenceTrackLimit = m_Stream.readU16();
    archivePlayer.streamLimit = m_Stream.readU16();
    archivePlayer.unkLimit = m_Stream.readU16();
    archivePlayer.streamChannelLimit = m_Stream.readU16();
    archivePlayer.waveLimit = m_Stream.readU16();
    archivePlayer.unkWaveLimit = m_Stream.readU16();

    return archivePlayer;
}

std::vector<uint32_t> BfsarReader::readBankIdTable() {
    uint32_t bankNum = m_Stream.readU32();
    std::vector<uint32_t> bankIds{};
    bankIds.reserve(bankNum);
    for (int i = 0; i < bankNum; ++i) {
        bankIds.emplace_back(m_Stream.readU32());
    }
    return bankIds;
}

bool BfsarReader::hasFlag(uint32_t flags, uint8_t index) {
    return (flags >> index) & 1;
}
