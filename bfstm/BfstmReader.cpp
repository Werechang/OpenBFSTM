//
// Created by cookieso on 27.11.24.
//

#include <variant>
#include "BfstmReader.h"
#include "BfstmFile.h"

BfstmReader::BfstmReader(const MemoryResource &resource) : m_Stream(resource) {
    if (!readBfstm()) {
        success = false;
    }
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
    BfstmTrackInfo info{};
    info.volume = stream.readU8();
    info.pan = stream.readU8();
    info.flags = stream.readU8();
    info.channelIndexTableFlag = stream.readU16();
    stream.skip(2);
    info.channelIndexTableOffset = stream.readS32();
    return info;
}

bool BfstmReader::readHeader() {
    BfstmHeader &header = m_Context.header;
    header.magic = m_Stream.readU32();
    header.bom = m_Stream.readU16();
    if (header.bom != 0xFEFF) {
        if (header.bom != 0xFFFE) return false;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        header.magic = __builtin_bswap32(header.magic);
    }
    header.headerSize = m_Stream.readU16();
    header.version = m_Stream.readU32();
    if (header.version != 0x60100)
        std::cout << "Warning: BFSTM version might not be supported. (0x" << std::hex << header.version << std::dec << ')'
                  << std::endl;
    header.fileSize = m_Stream.readU32();
    header.sectionCount = m_Stream.readU16();
    m_Stream.skip(2);
    for (int i = 0; i < header.sectionCount; ++i) {
        switch (auto section = readSectionInfo(m_Stream); section.flag) {
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
                return false;
        }
    }
    if (!header.infoSection || !header.dataSection) {
        std::cerr << "File has no info or data section!" << std::endl;
        return false;
    }
    return true;
}

void BfstmReader::readStreamInfo() {
    BfstmStreamInfo &streamInfo = m_Context.streamInfo;
    streamInfo.soundEncoding = static_cast<SoundEncoding>(m_Stream.readU8());
    streamInfo.isLoop = m_Stream.readU8();
    streamInfo.channelNum = m_Stream.readU8();
    streamInfo.regionNum = m_Stream.readU8();
    streamInfo.sampleRate = m_Stream.readU32();
    streamInfo.loopStart = m_Stream.readU32();
    streamInfo.loopEnd = m_Stream.readU32();
    streamInfo.blockCountPerChannel = m_Stream.readU32();
    streamInfo.blockSizeBytes = m_Stream.readU32();
    streamInfo.blockSizeSamples = m_Stream.readU32();
    streamInfo.lastBlockSizeBytes = m_Stream.readU32();
    streamInfo.lastBlockSizeSamples = m_Stream.readU32();
    streamInfo.lastBlockSizeBytesRaw = m_Stream.readU32();
    streamInfo.channelSeekInfoSize = m_Stream.readU32();
    streamInfo.seekSampleInterval = m_Stream.readU32();
    streamInfo.sampleDataFlag = m_Stream.readU16();
    m_Stream.skip(2);
    streamInfo.sampleDataOffset = m_Stream.readS32();
    streamInfo.regionInfoSize = m_Stream.readU16();
    m_Stream.skip(2);
    streamInfo.regionInfoFlag = m_Stream.readU16();
    m_Stream.skip(2);
    streamInfo.regionInfoOffset = m_Stream.readS32();
    if (m_Context.header.version > 0x40000) {
        // TODO Set to loopStart/loopEnd when version < 4
        streamInfo.loopStartUnaligned = m_Stream.readU32();
        streamInfo.loopEndUnaligned = m_Stream.readU32();
    }
    if (m_Context.header.version >= 0x50000) {
        streamInfo.checksum = m_Stream.readU32();
    }
}

