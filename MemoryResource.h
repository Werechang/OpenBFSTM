//
// Created by cookieso on 6/24/24.
//
#pragma once

#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>
#include <vector>
#include <fstream>

class InMemoryStream;

class OutMemoryStream;

class MemoryResource {
public:
    explicit MemoryResource(std::istream &input) {
        input.seekg(0, std::ios::end);
        const std::streamsize size = input.tellg();
        if (size > 0xffffffff) {
            std::cerr << "File too large! " << size << std::endl;
            exit(1);
        }
        input.seekg(0);
        m_Data.resize(size);
        input.read(reinterpret_cast<std::istream::char_type *>(m_Data.data()), size);
    }

    MemoryResource() = default;

    friend InMemoryStream;
    friend OutMemoryStream;

    [[nodiscard]] void *getAsPtrUnsafe(const size_t offset) {
        return m_Data.data() + offset;
    }

    [[nodiscard]] const void *getAsPtrUnsafe(const size_t offset) const {
        return m_Data.data() + offset;
    }

    void writeToFile(const char *file) {
        std::ofstream out{file, std::ios::binary};
        out.write(reinterpret_cast<const char *>(m_Data.data()), m_Data.size());
        out.close();
    }

private:
    std::vector<uint8_t> m_Data;
};

class InMemoryStream {
public:
    explicit InMemoryStream(const MemoryResource &resource) : m_Resource(resource) {
    }

    template<std::integral Num>
    Num readNum() {
        if (m_Pos + sizeof(Num) > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob read");
        Num res = *reinterpret_cast<const Num *>(m_Resource.getAsPtrUnsafe(m_Pos));
        m_Pos += sizeof(Num);
        return res;
    }

    uint8_t readU8() {
        return readNum<uint8_t>();
    }

    int8_t readS8() {
        return std::bit_cast<int8_t>(readU8());
    }

    uint16_t readU16() {
        return swapBO ? __builtin_bswap16(readNum<uint16_t>()) : readNum<uint16_t>();
    }

    int16_t readS16() {
        return std::bit_cast<int16_t>(readU16());
    }

    uint32_t readU32() {
        return swapBO ? __builtin_bswap32(readNum<uint32_t>()) : readNum<uint32_t>();
    }

    int32_t readS32() {
        return std::bit_cast<int32_t>(readU32());
    }

    void skip(const size_t off) {
        if (m_Pos + off > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob skip");
        m_Pos += off;
    }

    void rewind(const size_t off) {
        if (off > m_Pos) throw std::out_of_range("Resource oob rewind");
        m_Pos -= off;
    }

    void seek(const size_t off) {
        if (off > m_Resource.m_Data.size()) throw std::out_of_range("Resource oob seek");
        m_Pos = off;
    }

    [[nodiscard]] size_t tell() const {
        return m_Pos;
    }

    std::span<const uint8_t> getSpanAt(uint32_t offset, uint32_t size) {
        return {m_Resource.m_Data.begin() + offset, size};
    }

    bool swapBO = false;
private:
    const MemoryResource &m_Resource;
    size_t m_Pos = 0;
};

class OutMemoryStream {
public:
    explicit OutMemoryStream(MemoryResource &resource) : m_Resource(resource) {
    }

    template<std::integral Num>
    void writeNum(Num data) {
        if (m_Pos + sizeof(Num) > m_Resource.m_Data.size()) {
            m_Resource.m_Data.resize(m_Pos + sizeof(Num));
        }
        *reinterpret_cast<Num *>(m_Resource.getAsPtrUnsafe(m_Pos)) = data;
        m_Pos += sizeof(Num);
    }

    void writeU8(uint8_t num) {
        writeNum<uint8_t>(num);
    }

    void writeS8(int8_t num) {
        writeNum<int8_t>(num);
    }

    void writeU16(uint16_t num) {
        swapBO ? writeNum<uint16_t>(__builtin_bswap16(num)) : writeNum<uint16_t>(num);
    }

    void writeS16(int16_t num) {
        writeU16(std::bit_cast<uint16_t>(num));
    }

    void writeU32(uint32_t num) {
        swapBO ? writeNum<uint32_t>(__builtin_bswap32(num)) : writeNum<uint32_t>(num);
    }

    void writeS32(int32_t num) {
        writeU32(std::bit_cast<int32_t>(num));
    }

    size_t writeNull(const size_t bytes) {
        size_t res = tell();
        for (size_t i = 0; i < bytes; ++i) {
            writeU8(0);
        }
        return res;
    }

    size_t skip(const size_t off) {
        if (m_Pos + off > m_Resource.m_Data.size()) {
            m_Resource.m_Data.resize(m_Pos + off);
        }
        size_t oldPos = m_Pos;
        m_Pos += off;
        return oldPos;
    }

    void rewind(const size_t off) {
        if (off > m_Pos) throw std::out_of_range("Resource oob rewind");
        m_Pos -= off;
    }

    size_t seek(const size_t off) {
        size_t oldPos = m_Pos;
        if (off > m_Resource.m_Data.size()) {
            m_Resource.m_Data.resize(off);
        }
        m_Pos = off;
        return oldPos;
    }

    [[nodiscard]] size_t tell() const {
        return m_Pos;
    }

    uint32_t fillToAlign(uint32_t alignment) {
        uint32_t a = m_Pos % alignment;
        if (a == 0) return 0;
        a = alignment - a;
        writeNull(a);
        return a;
    }

    void writeBuffer(const std::span<const int8_t>& span) {
        writeBuffer(reinterpret_cast<const std::span<const uint8_t>&>(span));
    }

    void writeBuffer(const std::span<const uint8_t>& span) {
        uint32_t replaceCount = std::min(m_Resource.m_Data.size() - m_Pos, span.size());
        if (replaceCount > 0) {
            std::copy(span.begin(), span.begin() + replaceCount, m_Resource.m_Data.begin() + m_Pos);
        }
        if (replaceCount < span.size()) {
            m_Resource.m_Data.insert(m_Resource.m_Data.begin() + m_Pos + replaceCount, span.begin() + replaceCount, span.end());
        }
        m_Pos += span.size();
    }

    bool swapBO = false;
private:
    MemoryResource &m_Resource;
    size_t m_Pos = 0;
};
