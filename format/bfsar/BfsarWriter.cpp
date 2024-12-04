//
// Created by cookieso on 03.12.24.
//

#include "BfsarWriter.h"

BfsarWriter::BfsarWriter(MemoryResource &resource) : m_Stream(resource) {

}

void BfsarWriter::writeHeader() {
    m_Stream.writeU32(0x52415346);
    m_Stream.writeU16(0xfeff);
    m_Stream.writeU16(0x40);
    m_Stream.writeU32(0x20400);
    uint32_t sizeOff = m_Stream.skip(4);
    m_Stream.writeU16(0x3);
    m_Stream.writeNull(2);
    m_Stream.writeU16(0x2000);
    m_Stream.writeNull(2);
    m_Stream.writeS32(0x40);
    uint32_t strgSize = m_Stream.skip(4);
    m_Stream.writeU16(0x2001);
    m_Stream.writeNull(2);
    uint32_t infoData = m_Stream.skip(8);
    m_Stream.writeU16(0x2002);
    m_Stream.writeNull(2);
    uint32_t fileData = m_Stream.skip(8);

    m_Stream.seek(0x40);
    writeStrg();
}

void BfsarWriter::writeStrg() {

}
