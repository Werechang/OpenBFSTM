//
// Created by cookieso on 01.07.24.
//

#include <optional>
#include <iostream>
#include "BfgrpReader.h"

BfgrpReader::BfgrpReader(const MemoryResource &resource) : m_Stream(resource) {
    m_Context = readHeader();
}

std::optional<BfgrpReadContext> BfgrpReader::readHeader() {
    uint32_t magic = m_Stream.readU32();
    uint16_t bom = m_Stream.readU16();
    if (bom != 0xFEFF) {
        if (bom != 0xFFFE) return std::nullopt;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        magic = __builtin_bswap32(magic);
    }
    if (magic != 0x46475250) {
        std::cerr << "FGRP file magic does not match!" << std::endl;
        return std::nullopt;
    }
    uint16_t headerSize = m_Stream.readU16();
    uint32_t version = m_Stream.readU32();
    if (version != 0x10000)
        std::cout << "Warning: FGRP version might not be supported. (0x" << std::hex << version << std::dec << ')'
                  << std::endl;
    uint32_t fileSize = m_Stream.readU32();
    return readHeaderSections();
}

std::optional<BfgrpReadContext> BfgrpReader::readHeaderSections() {
    uint32_t sectionNum = m_Stream.readU16();
    m_Stream.skip(2);
    std::optional<SectionInfo> infoSection, fileSection, infoExSection;
    for (int i = 0; i < sectionNum; ++i) {
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
                std::cerr << "Unsupported section with flag " << section.flag << " in FGRP" << std::endl;
                return std::nullopt;
        }
    }
    m_Stream.seek(infoSection->offset);
    auto fileEntries = readInfo();
    m_Stream.seek(infoExSection->offset);
    auto depInfo = readGroupItemExtraInfo();

    if (!fileEntries || !depInfo) return std::nullopt;
    m_Stream.seek(fileSection->offset);
    m_FileOffset = readFile(fileEntries.value());
    if (m_FileOffset == 0) return std::nullopt;
    return BfgrpReadContext{fileEntries.value(), depInfo.value()};
}

std::optional<std::vector<BfgrpFileEntry>> BfgrpReader::readInfo() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x4f464e49) {
        std::cerr << "INFO magic in FGRP file does not match!" << std::endl;
        return std::nullopt;
    }
    uint32_t size = m_Stream.readU32();
    uint32_t infoStart = m_Stream.tell();
    uint32_t refCount = m_Stream.readU32();
    std::vector<int32_t> offsets{};
    for (int i = 0; i < refCount; ++i) {
        auto ref = readReferenceEntry(m_Stream);
        if (ref.flag != 0x7900) {
            std::cerr << "Unsupported reference with flag " << ref.flag << std::endl;
            return std::nullopt;
        }
        offsets.emplace_back(ref.offset);
    }
    std::vector<BfgrpFileEntry> entries{};
    for (auto offset: offsets) {
        m_Stream.seek(infoStart + offset);
        auto res = readFileLocationInfo();
        if (!res) return std::nullopt;
        entries.emplace_back(res.value());
    }
    return entries;
}

std::optional<BfgrpFileEntry> BfgrpReader::readFileLocationInfo() {
    BfgrpFileEntry entry{};
    entry.fileIndex = m_Stream.readU32();
    uint16_t type = m_Stream.readU16();
    if (type != 0x1f00) {
        std::cerr << "Unsupported file location reference with flag " << type << " in FGRP" << std::endl;
        return std::nullopt;
    }
    m_Stream.skip(2);
    entry.offset = m_Stream.readS32();
    entry.size = m_Stream.readU32();
    return entry;
}

std::optional<std::vector<BfgrpDepEntry>> BfgrpReader::readGroupItemExtraInfo() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x58464e49) {
        std::cerr << "INFX magic in FGRP file does not match!" << std::endl;
        return std::nullopt;
    }
    uint32_t size = m_Stream.readU32();

    uint32_t infoExStart = m_Stream.tell();
    uint32_t depSize = m_Stream.readU32();
    std::vector<int32_t> offsets{};
    for (int i = 0; i < depSize; ++i) {
        auto ref = readReferenceEntry(m_Stream);
        if (ref.flag != 0x7901) {
            std::cerr << "Unsupported reference with flag " << ref.flag << std::endl;
            return std::nullopt;
        }
        offsets.emplace_back(ref.offset);
    }
    std::vector<BfgrpDepEntry> depInfo{};
    for (int32_t offset: offsets) {
        m_Stream.seek(infoExStart + offset);
        depInfo.emplace_back(BfgrpDepEntry{m_Stream.readU32(), m_Stream.readU32()});
    }
    return depInfo;
}

uint32_t BfgrpReader::readFile(const std::vector<BfgrpFileEntry> &entries) {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x454c4946) {
        std::cerr << "FILE magic in FGRP file does not match!" << std::endl;
        return 0;
    }

    uint32_t size = m_Stream.readU32();
    uint32_t start = m_Stream.tell();
    for (const auto &item: entries) {
        m_Stream.seek(start + item.offset);
        std::string str{};
        for (int i = 0; i < 4; ++i) {
            str += m_Stream.readS8();
        }
    }
    return start;
}

std::span<const uint8_t> BfgrpReader::getFileData(uint32_t offset, uint32_t size) {
    if (!m_Context) return {};
    m_Stream.seek(offset + m_FileOffset);
    return m_Stream.getSpanAt(offset, size);
}
