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

    explicit MemoryResource(size_t size) {
        m_Data.resize(size);
    }

    friend InMemoryStream;
    friend OutMemoryStream;

    [[nodiscard]] void *getAsPtrUnsafe(const size_t offset) {
        return m_Data.data() + offset;
    }

    [[nodiscard]] const void *getAsPtrUnsafe(const size_t offset) const {
        return m_Data.data() + offset;
    }

    void writeToFile(const char* file) {
        std::ofstream out{file, std::ios::binary};
        out.write(reinterpret_cast<const char*>(&m_Data[0]), m_Data.size());
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
        if (pos + 1 + sizeof(Num) >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob read");
        Num res = *reinterpret_cast<const Num*>(m_Resource.getAsPtrUnsafe(pos));
        pos += sizeof(Num);
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
        if (pos + off + 1 >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob skip");
        pos += off;
    }

    void rewind(const size_t off) {
        if (off > pos) throw std::out_of_range("Resource oob rewind");
        pos -= off;
    }

    void seek(const size_t off) {
        if (off + 1 >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob seek");
        pos = off;
    }

    [[nodiscard]] size_t tell() const {
        return pos;
    }

    bool swapBO = false;
private:
    const MemoryResource &m_Resource;
    size_t pos = 0;
};

// TODO resize
class OutMemoryStream {
public:
    explicit OutMemoryStream(MemoryResource &resource) : m_Resource(resource) {
    }

    template<std::integral Num>
    void writeNum(Num data) {
        if (pos + 1 + sizeof(Num) >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob write");
        *reinterpret_cast<Num*>(m_Resource.getAsPtrUnsafe(pos)) = data;
        pos += sizeof(Num);
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
        if (pos + off + 1 >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob skip");
        size_t oldPos = pos;
        pos += off;
        return oldPos;
    }

    void rewind(const size_t off) {
        if (off > pos) throw std::out_of_range("Resource oob rewind");
        pos -= off;
    }

    size_t seek(const size_t off) {
        size_t oldPos = pos;
        if (off + 1 >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob seek");
        pos = off;
        return oldPos;
    }

    [[nodiscard]] size_t tell() const {
        return pos;
    }

    bool swapBO = false;
private:
    MemoryResource &m_Resource;
    size_t pos = 0;
};
