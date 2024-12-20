//
// Created by cookieso on 08.12.24.
//

#include "BfwavReader.h"

BfwavReader::BfwavReader(const MemoryResource &resource) : m_Stream(resource) {
    m_Context = readHeader();
}

std::optional<BfwavReadContext> BfwavReader::readHeader() {
    uint32_t magic = m_Stream.readU32();
    uint16_t bom = m_Stream.readU16();
    if (bom != 0xFEFF) {
        if (bom != 0xFFFE) return std::nullopt;
        m_Stream.swapBO = true;
    }
    if constexpr (std::endian::native == std::endian::little) {
        magic = __builtin_bswap32(magic);
    }
    if (magic != 0x46574156) {
        std::cerr << "FWAV file magic does not match!" << std::endl;
        return std::nullopt;
    }
    uint16_t headerSize = m_Stream.readU16();
    uint32_t version = m_Stream.readU32();
    if (version != 0x10200 && version != 0x10100)
        std::cout << "Warning: FWAV version might not be supported. (0x" << std::hex << version << std::dec << ')'
                  << std::endl;
    uint32_t fileSize = m_Stream.readU32();
    return readHeaderSections();
}

std::optional<BfwavReadContext> BfwavReader::readHeaderSections() {
    uint32_t sectionNum = m_Stream.readU16();
    m_Stream.skip(2);
    std::optional<SectionInfo> infoSection, dataSection;
    for (int i = 0; i < sectionNum; ++i) {
        switch (auto section = readSectionInfo(m_Stream); section.flag) {
            case 0x7000:
                infoSection = section;
                break;
            case 0x7001:
                dataSection = section;
                break;
            default:
                std::cerr << "Unsupported section with flag " << section.flag << " in FWAV" << std::endl;
                return std::nullopt;
        }
    }
    m_Stream.seek(infoSection->offset);
    auto context = readInfo();

    if (!context) return std::nullopt;
    m_Stream.seek(dataSection->offset);
    m_DataOffset = readData();
    if (m_DataOffset == 0) return std::nullopt;
    return context;
}

std::optional<BfwavReadContext> BfwavReader::readInfo() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x4f464e49) {
        std::cerr << "INFO magic in FWAR file does not match!" << std::endl;
        return std::nullopt;
    }
    uint32_t size = m_Stream.readU32();
    BfwavReadContext context{};
    context.format = static_cast<SoundEncoding>(m_Stream.readU8());
    std::cout << context.format << std::endl;
    bool isLoop = m_Stream.readU8();
    m_Stream.skip(2);
    context.sampleRate = m_Stream.readU32();
    if (isLoop) {
        BfwavLoopInfo loopInfo{};
        loopInfo.loopStartSample = m_Stream.readU32();
        context.sampleCount = m_Stream.readU32();
        loopInfo.alignedLoopStartSample = m_Stream.readU32();
        if (loopInfo.alignedLoopStartSample == 0) {
            loopInfo.alignedLoopStartSample = loopInfo.loopStartSample;
        }
        context.loopInfo = loopInfo;
    } else {
        m_Stream.skip(4);
        context.sampleCount = m_Stream.readU32();
        m_Stream.skip(4);
    }
    uint32_t ciStart = m_Stream.tell();
    context.channelNum = m_Stream.readU32();
    std::vector<int32_t> offsets{};
    for (int i = 0; i < context.channelNum; ++i) {
        auto ref = readReferenceEntry(m_Stream);
        if (ref.flag != 0x7100) {
            std::cerr << "Channel Reference flag " << std::hex << ref.flag << " unknown in FWAV info" << std::endl;
            return std::nullopt;
        }
        offsets.emplace_back(ref.offset);
    }
    // read channel info
    for (int32_t offset : offsets) {
        m_Stream.seek(ciStart + offset);
        auto dataRef = readReferenceEntry(m_Stream);
        if (dataRef.flag != 0x1f00) {
            std::cerr << "Data Reference flag " << std::hex << dataRef.flag << " unknown in FWAV info" << std::endl;
            return std::nullopt;
        }
        context.channelDataOffsets.emplace_back(dataRef.offset);
        auto dspAdpcmRef = readReferenceEntry(m_Stream);
        if (dspAdpcmRef.flag != 0x0300) {
            std::cerr << "Data Reference flag " << std::hex << dspAdpcmRef.flag << " unknown in FWAV info" << std::endl;
            return std::nullopt;
        }
        // read dsp channel info
        if (context.format == SoundEncoding::DSP_ADPCM) {
            m_Stream.seek(ciStart + offset + dspAdpcmRef.offset);
            BfstmDSPADPCMChannelInfo dsp{};
            for (auto &coefficient: dsp.coefficients) {
                coefficient[0] = m_Stream.readS16();
                coefficient[1] = m_Stream.readS16();
            }
            dsp.startContext.header = m_Stream.readU16();
            dsp.startContext.yn1 = m_Stream.readS16();
            dsp.startContext.yn2 = m_Stream.readS16();
            dsp.loopContext.header = m_Stream.readU16();
            dsp.loopContext.yn1 = m_Stream.readS16();
            dsp.loopContext.yn2 = m_Stream.readS16();
            context.dspAdpcmChannelInfo.emplace_back(dsp);
        }
    }

    return context;
}

uint32_t BfwavReader::readData() {
    uint32_t magic = m_Stream.readU32();
    if (magic != 0x41544144) {
        std::cerr << "DATA magic in FWAV file does not match!" << std::endl;
        return 0;
    }

    uint32_t size = m_Stream.readU32();
    return m_Stream.tell();
}
