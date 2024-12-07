//
// Created by cookieso on 06.12.24.
//

#include "BfgrpWriter.h"

BfgrpWriter::BfgrpWriter(MemoryResource &resource, const BfgrpWriteContext &context) : m_Stream(resource) {
    writeHeader(context);
}

void BfgrpWriter::writeHeader(const BfgrpWriteContext &context) {
    m_Stream.writeU32(0x46475250);
    m_Stream.writeU16(0xfeff);
    m_Stream.writeU16(0x40);
    m_Stream.writeU32(0x010000);
    uint32_t sizeOff = m_Stream.skip(4);
    m_Stream.writeU16(0x3);
    m_Stream.writeNull(2);

    m_Stream.writeU16(0x7800);
    m_Stream.writeNull(2);
    m_Stream.writeS32(0x40);
    uint32_t infoSizeOff = m_Stream.skip(4);

    m_Stream.writeU16(0x7801);
    m_Stream.writeNull(2);
    uint32_t fileOffsetOff = m_Stream.skip(8);

    m_Stream.writeU16(0x7802);
    m_Stream.writeNull(2);
    uint32_t infxOffsetOff = m_Stream.skip(8);

    m_Stream.seek(0x40);
    uint32_t infoSize = writeInfo(context);
    m_Stream.seek(0x40 + infoSize);
    uint32_t fileOffset = m_Stream.tell();
    uint32_t fileSize = writeFile(context);
    m_Stream.seek(fileOffset + fileSize);
    uint32_t infxOffset = m_Stream.tell();
    uint32_t infxSize = writeInfx(context);

    uint32_t size = m_Stream.tell();

    m_Stream.seek(sizeOff);
    m_Stream.writeU32(size);
    m_Stream.seek(infoSizeOff);
    m_Stream.writeU32(infoSize);
    m_Stream.seek(fileOffsetOff);
    m_Stream.writeU32(fileOffset);
    m_Stream.writeU32(fileSize);
    m_Stream.seek(infxOffsetOff);
    m_Stream.writeU32(infxOffset);
    m_Stream.writeU32(infxSize);
}

uint32_t BfgrpWriter::writeInfo(const BfgrpWriteContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x4f464e49);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.writeU32(context.files.size());
    for (int i = 0; i < context.files.size(); ++i) {
        m_Stream.writeU16(0x7900);
        m_Stream.writeU16(0);
        m_Stream.writeS32(0x8 * context.files.size() + 0x4 + i * 0x10);
    }
    uint32_t nextFileOff = 0x38;
    for (auto &file : context.files) {
        m_Stream.writeU32(file.fileIndex);
        m_Stream.writeU16(0x1f00);
        m_Stream.writeU16(0);
        m_Stream.writeS32(nextFileOff);
        m_Stream.writeU32(file.file.size());
        uint32_t alignment = file.file.size() % 0x20;
        if (alignment != 0) {
            alignment = 0x20 - alignment;
        }
        nextFileOff += file.file.size() + alignment;
    }
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}

uint32_t BfgrpWriter::writeFile(const BfgrpWriteContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x454c4946);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    for (auto &file : context.files) {
        m_Stream.writeBuffer(file.file);
        m_Stream.fillToAlign(0x20);
    }
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}

uint32_t BfgrpWriter::writeInfx(const BfgrpWriteContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x58464e49);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.writeU32(context.dependencies.size());

    for (int i = 0; i < context.dependencies.size(); ++i) {
        m_Stream.writeU16(0x7901);
        m_Stream.writeU16(0);
        m_Stream.writeS32(0x4 + context.dependencies.size() * 0x8 + i * 0x8);
    }

    for (auto &dep : context.dependencies) {
        m_Stream.writeU32(dep.id);
        m_Stream.writeU32(dep.loadFlags);
    }

    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return 0;
}
