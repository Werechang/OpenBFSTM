//
// Created by cookieso on 6/24/24.
//
#pragma once
#include <cstdint>
#include <iostream>
#include <istream>
#include <memory>

class InMemoryStream;

class InMemoryResource {
public:
    explicit InMemoryResource(std::istream &input) {
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

    friend InMemoryStream;

    [[nodiscard]] const void *getAsPtrUnsafe(const size_t offset) const {
        return m_Data.data() + offset;
    }

private:
    std::vector<uint8_t> m_Data;
};

class InMemoryStream {
public:
    explicit InMemoryStream(const InMemoryResource &resource) : m_Resource(std::move(resource)) {
    }

    template<std::integral Num>
    Num readNum() {
        if (pos + 1 + sizeof(Num) >= m_Resource.m_Data.size()) throw std::out_of_range("Resource oob read");
        Num res = *static_cast<const Num*>(m_Resource.getAsPtrUnsafe(pos));
        pos += sizeof(Num);
        return res;
    }

    uint8_t readU8() {
        return readNum<uint8_t>();
    }

    int8_t readS8() {
        return static_cast<int8_t>(readU8());
    }

    uint16_t readU16() {
        return swapBO ? __builtin_bswap16(readNum<uint16_t>()) : readNum<uint16_t>();
    }

    int16_t readS16() {
        return static_cast<int16_t>(readU16());
    }

    uint32_t readU32() {
        return swapBO ? __builtin_bswap16(readNum<uint32_t>()) : readNum<uint32_t>();
    }

    int32_t readS32() {
        return static_cast<int32_t>(readU32());
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
    const InMemoryResource &m_Resource;
    size_t pos = 0;
};
