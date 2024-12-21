//
// Created by cookieso on 20.12.24.
//
#include "MemoryResource.h"

InMemoryStream::InMemoryStream(const MemoryResource &resource) : m_Resource(resource) {
#ifdef COVERAGE_CHECK
    m_Coverage.resize(resource.m_Data.size(), false);
#endif
}

void InMemoryStream::evaluateCoverage() {
#ifdef COVERAGE_CHECK
    uint32_t start = -1;
        uint32_t size = 0;
        for (auto i = 0; i < m_Coverage.size(); ++i) {
            if (!m_Coverage[i]) {
                if (start != -1) {
                    ++size;
                } else if (m_Resource.m_Data[i] != 0) { // data shall not be 0 at start because it indicates padding
                    start = i;
                    ++size;
                }
            } else if (start != -1) {
                std::cout << "No coverage at " << std::hex << start << '-' << start + size << std::endl;
                start = -1;
                size = 0;
            }
        }
        if (start != -1) {
            std::cout << "No coverage at " << std::hex << start << '-' << start + size << std::endl;
        }
#endif
}

void InMemoryStream::addCoverageRegion(uint32_t start, uint32_t size) {
#ifdef COVERAGE_CHECK
    for (auto i = start; i < start + size; ++i) {
            if (m_Coverage[i]) {
                std::cout << std::hex << "Duplicate coverage at " << start << '-' << start + size << std::endl;
            }
            m_Coverage[i] = true;
        }
#endif
}

void InMemoryStream::skip(const size_t off) {
    if (m_Pos + off > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob skip");
#ifdef COVERAGE_CHECK
    addCoverageRegion(m_Pos, off);
#endif
    m_Pos += off;
}

std::span<const uint8_t> InMemoryStream::getSpanAt(uint32_t offset, uint32_t size) {
#ifdef COVERAGE_CHECK
    addCoverageRegion(offset, size);
#endif
    return {m_Resource.m_Data.begin() + offset, size};
}

uint8_t InMemoryStream::readU8() {
    return readNum<uint8_t>();
}

int8_t InMemoryStream::readS8() {
    return std::bit_cast<int8_t>(readU8());
}

uint16_t InMemoryStream::readU16() {
    return swapBO ? __builtin_bswap16(readNum<uint16_t>()) : readNum<uint16_t>();
}

int16_t InMemoryStream::readS16() {
    return std::bit_cast<int16_t>(readU16());
}

uint32_t InMemoryStream::readU32() {
    return swapBO ? __builtin_bswap32(readNum<uint32_t>()) : readNum<uint32_t>();
}

int32_t InMemoryStream::readS32() {
    return std::bit_cast<int32_t>(readU32());
}

void InMemoryStream::rewind(const size_t off) {
    if (off > m_Pos) throw std::out_of_range("Resource oob rewind");
    m_Pos -= off;
}

void InMemoryStream::seek(const size_t off) {
    if (off > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob seek");
    m_Pos = off;
}

template<std::integral Num>
Num InMemoryStream::readNum()  {
    if (m_Pos + sizeof(Num) > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob read");
    Num res = *reinterpret_cast<const Num *>(m_Resource.getAsPtrUnsafe(m_Pos));
#ifdef COVERAGE_CHECK
    addCoverageRegion(m_Pos, sizeof(Num));
#endif
    m_Pos += sizeof(Num);
    return res;
}
