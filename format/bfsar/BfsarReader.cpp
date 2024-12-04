//
// Created by cookieso on 27.11.24.
//

#include "BfsarReader.h"
#include "../../BfFile.h"

BfsarReader::BfsarReader(const MemoryResource &resource) : m_Stream(resource) {
    readHeader();
}

bool BfsarReader::readHeader() {
    BfsarHeader header{};
    header.magic = m_Stream.readU32();
    header.bom = m_Stream.readU16();
    if (header.bom != 0xFEFF) {
        if (header.bom != 0xFFFE) return false;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        header.magic = __builtin_bswap32(header.magic);
    }
    header.headerSize = m_Stream.readU16();
    header.version = m_Stream.readU32();
    if (header.version > 0x020400)
        std::cout << "Warning: BFSAR version might not be supported. (0x" << std::hex << header.version << std::dec
                  << ')'
                  << std::endl;
    header.fileSize = m_Stream.readU32();
    header.sectionNum = m_Stream.readU16();
    m_Stream.skip(2);
    readHeaderSections(header);
    return true;
}

bool BfsarReader::readHeaderSections(BfsarHeader &header) {
    std::optional<SectionInfo> strgSection, infoSection, fileSection;
    for (int i = 0; i < header.sectionNum; ++i) {
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
                return false;
        }
    }
    m_Stream.seek(strgSection->offset);
    readStrg();
    m_Stream.seek(infoSection->offset);
    auto res = readInfo();
    if (res.has_value()) {
        m_Stream.seek(fileSection->offset);
        readFile(res.value());
    }
    return true;
}

static std::vector<std::string> stringTable;

struct SummaryObject {
    std::string name;
    uint32_t itemId;
    int32_t soundInfoRefs = 0;
    int32_t soundGroupIndex = -1;
    int32_t bankIndex = -1;
    int32_t waveArcIndex = -1;
    int32_t groupIndex = -1;
    int32_t fileIndex = -1;
};

static std::array<uint32_t, 8> numOfType{};
static std::array<uint32_t, 8> numOfInfo{};

bool BfsarReader::readStrg() {
    stringTable.clear();
    numOfType.fill(0);
    numOfInfo.fill(0);
    BfsarStringTable strg{};
    strg.magic = m_Stream.readU32();
    strg.sectionSize = m_Stream.readU32();
    uint32_t strgOff = m_Stream.tell();
    if (m_Stream.readU16() != 0x2400) {
        std::cerr << "First entry in string section is not string table!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t strTblOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2401) {
        std::cerr << "Second entry in string section is not lookup table!" << std::endl;
        return false;
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
    for (auto entry: stringEntries) {
        m_Stream.seek(strgOff + strTblOff + entry.offset);
        std::string entryName;
        for (int i = 0; i < entry.size - 1; ++i) {
            entryName += m_Stream.readS8();
        }
        stringTable.emplace_back(entryName);
    }
    m_Stream.seek(strgOff + lutOff);
    readLut();
    return true;
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
        ++numOfType[type];
        std::cout << getTypeAsString(type);
        std::cout << ": " << stringTable[strTblIdx] << " (str index: " << std::hex << strTblIdx << ", id: " << itemId
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
        return true;
    }
    uint32_t lutStartOff = m_Stream.tell();
    m_Stream.skip(rootIndex * 0x5 * 0x4);
    printLutEntry(lutStartOff, "", false);
    return false;
}

