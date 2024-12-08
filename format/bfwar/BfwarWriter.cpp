//
// Created by cookieso on 08.12.24.
//

#include "BfwarWriter.h"

BfwarWriter::BfwarWriter(MemoryResource &resource, const BfwarWriteContext &context) : m_Stream(resource) {
    writeHeader(context);
}

void BfwarWriter::writeHeader(const BfwarWriteContext &context) {
    m_Stream.writeU32(0x52415746);
    m_Stream.writeU16(0xfeff);
    m_Stream.writeU16(0x40);
    m_Stream.writeU32(0x010000);
    uint32_t sizeOff = m_Stream.skip(4);
    m_Stream.writeU16(0x2);
    m_Stream.writeNull(2);

    m_Stream.writeU16(0x6800);
    m_Stream.writeNull(2);
    m_Stream.writeS32(0x40);
    uint32_t infoSizeOff = m_Stream.skip(4);

    m_Stream.writeU16(0x6801);
    m_Stream.writeNull(2);
    uint32_t fileOffsetOff = m_Stream.skip(8);

    m_Stream.seek(0x40);
    uint32_t infoSize = writeInfo(context);
    m_Stream.seek(0x40 + infoSize);
    uint32_t fileOffset = m_Stream.tell();
    uint32_t fileSize = writeFile(context);
    m_Stream.seek(fileOffset + fileSize);

    uint32_t size = m_Stream.tell();

    m_Stream.seek(sizeOff);
    m_Stream.writeU32(size);
    m_Stream.seek(infoSizeOff);
    m_Stream.writeU32(infoSize);
    m_Stream.seek(fileOffsetOff);
    m_Stream.writeU32(fileOffset);
    m_Stream.writeU32(fileSize);
}

uint32_t BfwarWriter::writeInfo(const BfwarWriteContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x4f464e49);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.writeU32(context.files.size());

    uint32_t nextFileOff = 0x38;
    for (const std::span<const uint8_t> &file: context.files) {
        m_Stream.writeU16(0x1f00);
        m_Stream.writeU16(0);
        m_Stream.writeS32(nextFileOff);
        m_Stream.writeU32(file.size());
        uint32_t alignment = file.size() % 0x20;
        if (alignment != 0) {
            alignment = 0x20 - alignment;
        }
        nextFileOff += file.size() + alignment;
    }
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}

uint32_t BfwarWriter::writeFile(const BfwarWriteContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x454c4946);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.fillToAlign(0x40);
    for (const std::span<const uint8_t> &file: context.files) {
        m_Stream.writeBuffer(file);
        m_Stream.fillToAlign(0x20);
    }
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}
