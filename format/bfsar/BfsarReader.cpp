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
    return true;
}

bool BfsarReader::readStrg() {
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
        //std::cout << entryName << std::endl;
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

static int lutLeafCounter = 0;

void BfsarReader::printLutEntry(uint32_t baseOff, const std::string &prefix, bool isLeft) {
    uint16_t isLeaf = m_Stream.readU16();
    uint16_t compareFunc = m_Stream.readU16();
    uint32_t leftChild = m_Stream.readU32();
    uint32_t rightChild = m_Stream.readU32();
    uint32_t strTblIdx = m_Stream.readU32();
    uint32_t itemId = m_Stream.readU32();
    std::cout << prefix << (isLeft ? "├──" : "└──" );
    if (strTblIdx != 0xffffffffu) {
        std::cout << std::hex << strTblIdx << std::endl;
    } else {
        std::cout << std::dec << (compareFunc >> 3) << "*" << (compareFunc & 7) << std::endl;
    }
    if (!isLeaf) {
        m_Stream.seek(baseOff + leftChild * 0x5 * 0x4);
        printLutEntry(baseOff, prefix + (isLeft ? "│   " : "    "), true);
        m_Stream.seek(baseOff + rightChild * 0x5 * 0x4);
        printLutEntry(baseOff, prefix + (isLeft ? "│   " : "    "), false);
    } else if (itemId == 0xffffffffu) {
        std::cerr << "Leaf node has item id!" << std::endl;
    } else {
        ++lutLeafCounter;
    }
}

bool BfsarReader::readLut() {
    uint32_t rootIndex = m_Stream.readU32();
    uint32_t entryCount = m_Stream.readU32();
    uint32_t lutStartOff = m_Stream.tell();
    m_Stream.skip(rootIndex * 0x5 * 0x4);
    printLutEntry(lutStartOff, "", false);
    return false;
}

bool BfsarReader::readInfo() {
    BfsarInfo info{};
    uint32_t magic = m_Stream.readU32();
    uint32_t size = m_Stream.readU32();
    if (m_Stream.readU16() != 0x2100) {
        std::cerr << "First entry in info section is not sound!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t soundOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2104) {
        std::cerr << "Second entry in info section is not sound group!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t soundGroupOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2101) {
        std::cerr << "Third entry in info section is not bank!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t bankOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2103) {
        std::cerr << "Fourth entry in info section is not wave archive!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t waveArcOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2105) {
        std::cerr << "Fifth entry in info section is not group info!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t groupInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2102) {
        std::cerr << "Sixth entry in info section is not player info!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t playerInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x2106) {
        std::cerr << "Seventh entry in info section is not file info!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t fileInfoOff = m_Stream.readS32();
    if (m_Stream.readU16() != 0x220B) {
        std::cerr << "Eight entry in info section is not sound archive player info!" << std::endl;
        return false;
    }
    m_Stream.skip(2);
    int32_t soundArcPlayerInfoOff = m_Stream.readS32();
    return false;
}