std::optional<std::vector<int32_t>> BfsarReader::readInfo() {
    BfsarInfo info{};
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

    std::cout << std::hex;

    std::vector<uint32_t> infoRefs;

    std::cout << "Entries: " << std::hex << stringTable.size() << std::endl;

    //std::cout << "---Sound Info---" << std::endl;
    m_Stream.seek(soundOff + infoOff);
    infoRefs = readInfoRef(0x2200);
    numOfInfo[1] = infoRefs.size();
    for (auto entry: infoRefs) {
        m_Stream.seek(soundOff + infoOff + entry);
        SoundInfo soundInfo = readSoundInfo();
    }

    //std::cout << "---Sound Group Info---" << std::endl;
    m_Stream.seek(soundGroupOff + infoOff);
    infoRefs = readInfoRef(0x2204);
    numOfInfo[2] = infoRefs.size();
    for (int i = 0; i < infoRefs.size(); ++i) {
        m_Stream.seek(soundGroupOff + infoOff + infoRefs[i]);
        SoundGroupInfo soundGroupInfo = readSoundGroupInfo();
        if (soundGroupInfo.startId == 0xffffffff) continue;
        for (uint32_t j = soundGroupInfo.startId; j <= soundGroupInfo.endId; ++j) {
        }
    }

    //std::cout << "---Bank Info---" << std::endl;
    m_Stream.seek(bankOff + infoOff);
    infoRefs = readInfoRef(0x2206);
    numOfInfo[3] = infoRefs.size();
    for (int i = 0; i < infoRefs.size(); ++i) {
        m_Stream.seek(bankOff + infoOff + infoRefs[i]);
        BankInfo bankInfo = readBankInfo();
    }

    //std::cout << "---Wave Archive Info---" << std::endl;
    m_Stream.seek(waveArcOff + infoOff);
    infoRefs = readInfoRef(0x2207);
    numOfInfo[5] = infoRefs.size();
    for (int i = 0; i < infoRefs.size(); ++i) {
        m_Stream.seek(waveArcOff + infoOff + infoRefs[i]);
        WaveArchiveInfo waveArchiveInfo = readWaveArchiveInfo();
    }

    //std::cout << "---Group Info---" << std::endl;
    m_Stream.seek(groupInfoOff + infoOff);
    infoRefs = readInfoRef(0x2208);
    numOfInfo[6] = infoRefs.size();
    for (auto entry: infoRefs) {
        m_Stream.seek(groupInfoOff + infoOff + entry);
        readGroupInfo();
    }

    //std::cout << "---Player Info---" << std::endl;
    m_Stream.seek(playerInfoOff + infoOff);
    infoRefs = readInfoRef(0x2209);
    numOfInfo[4] = infoRefs.size();
    for (auto entry: infoRefs) {
        m_Stream.seek(playerInfoOff + infoOff + entry);
        readPlayerInfo();
    }

    //std::cout << "---File Info---" << std::endl;
    m_Stream.seek(fileInfoOff + infoOff);
    std::vector<int32_t> fileOffsets{};
    infoRefs = readInfoRef(0x220a);
    std::cout << "File Info Count: " << infoRefs.size() << std::endl;
    for (unsigned int infoRef : infoRefs) {
        m_Stream.seek(fileInfoOff + infoOff + infoRef);
        int32_t result = readFileInfo();
        if (result != -1) {
            fileOffsets.emplace_back(result);
        }
    }

    //std::cout << "---Sound Archive Player Info---" << std::endl;
    m_Stream.seek(soundArcPlayerInfoOff + infoOff);
    readSoundArchivePlayerInfo();

    for (int i = 1; i < numOfType.size()-1; ++i) {
        if (i != 5 && numOfInfo[i] != numOfType[i]) {
            abort();
        }
        std::cout << getTypeAsString(i) << ": " << numOfType[i] << " Info: " << numOfInfo[i] << std::endl;
    }

    return fileOffsets;
}

