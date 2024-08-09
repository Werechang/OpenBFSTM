//
// Created by cookieso on 6/24/24.
//

#pragma once

#include <cstdint>
#include <ostream>
#include <vector>
#include "BfFile.h"

class InMemoryStream;
class OutMemoryStream;

class MemoryResource;

struct BfstmData {
    uint32_t magic;
    uint32_t sectionSize;

    [[nodiscard]] bool verifyData() const;
};

struct BfstmHistoryInfo {
    int16_t histSample1;
    int16_t histSample2;
};

struct BfstmSeek {
    uint32_t magic;
    uint32_t sectionSize;
};

struct DSPAdpcmContext {
    uint16_t header;
    int16_t yn1;
    int16_t yn2;
};

struct BfstmDSPADPCMChannelInfo {
    int16_t coefficients[8][2];
    DSPAdpcmContext startContext;
    DSPAdpcmContext loopContext;
};

struct BfstmIMAADPCMChannelInfo {

};

struct BfstmChannelInfo {
    uint16_t sectionFlag;
    int32_t channelInfoOffset;
};

struct BfstmGlobalChannelIndexTable {
    uint32_t count;
    uint8_t *items;
};

struct BfstmTrackInfo {
    uint8_t volume;
    uint8_t pan;
    uint8_t span;
    uint8_t flags;
    uint16_t channelIndexTableFlag;
    uint16_t _u0;
    int32_t channelIndexTableOffset;
};

enum class SoundEncoding : uint8_t {
    PCM8 = 0,
    PCM16 = 1,
    DSP_ADPCM = 2,
    IMA_ADPCM = 3
};

inline std::ostream &operator<<(std::ostream &os, const SoundEncoding encoding) {
    switch (encoding) {
        case SoundEncoding::PCM8:
            os << "PCM8";
            break;
        case SoundEncoding::PCM16:
            os << "PCM16";
            break;
        case SoundEncoding::DSP_ADPCM:
            os << "DSP ADPCM";
            break;
        case SoundEncoding::IMA_ADPCM:
            os << "IMA ADPCM";
            break;
    }
    return os;
}

struct BfstmStreamInfo {
    SoundEncoding soundEncoding;
    uint8_t isLoop;
    uint8_t channelNum;
    uint8_t regionNum;
    uint32_t sampleRate;
    uint32_t loopStart;
    uint32_t sampleCount;
    uint32_t blockCountPerChannel;
    uint32_t blockSizeBytes;
    uint32_t blockSizeSamples;
    uint32_t lastBlockSizeBytes;
    uint32_t lastBlockSizeSamples;
    uint32_t lastBlockSizeBytesRaw;
    uint32_t channelSeekInfoSize;
    uint32_t seekSampleInterval;
    uint16_t sampleDataFlag;
    int32_t sampleDataOffset;
    uint16_t regionInfoSize;
    uint16_t regionInfoFlag;
    int32_t regionInfoOffset;
    uint32_t loopStartUnaligned;
    uint32_t loopEndUnaligned;
    uint32_t checksum;
};

struct BfstmRegionInfo {
    uint32_t startSample;
    uint32_t endSample;
};

struct BfstmRegion {
    uint32_t magic;
    uint32_t sectionSize;
};

struct BfstmInfo {
    uint32_t magic;
    uint32_t sectionSize;
    std::optional<ReferenceEntry> streamInfo, trackInfo, channelInfo;

    [[nodiscard]] bool verifyInfo() const;
};

struct BfstmHeader {
    uint32_t magic;
    uint16_t bom;
    uint16_t headerSize;
    uint32_t version;
    uint32_t fileSize;
    uint16_t sectionCount;
    std::optional<SectionInfo> infoSection, dataSection, seekSection, regionSection, pdatSection;

    friend std::ostream &operator<<(std::ostream &os, const BfstmHeader &obj) {
        return os << std::hex
                  << "magic: " << obj.magic
                  << " bom: " << obj.bom
                  << " headerSize: " << obj.headerSize
                  << " version: " << obj.version
                  << " fileSize: " << obj.fileSize
                  << " sectionCount: " << obj.sectionCount << std::dec;
    }

    [[nodiscard]] bool verifyHeader() const;
};

struct BfstmContext {
    BfstmHeader header{};
    BfstmInfo info{};
    BfstmStreamInfo streamInfo{};
    std::vector<BfstmTrackInfo> trackInfos{};
    std::vector<std::variant<BfstmDSPADPCMChannelInfo, BfstmIMAADPCMChannelInfo>> channelInfos{};
    std::vector<std::pair<BfstmRegionInfo, std::vector<DSPAdpcmContext>>> regionInfos{};
};

std::optional<std::variant<BfstmDSPADPCMChannelInfo, BfstmIMAADPCMChannelInfo>>
readChannelInfo(InMemoryStream &stream, SoundEncoding encoding);

BfstmTrackInfo readTrackInfo(InMemoryStream &stream);

ReferenceEntry readReferenceEntry(InMemoryStream &stream);

SectionInfo readSectionInfo(InMemoryStream &stream);

std::optional<BfstmContext> readBfstm(const MemoryResource &resource);

void writeBfstm(OutMemoryStream &stream, bool le);