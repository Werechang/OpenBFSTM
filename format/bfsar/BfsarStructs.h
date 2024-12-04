//
// Created by cookieso on 27.11.24.
//

#pragma once

#include <cstdint>

struct BfsarFile {
    uint32_t magic;
    uint32_t sectionSize;
};

struct BfsarInfo {
    uint32_t magic;
    uint32_t sectionSize;
};

struct SoundGroupInfo {
    uint32_t startId;
    uint32_t endId;
};

struct BankInfo {
    uint32_t fileId;
};

struct WaveArchiveInfo {
    uint32_t fileId;
    uint32_t fileNum;
};

struct FileInfo {
    uint16_t locType;
    int32_t offset;
};

struct SoundInfo {
    uint32_t fileId;
    uint32_t playerId;
    uint8_t volume;
    uint8_t remoteFilter;
    uint16_t soundType;
};

struct BfsarLutEntry {
    bool a;
    uint16_t b;
    uint32_t c;
    uint32_t d;

};

struct BfsarStringEntry {
    int32_t offset;
    uint32_t size;
};

struct BfsarStringTable {
    uint32_t magic;
    uint32_t sectionSize;
    int32_t stringTableOff;
    int32_t lutOffset;
};

struct BfsarHeader {
    uint32_t magic;
    uint16_t bom;
    uint16_t headerSize;
    uint32_t version;
    uint32_t fileSize;
    uint16_t sectionNum;
};