bool BfsarReader::readFile(const std::vector<int32_t> &fileOffsets) {
    uint32_t magic = m_Stream.readU32();
    uint32_t size = m_Stream.readU32();
    uint32_t start = m_Stream.tell();
    for (auto off: fileOffsets) {
        m_Stream.seek(start + off);
        std::string str{};
        for (int i = 0; i < 4; ++i) {
            str += m_Stream.readS8();
        }
        //std::cout << str << std::endl;
    }
    return true;
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

SoundInfo BfsarReader::readSoundInfo() {
    SoundInfo soundInfo{};
    soundInfo.fileId = m_Stream.readU32();
    soundInfo.playerId = m_Stream.readU32();
    soundInfo.volume = m_Stream.readU8();
    soundInfo.remoteFilter = m_Stream.readU8();
    m_Stream.skip(2);
    soundInfo.soundType = m_Stream.readU16();

    if (true) {
        std::cout << std::hex << "File Id: " << soundInfo.fileId;
        std::cout << " Player Id: " << soundInfo.playerId;
        std::cout << " Volume: " << static_cast<uint32_t>(soundInfo.volume);
        std::cout << " Remote Filter: " << static_cast<uint32_t>(soundInfo.remoteFilter);
        std::cout << " Sound type: " << soundInfo.soundType;
        m_Stream.skip(2);
        std::cout << " Info Offset: " << m_Stream.readU32();
        std::cout << " Flags: " << m_Stream.readU32() << std::endl;
    }

    return soundInfo;
}

SoundGroupInfo BfsarReader::readSoundGroupInfo() {
    SoundGroupInfo groupInfo{};
    groupInfo.startId = m_Stream.readU32();
    groupInfo.endId = m_Stream.readU32();
    if (true) {
        std::cout << std::hex << "Start Id: " << groupInfo.startId;
        std::cout << " End Id: " << groupInfo.endId;
        std::cout << " Unk: " << m_Stream.readU32();
        std::cout << " Unk Off: " << m_Stream.readU32();
        std::cout << " Flag: " << m_Stream.readU16();
        m_Stream.skip(2);
        std::cout << " Off: " << m_Stream.readS32() << std::endl;
    }
    return groupInfo;
}

BankInfo BfsarReader::readBankInfo() {
    BankInfo bankInfo{};
    bankInfo.fileId = m_Stream.readU32();
    if (false) {
        std::cout << "File Id: " << bankInfo.fileId;
        std::cout << " Unk: " << m_Stream.readU32();
        std::cout << " WaveArcIdTableOff: " << m_Stream.readS32() << std::endl;
    }

    return bankInfo;
}

WaveArchiveInfo BfsarReader::readWaveArchiveInfo() {
    WaveArchiveInfo waveArchiveInfo{};
    waveArchiveInfo.fileId = m_Stream.readU32();
    waveArchiveInfo.fileNum = m_Stream.readU32();
    if (true) {
        std::cout << "File Id: " << waveArchiveInfo.fileId;
        std::cout << " File Num: " << waveArchiveInfo.fileNum;
        std::cout << " Flags: " << m_Stream.readU32() << std::endl;
    }

    return waveArchiveInfo;
}

bool BfsarReader::readGroupInfo() {
    if (true) {
        // If -1 then external
        std::cout << "File Info Entry: " << m_Stream.readU32() << std::endl;
    }
    return false;
}

bool BfsarReader::readPlayerInfo() {
    if (false) {
        std::cout << "Playable Sound Num: " << m_Stream.readU32();
        std::cout << " Flags: " << m_Stream.readU32() << std::endl;
    }
    return false;
}

int32_t BfsarReader::readFileInfo() {
    FileInfo fileInfo{};
    uint32_t start = m_Stream.tell();
    fileInfo.locType = m_Stream.readU16();
    m_Stream.skip(2);
    // Always 0xc?
    fileInfo.offset = m_Stream.readS32();
    m_Stream.seek(start + fileInfo.offset);
    if (false) {
        std::cout << "Location Type: " << fileInfo.locType;
        std::cout << " Offset: " << fileInfo.offset;
    }

    if (fileInfo.locType == 0x220d) {
        std::string str{};
        char chr = m_Stream.readS8();
        while (chr != 0) {
            str += chr;
            chr = m_Stream.readS8();
        }
        if (false) {
            std::cout << "--External File Info--" << std::endl;
            std::cout << str << std::endl;
        }
    } else if (fileInfo.locType == 0x220c) {
        uint16_t flag = m_Stream.readU16();
        m_Stream.skip(2);
        int32_t fileOff = m_Stream.readS32();
        if (fileOff == -1) {
            // Internal but inside a fgrp (the fgrp might be external though)
        }
        uint32_t unk1 = m_Stream.readU32();
        uint32_t unk2 = m_Stream.readU32();
        int32_t groupTableOff = m_Stream.readS32();

        if (false) {
            std::cout << "--Internal File Info--" << std::endl;
            std::cout << " Flag: " << flag;
            std::cout << " File Offset: " << fileOff;
            std::cout << " Unk: " << unk1;
            std::cout << " Unk: " << unk2;
            std::cout << " Group Table Offset: " << groupTableOff << std::endl;
        }
        return fileOff;
    } else {
        std::cerr << "Location type unknown! " << fileInfo.locType << std::endl;
    }
    return -1;
}

bool BfsarReader::readSoundArchivePlayerInfo() {
    if (false) {
        std::cout << "Sequence Num: " << m_Stream.readU16();
        std::cout << " Sequence Track Num: " << m_Stream.readU16();
        std::cout << " Stream Num: " << m_Stream.readU16();
        std::cout << " Unk: " << m_Stream.readU16();
        std::cout << " Unk: " << m_Stream.readU16();
        std::cout << " Wave Num: " << m_Stream.readU16();
        std::cout << " Unk: " << m_Stream.readU16();
        std::cout << " Stream Buffer Times: " << static_cast<uint32_t>(m_Stream.readU8());
        std::cout << " Is Advanced Wave: " << static_cast<uint32_t>(m_Stream.readU8()) << std::endl;
    }

    return false;
}
