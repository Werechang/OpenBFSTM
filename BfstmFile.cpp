//
// Created by cookieso on 6/24/24.
//

#include "BfstmFile.h"

#include "MemoryResource.h"

bool BfstmData::verifyData() const {
    return this->magic == 0x41544144;
}

bool BfstmInfo::verifyInfo() const {
    return this->magic == 0x4f464e49;
}

bool BfstmHeader::verifyHeader() const {
    return this->magic == 0x4d545346 && this->bom == 0xfeff;
}

std::optional<std::variant<BfstmDSPADPCMChannelInfo, BfstmIMAADPCMChannelInfo>>
readChannelInfo(InMemoryStream &stream, SoundEncoding encoding) {
    size_t current = stream.tell();
    uint16_t flag = stream.readU16();
    if (flag != 0x0300) {
        std::cerr << "Channel info flag invalid!" << std::hex << flag << std::endl;
        return std::nullopt;
    }
    stream.skip(2);
    int32_t off = stream.readS32();
    stream.seek(current + off);
    if (encoding == SoundEncoding::DSP_ADPCM) {
        BfstmDSPADPCMChannelInfo dsp{};
        for (auto &coefficient: dsp.coefficients) {
            coefficient[0] = stream.readS16();
            coefficient[1] = stream.readS16();
        }
        dsp.startContext.header = stream.readU16();
        dsp.startContext.yn1 = stream.readS16();
        dsp.startContext.yn2 = stream.readS16();
        dsp.loopContext.header = stream.readU16();
        dsp.loopContext.yn1 = stream.readS16();
        dsp.loopContext.yn2 = stream.readS16();
        return {dsp};
    } else if (encoding == SoundEncoding::IMA_ADPCM) {
        std::cerr << "IMA ADPCM Channel Info not yet supported!" << std::endl;
        return {};
    } else {
        return {};
    }
}

BfstmTrackInfo readTrackInfo(InMemoryStream &stream) {
    return BfstmTrackInfo();
}

ReferenceEntry readReferenceEntry(InMemoryStream &stream) {
    ReferenceEntry entry{};
    entry.flag = stream.readU16();
    stream.skip(2);
    entry.offset = stream.readS32();
    return entry;
}

SectionInfo readSectionInfo(InMemoryStream &stream) {
    SectionInfo info{};
    info.flag = stream.readU16();
    stream.skip(2);
    info.offset = stream.readS32();
    info.size = stream.readU32();
    return info;
}

