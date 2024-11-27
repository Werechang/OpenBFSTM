//
// Created by cookieso on 01.07.24.
//

#include <optional>
#include <iostream>
#include "BfgrpReader.h"

BfgrpReader::BfgrpReader(const MemoryResource& resource) : m_Stream(resource) {
}

void BfgrpReader::readGroupItemLocInfo(int index) {

}

bool BfgrpReader::readHeader() {
    BfgrpHeader &header = m_Context.header;
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
    if (header.version != 0x10000)
        std::cout << "Warning: BFSTM version might not be supported. (0x" << std::hex << header.version << std::dec << ')'
                  << std::endl;
    header.fileSize = m_Stream.readU32();
    header.sectionNum = m_Stream.readU16();
    m_Stream.skip(2);
    readHeaderSections(header);
    return false;
}

bool BfgrpReader::readHeaderSections(BfgrpHeader& header) {
    std::optional<SectionInfo> infoSection, fileSection, infoExSection;
    for (int i = 0; i < header.sectionNum; ++i) {
        switch (auto section = readSectionInfo(m_Stream); section.flag) {
            case 0x7800:
                infoSection = section;
                break;
            case 0x7801:
                fileSection = section;
                break;
            case 0x7802:
                infoExSection = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << std::endl;
                return false;
        }
    }
    m_Stream.seek(infoSection->offset);
    readInfo();
    return true;
}

bool BfgrpReader::readInfo() {
    BfgrpInfo info{};
    info.magic = m_Stream.readU32();
    info.size = m_Stream.readU32();
    uint32_t infoStart = m_Stream.tell();
    uint32_t refCount = m_Stream.readU32();
    std::vector<int32_t> offsets{};
    for (int i = 0; i < refCount; ++i) {
        auto ref = readReferenceEntry(m_Stream);
        if (ref.flag != 0x7900) {
            std::cerr << "Unsupported reference with flag " << ref.flag << std::endl;
            return false;
        }
        offsets.emplace_back(ref.offset);
    }
    for (auto offset : offsets) {
        m_Stream.seek(infoStart + offset);
        readFileLocationInfo();
    }
    return true;
}

void BfgrpReader::readFileLocationInfo() {
    BfgrpFileEntry entry{};
    entry.id = m_Stream.readU32();
    entry.type = m_Stream.readU16();
    entry.offset = m_Stream.readS32();
}
