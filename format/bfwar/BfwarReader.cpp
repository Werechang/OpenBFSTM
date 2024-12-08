//
// Created by cookieso on 08.12.24.
//

#include "BfwarReader.h"
#include "../../BfFile.h"

BfwarReader::BfwarReader(const MemoryResource &resource) : m_Stream(resource) {
    m_Context = readHeader();
}

std::optional<BfwarReadContext> BfwarReader::readHeader() {
    uint32_t magic = m_Stream.readU32();
    uint16_t bom = m_Stream.readU16();
    if (bom != 0xFEFF) {
        if (bom != 0xFFFE) return std::nullopt;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        magic = __builtin_bswap32(magic);
    }
    if (magic != 0x46574152) {
        std::cerr << "FWAR file magic does not match!" << std::endl;
        return std::nullopt;
    }
    uint16_t headerSize = m_Stream.readU16();
    uint32_t version = m_Stream.readU32();
    if (version != 0x10000)
        std::cout << "Warning: FWAR version might not be supported. (0x" << std::hex << version << std::dec << ')'
                  << std::endl;
    uint32_t fileSize = m_Stream.readU32();
    return readHeaderSections();
}

std::optional<BfwarReadContext> BfwarReader::readHeaderSections() {
    uint32_t sectionNum = m_Stream.readU16();
    m_Stream.skip(2);
    std::optional<SectionInfo> infoSection, fileSection;
    for (int i = 0; i < sectionNum; ++i) {
        switch (auto section = readSectionInfo(m_Stream); section.flag) {
            case 0x6800:
                infoSection = section;
                break;
            case 0x6801:
                fileSection = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << " in FWAR" << std::endl;
                return std::nullopt;
        }
    }
    m_Stream.seek(infoSection->offset);
    auto fileEntries = readInfo();

    if (!fileEntries) return std::nullopt;
    m_Stream.seek(fileSection->offset);
    m_FileOffset = readFile();
    if (m_FileOffset == 0) return std::nullopt;
    return BfwarReadContext{fileEntries.value()};
}

std::optional<std::vector<BfwarFile>> BfwarReader::readInfo() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x4f464e49) {
        std::cerr << "INFO magic in FGRP file does not match!" << std::endl;
        return std::nullopt;
    }
    uint32_t size = m_Stream.readU32();
    uint32_t infoStart = m_Stream.tell();
    uint32_t refCount = m_Stream.readU32();
    std::vector<BfwarFile> offsets{};
    for (int i = 0; i < refCount; ++i) {
        auto ref = readReferenceEntry(m_Stream);
        if (ref.flag != 0x1f00) {
            std::cerr << "Unsupported reference with flag " << ref.flag << std::endl;
            return std::nullopt;
        }
        offsets.emplace_back(BfwarFile{ref.offset, m_Stream.readU32()});
    }
    return offsets;
}

uint32_t BfwarReader::readFile() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x454c4946) {
        std::cerr << "FILE magic in FGRP file does not match!" << std::endl;
        return 0;
    }

    uint32_t size = m_Stream.readU32();
    return m_Stream.tell();
}

std::span<const uint8_t> BfwarReader::getFileData(uint32_t offset, uint32_t size) {
    if (!m_Context) return {};
    return m_Stream.getSpanAt(m_FileOffset + offset, size);
}