std::optional<BfstmContext> readBfstm(const MemoryResource &resource) {
    // TODO crc32check available from version 5
    InMemoryStream stream{resource};
    BfstmContext context{};
    BfstmHeader &header = context.header;
    header.magic = stream.readU32();
    header.bom = stream.readU16();
    if (header.bom != 0xFEFF) {
        if (header.bom != 0xFFFE) return {};
        stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        header.magic = __builtin_bswap32(header.magic);
    }
    header.headerSize = stream.readU16();
    header.version = stream.readU32();
    if (header.version != 0x60100)
        std::cout << "Warning: BFSTM version might not be supported. (0x" << std::hex << header.version << std::dec << ')'
                  << std::endl;
    header.fileSize = stream.readU32();
    header.sectionCount = stream.readU16();
    stream.skip(2);
    for (int i = 0; i < header.sectionCount; ++i) {
        switch (auto section = readSectionInfo(stream); section.flag) {
            case 0x4000:
                header.infoSection = section;
                break;
            case 0x4001:
                header.seekSection = section;
                break;
            case 0x4002:
                header.dataSection = section;
                break;
            case 0x4003:
                header.regionSection = section;
                break;
            case 0x4004:
                // Shouldn't be used
                std::cout << "Warning: pdat section should be unused. Is the file the right version?" << std::endl;
                header.pdatSection = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << std::endl;
                return {};
        }
    }
    if (!header.infoSection || !header.dataSection) {
        std::cerr << "File has no info or data section!" << std::endl;
        return {};
    }

    // Info
    stream.seek(header.infoSection->offset);
    BfstmInfo &info = context.info;
    info.magic = stream.readU32();
    info.sectionSize = stream.readU32();
    auto strInf = readReferenceEntry(stream);
    if (strInf.flag == 0x4100) info.streamInfo = strInf;
    auto trInf = readReferenceEntry(stream);
    if (trInf.flag == 0x0101) info.trackInfo = trInf;
    auto chInf = readReferenceEntry(stream);
    if (chInf.flag == 0x0101) info.channelInfo = chInf;
    if (!info.streamInfo) {
        std::cerr << "No stream info found!" << std::endl;
        return {};
    }

    // Stream Info
    stream.seek(header.infoSection->offset + 0x8 + info.streamInfo->offset);
    BfstmStreamInfo &streamInfo = context.streamInfo;
    streamInfo.soundEncoding = static_cast<SoundEncoding>(stream.readU8());
    streamInfo.isLoop = stream.readU8();
    streamInfo.channelNum = stream.readU8();
    streamInfo.regionNum = stream.readU8();
    streamInfo.sampleRate = stream.readU32();
    streamInfo.loopStart = stream.readU32();
    streamInfo.sampleCount = stream.readU32();
    streamInfo.blockCountPerChannel = stream.readU32();
    streamInfo.blockSizeBytes = stream.readU32();
    streamInfo.blockSizeSamples = stream.readU32();
    streamInfo.lastBlockSizeBytes = stream.readU32();
    streamInfo.lastBlockSizeSamples = stream.readU32();
    streamInfo.lastBlockSizeBytesRaw = stream.readU32();
    streamInfo.channelSeekInfoSize = stream.readU32();
    streamInfo.seekSampleInterval = stream.readU32();
    streamInfo.sampleDataFlag = stream.readU16();
    stream.skip(2);
    streamInfo.sampleDataOffset = stream.readS32();
    streamInfo.regionInfoSize = stream.readU16();
    stream.skip(2);
    streamInfo.regionInfoFlag = stream.readU16();
    stream.skip(2);
    streamInfo.regionInfoOffset = stream.readS32();
    if (header.version > 0x40000) {
        streamInfo.loopStartUnaligned = stream.readU32();
        streamInfo.loopEndUnaligned = stream.readU32();
    }
    if (header.version >= 0x50000) {
        streamInfo.checksum = stream.readU32();
    }

    // Track Info
    if (info.trackInfo) {
        size_t startOff = header.infoSection->offset + 0x8 + info.trackInfo->offset;
        stream.seek(startOff);
        uint32_t refCount = stream.readU32();
        if (refCount > 8) {
            std::cout << "Warning: Track info has more than 8 references, but only 8 are supported." << std::endl;
            refCount = 8;
        }
        auto offsets = std::vector<int32_t>(refCount);
        for (int i = 0; i < refCount; ++i) {
            auto refEntry = readReferenceEntry(stream);
            std::cout << std::hex << refEntry.flag << std::endl;
            if (refEntry.flag == 0x4101) {
                offsets[i] = refEntry.offset;
            }
        }
        for (auto offset: offsets) {
            stream.seek(startOff + offset);
            context.trackInfos.emplace_back(readTrackInfo(stream));
        }
    }

    // Channel Info
    if (info.channelInfo && context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM ||
        context.streamInfo.soundEncoding == SoundEncoding::IMA_ADPCM) {
        size_t startOff = header.infoSection->offset + 0x8 + info.channelInfo->offset;
        stream.seek(startOff);
        uint32_t refCount = stream.readU32();
        auto offsets = std::vector<int32_t>(refCount);
        for (int i = 0; i < refCount; ++i) {
            auto refEntry = readReferenceEntry(stream);
            if (refEntry.flag == 0x4102) {
                offsets[i] = refEntry.offset;
            }
        }
        for (auto offset: offsets) {
            stream.seek(startOff + offset);
            context.channelInfos.emplace_back(readChannelInfo(stream, streamInfo.soundEncoding).value());
        }
    }

    if ((streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM ||
         streamInfo.soundEncoding == SoundEncoding::IMA_ADPCM) &&
        context.channelInfos.size() != streamInfo.channelNum) {
        std::cerr << "Channel Number does not match channel info number!" << std::endl;
    }

    if (header.regionSection) {
        stream.seek(header.regionSection->offset);
        BfstmRegion region{};
        region.magic = stream.readU32();
        if (region.magic != 0x4e474552) {
            std::cerr << "Region section invalid!" << std::endl;
        }
        region.sectionSize = stream.readU32();
        uint32_t baseOff = header.regionSection->offset + 0x8 + streamInfo.regionInfoOffset;
        for (int i = 0; i < streamInfo.regionNum; ++i) {
            stream.seek(baseOff + i * streamInfo.regionInfoSize);
            BfstmRegionInfo regInfo{};
            regInfo.startSample = stream.readU32();
            regInfo.endSample = stream.readU32();

            auto ctx = std::vector<DSPAdpcmContext>(streamInfo.channelNum);
            for (int j = 0; j < streamInfo.channelNum; ++j) {
                ctx[j] = DSPAdpcmContext{stream.readU16(), stream.readS16(), stream.readS16()};
            }
            context.regionInfos.emplace_back(regInfo, ctx);
        }
    }

    if (header.seekSection) {
        stream.seek(header.seekSection->offset);
        BfstmSeek seek{};
        seek.magic = stream.readU32();
        seek.sectionSize = stream.readU32();
    }

    stream.seek(header.dataSection->offset);
    BfstmData data{};
    data.magic = stream.readU32();
    data.sectionSize = stream.readU32();

    return context;
}

void writeStreamInfo(OutMemoryStream &stream, SoundEncoding encoding) {
    stream.writeU8(static_cast<uint8_t>(encoding));
}

void writeBfstm(OutMemoryStream &stream, bool le) {
    stream.writeU32(0x4d545346);
    stream.writeU16(0xfeff);
    stream.writeU16(0x40);
    stream.writeU32(0x00060100);
    size_t fileSizeOff = stream.skip(4);
    stream.writeU16(0x3);
    stream.writeU16(0);
    // info
    stream.writeU16(0x4000);
    stream.writeU16(0);
    stream.writeS32(0x40);
    //stream.writeU32(size);
    // seek
    stream.writeU16(0x4001);
    stream.writeU16(0);
    //stream.writeS32(off);
    //stream.writeU32(size);
    // data
}
