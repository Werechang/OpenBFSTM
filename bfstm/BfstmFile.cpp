//
// Created by cookieso on 6/24/24.
//

#include "BfstmFile.h"

#include "../MemoryResource.h"

bool BfstmData::verifyData() const {
    return this->magic == 0x41544144;
}

bool BfstmInfo::verifyInfo() const {
    return this->magic == 0x4f464e49;
}

bool BfstmHeader::verifyHeader() const {
    return this->magic == 0x4d545346 && this->bom == 0xfeff;
}

enum class WriteInfoVerificationResult {
    SUCCESS, UNSURE_ENCODING, UNSURE_RATE, UNSURE_LOOP, FAILURE
};

WriteInfoVerificationResult verifyWriteInfo(const BfstmWriteInfo& writeInfo) {
    WriteInfoVerificationResult result = WriteInfoVerificationResult::SUCCESS;
    if (writeInfo.encoding != SoundEncoding::PCM16 && writeInfo.encoding != SoundEncoding::DSP_ADPCM) {
        result = WriteInfoVerificationResult::UNSURE_ENCODING;
    }
    if (writeInfo.channelNum != 1 && writeInfo.channelNum % 2 != 0) {
        return WriteInfoVerificationResult::FAILURE;
    }
    if (writeInfo.isLoop) {
        // TODO Check loop sample index
        if (writeInfo.loopStart >= writeInfo.loopEnd) {
            return WriteInfoVerificationResult::FAILURE;
        }
    }
    constexpr std::array<int, 3> usedRates{22000, 32000, 48000};
    if (std::find(usedRates.begin(), usedRates.end(), writeInfo.sampleRate) == usedRates.end()) {
        result = WriteInfoVerificationResult::UNSURE_RATE;
    }
    return result;
}

void writeStreamBlock(OutMemoryStream &stream, const BfstmWriteInfo& writeInfo, uint8_t regionNum) {
    stream.writeU8(static_cast<uint8_t>(writeInfo.encoding));
    stream.writeU8(writeInfo.isLoop);
    // 1 or even number
    stream.writeU8(writeInfo.channelNum);
    stream.writeU8(regionNum);
    // 22000, 32000, 48000 used
    stream.writeU32(writeInfo.sampleRate);
    stream.writeU32(writeInfo.loopStart);
    stream.writeU32(writeInfo.loopEnd);
    // TODO, write 0x50 in total
    stream.skip(0x40);
}

void writeChannelBlock(OutMemoryStream& stream, const BfstmWriteInfo& writeInfo) {
    stream.writeU32(writeInfo.channelNum);
    int ciOffset = 0x4 + 0x8 * writeInfo.channelNum;
    for (int i = 0; i < writeInfo.channelNum; ++i) {
        stream.writeU16(0x4102);
        stream.writeU16(0);
        stream.writeS32(ciOffset);
        ciOffset += 0x8;
    }
    ciOffset = 0x8 * writeInfo.channelNum;
    for (int i = 0; i < writeInfo.channelNum; ++i) {
        if (writeInfo.encoding != SoundEncoding::DSP_ADPCM) {
            stream.writeU16(0);
            stream.writeU16(0);
            stream.writeS32(-1);
        } else {
            stream.writeU16(0x0300);
            stream.writeU16(0);
            stream.writeS32(ciOffset);
            ciOffset += 0x26;
        }
    }
    // TODO DSP Adpcm info
    if (writeInfo.encoding != SoundEncoding::DSP_ADPCM) return;
    for (int i = 0; i < writeInfo.channelNum; ++i) {
        stream.skip(0x2c);
    }
}

uint32_t writeInfoBlock(OutMemoryStream &stream, const BfstmWriteInfo& writeInfo) {
    uint32_t startPos = stream.tell();
    stream.writeU32(0x4f464e49);
    // 0x20 aligned
    uint32_t sizeOff = stream.skip(4);
    // stream info
    stream.writeU16(0x4100);
    stream.writeU16(0);
    stream.writeS32(0x18);
    // track info
    stream.writeU16(0);
    stream.writeU16(0);
    stream.writeS32(-1);
    // channel info
    stream.writeU16(0x0101);
    stream.writeU16(0);
    stream.writeS32(0x68);
    writeStreamBlock(stream, writeInfo, 0);
    writeChannelBlock(stream, writeInfo);
    uint32_t currentOff = stream.tell();
    if (currentOff % 0x20 != 0) {
        stream.skip(0x20 - (currentOff % 0x20));
    }
    uint32_t size = stream.seek(sizeOff) - startPos;
    stream.writeU32(size);
    return size;
}

// Unknown stuff:
// TODO Region Info without dspadpcm audio
void writeBfstm(OutMemoryStream &stream, const BfstmWriteInfo& writeInfo) {
    bool hasRegionInfo;
    int sectionCount = 3;
    // Info and data always, seek section only in dspadpcm, region section very rare
    if (hasRegionInfo) ++sectionCount;
    if (writeInfo.encoding != SoundEncoding::DSP_ADPCM) --sectionCount;
    stream.writeU32(0x4d545346);
    stream.writeU16(0xfeff);
    // Size is 0x60 for the file with region info
    int headerSize = sectionCount == 4 ? 0x60 : 0x40;
    stream.writeU16(headerSize);
    stream.writeU32(0x00060100);
    size_t fileSizeOff = stream.skip(4);
    stream.writeU16(sectionCount);
    stream.writeU16(0);
    // info
    stream.writeU16(0x4000);
    stream.writeU16(0);
    stream.writeS32(headerSize);
    size_t infoSectionSizeOff = stream.skip(4);
    uint32_t seekOffsetOff = 0;
    uint32_t seekSizeOff = 0;
    if (writeInfo.encoding == SoundEncoding::DSP_ADPCM) {
        // seek
        stream.writeU16(0x4001);
        stream.writeU16(0);
        seekOffsetOff = stream.skip(4);
        seekSizeOff = stream.skip(4);
    }
    // data
    stream.writeU16(0x4002);
    stream.writeU16(0);
    stream.writeS32(0);
    stream.writeU32(0);
    stream.seek(headerSize);
    uint32_t infoSize = writeInfoBlock(stream, writeInfo);
    stream.seek(infoSectionSizeOff);
    stream.writeU32(infoSize);
    if (writeInfo.encoding == SoundEncoding::DSP_ADPCM) {
        stream.seek(seekOffsetOff);
        stream.writeS32(infoSize + headerSize);
    }
}
