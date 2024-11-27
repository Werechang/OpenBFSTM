//
// Created by cookieso on 27.11.24.
//

#include "BfsarReader.h"
#include "../BfFile.h"

BfsarReader::BfsarReader(const MemoryResource &resource) : m_Stream(resource) {

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
        std::cout << "Warning: BFSAR version might not be supported. (0x" << std::hex << header.version << std::dec << ')'
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
    m_Stream.seek(strgOff + lutOff);
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

bool BfsarReader::readLut() {
    uint32_t unkIndex = m_Stream.readU32();
    uint32_t lutCount = m_Stream.readU32();
    for (int i = 0; i < lutCount; ++i) {
        uint16_t endpoint = m_Stream.readU16();
        uint16_t b = m_Stream.readU16();
        uint32_t c = m_Stream.readU32();
        uint32_t d = m_Stream.readU32();
        uint32_t strTblIdx = m_Stream.readU32();
        uint8_t fileType = m_Stream.readU8();
        uint32_t fileId = m_Stream.readU32() & 0xffffff;
        m_Stream.rewind(1);
    }
    return false;
}
