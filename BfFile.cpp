//
// Created by cookieso on 27.11.24.
//
#include "BfFile.h"

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