bool BfstmReader::readBfstm() {
    // TODO crc32check available from version 5
    if (!readHeader()) {
        return false;
    }
    BfstmHeader &header = m_Context.header;

    // Info
    m_Stream.seek(header.infoSection->offset);
    BfstmInfo &info = m_Context.info;
    info.magic = m_Stream.readU32();
    info.sectionSize = m_Stream.readU32();
    auto strInf = readReferenceEntry(m_Stream);
    if (strInf.flag == 0x4100) info.streamInfo = strInf;
    // TODO Only used before version 2.0.1
    auto trInf = readReferenceEntry(m_Stream);
    if (trInf.flag == 0x0101) info.trackInfo = trInf;
    auto chInf = readReferenceEntry(m_Stream);
    if (chInf.flag == 0x0101) info.channelInfo = chInf;
    if (!info.streamInfo) {
        std::cerr << "No stream info found!" << std::endl;
        return false;
    }

    // Stream Info
    m_Stream.seek(header.infoSection->offset + 0x8 + info.streamInfo->offset);
    readStreamInfo();

    // Track Info
    if (info.trackInfo) {
        size_t startOff = header.infoSection->offset + 0x8 + info.trackInfo->offset;
        m_Stream.seek(startOff);
        uint32_t refCount = m_Stream.readU32();
        if (refCount > 8) {
            std::cout << "Warning: Track info has more than 8 references, but only 8 are supported." << std::endl;
            refCount = 8;
        }
        auto offsets = std::vector<int32_t>(refCount);
        for (int i = 0; i < refCount; ++i) {
            auto refEntry = readReferenceEntry(m_Stream);
            std::cout << std::hex << refEntry.flag << std::endl;
            if (refEntry.flag == 0x4101) {
                offsets[i] = refEntry.offset;
            }
        }
        for (auto offset: offsets) {
            m_Stream.seek(startOff + offset);
            m_Context.trackInfos.emplace_back(readTrackInfo(m_Stream));
        }
    }
    auto &streamInfo = m_Context.streamInfo;

    // Channel Info
    if (info.channelInfo && m_Context.streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM ||
        m_Context.streamInfo.soundEncoding == SoundEncoding::IMA_ADPCM) {
        size_t startOff = header.infoSection->offset + 0x8 + info.channelInfo->offset;
        m_Stream.seek(startOff);
        uint32_t refCount = m_Stream.readU32();
        auto offsets = std::vector<int32_t>(refCount);
        for (int i = 0; i < refCount; ++i) {
            auto refEntry = readReferenceEntry(m_Stream);
            if (refEntry.flag == 0x4102) {
                offsets[i] = refEntry.offset;
            } else {
                std::cerr << "Unknown entry flag in channel info array" << refEntry.flag << std::endl;
            }
        }
        for (auto offset: offsets) {
            m_Stream.seek(startOff + offset);
            m_Context.channelInfos.emplace_back(readChannelInfo(m_Stream, streamInfo.soundEncoding).value());
        }
    }

    if ((streamInfo.soundEncoding == SoundEncoding::DSP_ADPCM ||
         streamInfo.soundEncoding == SoundEncoding::IMA_ADPCM) &&
        m_Context.channelInfos.size() != streamInfo.channelNum) {
        std::cerr << "Channel Number does not match channel info number!" << std::endl;
    }

    if (header.regionSection) {
        m_Stream.seek(header.regionSection->offset);
        BfstmRegion region{};
        region.magic = m_Stream.readU32();
        if (region.magic != 0x4e474552) {
            std::cerr << "Region section invalid!" << std::endl;
        }
        region.sectionSize = m_Stream.readU32();
        uint32_t baseOff = header.regionSection->offset + 0x8 + streamInfo.regionInfoOffset;
        for (int i = 0; i < streamInfo.regionNum; ++i) {
            m_Stream.seek(baseOff + i * streamInfo.regionInfoSize);
            BfstmRegionInfo regInfo{};
            regInfo.startSample = m_Stream.readU32();
            regInfo.endSample = m_Stream.readU32();

            auto ctx = std::vector<DSPAdpcmContext>(streamInfo.channelNum);
            for (int j = 0; j < streamInfo.channelNum; ++j) {
                ctx[j] = DSPAdpcmContext{m_Stream.readU16(), m_Stream.readS16(), m_Stream.readS16()};
            }
            m_Context.regionInfos.emplace_back(regInfo, ctx);
        }
    }

    if (header.seekSection) {
        m_Stream.seek(header.seekSection->offset);
        BfstmSeek seek{};
        seek.magic = m_Stream.readU32();
        seek.sectionSize = m_Stream.readU32();
    }

    m_Stream.seek(header.dataSection->offset);
    BfstmData data{};
    data.magic = m_Stream.readU32();
    data.sectionSize = m_Stream.readU32();

    return true;
}
