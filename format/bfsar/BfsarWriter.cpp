//
// Created by cookieso on 03.12.24.
//

#include <stack>
#include "BfsarWriter.h"

BfsarWriter::BfsarWriter(MemoryResource &resource, const BfsarContext &context) : m_Stream(resource) {
    uint16_t type = 0;
    for (auto &snd: context.sounds) {
        if (snd.subInfo.index() < type) {
            std::cerr << "Sounds in writing context are not sorted by type!" << std::endl;
            return;
        } else {
            type = snd.subInfo.index();
        }
    }
    WriteInfo writeInfo{};
    // TODO Do ids only exist for entries with a string? (prob not)
    uint32_t id = 0;
    for (auto &entry: context.sounds) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x1000000);
        }
        ++id;
    }
    id = 0;
    for (auto &entry: context.soundGroups) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x2000000);
        }
        ++id;
    }
    id = 0;
    for (auto &entry: context.banks) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x3000000);
        }
        ++id;
    }
    id = 0;
    for (auto &entry: context.waveArchives) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x5000000);
        }
        ++id;
    }
    id = 0;
    for (auto &entry: context.groups) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x6000000);
        }
        ++id;
    }
    id = 0;
    for (auto &entry: context.players) {
        if (!entry.name.empty()) {
            writeInfo.strTable.emplace_back(&entry.name);
            writeInfo.strIndexedIds.emplace_back(id | 0x4000000);
        }
        ++id;
    }

    writeHeader(context, writeInfo);
}

void BfsarWriter::writeHeader(const BfsarContext &context, const WriteInfo &writeInfo) {
    m_Stream.writeU32(0x52415346);
    m_Stream.writeU16(0xfeff);
    m_Stream.writeU16(0x40);
    m_Stream.writeU32(0x20400);
    uint32_t sizeOff = m_Stream.skip(4);
    m_Stream.writeU16(0x3);
    m_Stream.writeNull(2);
    m_Stream.writeU16(0x2000);
    m_Stream.writeNull(2);
    m_Stream.writeS32(0x40);
    uint32_t strgSizeOff = m_Stream.skip(4);
    m_Stream.writeU16(0x2001);
    m_Stream.writeNull(2);
    uint32_t infoSecInfOff = m_Stream.skip(8);
    m_Stream.writeU16(0x2002);
    m_Stream.writeNull(2);
    uint32_t fileSecInfOff = m_Stream.skip(8);

    m_Stream.seek(0x40);
    uint32_t strgSize = writeStrgSec(writeInfo);
    uint32_t infoOff = 0x40 + strgSize;
    m_Stream.seek(infoOff);
    uint32_t infoSize = writeInfoSec(context, writeInfo);
    uint32_t fileOff = infoOff + infoSize;
    m_Stream.seek(fileOff);
    uint32_t fileSize = writeFileSec(context);
    uint32_t total = fileOff + fileSize;

    m_Stream.seek(sizeOff);
    m_Stream.writeU32(total);
    m_Stream.seek(strgSizeOff);
    m_Stream.writeU32(strgSize);
    m_Stream.seek(infoSecInfOff);
    m_Stream.writeU32(infoOff);
    m_Stream.writeU32(infoSize);
    m_Stream.seek(fileSecInfOff);
    m_Stream.writeU32(fileOff);
    m_Stream.writeU32(fileSize);
}

uint32_t BfsarWriter::writeStrgSec(const WriteInfo &writeInfo) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x47525453);
    uint32_t paddedSizeOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2400);
    m_Stream.writeNull(2);
    m_Stream.writeU32(0x10);

    m_Stream.writeU16(0x2401);
    m_Stream.writeNull(2);
    uint32_t lutOffOff = m_Stream.skip(4);

    writeStrTable(writeInfo.strTable);
    int32_t lutOff = m_Stream.tell() - (start + 0x8);
    writeLUT(writeInfo);
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);

    m_Stream.seek(lutOffOff);
    m_Stream.writeS32(lutOff);
    return paddedSize;
}

void BfsarWriter::writeStrTable(const std::vector<const std::string *> &names) {
    m_Stream.writeU32(names.size());
    uint32_t poolOffset = 0;
    for (int i = 0; i < names.size(); ++i) {
        m_Stream.writeU16(0x1f01);
        m_Stream.writeNull(2);
        m_Stream.writeS32(poolOffset + 0xc * names.size() + 0x4);
        m_Stream.writeU32(names[i]->size() + 1);
        poolOffset += names[i]->size() + 1;
    }
    for (auto name: names) {
        m_Stream.writeBuffer(std::span(reinterpret_cast<const uint8_t *>(name->data()), name->size()));
        m_Stream.writeNull(1);
    }
    m_Stream.fillToAlign(4);
}

inline uint8_t getHighestBitIndex(uint32_t num) {
    uint8_t bitIndex = 0;
    while (num >>= 1) {
        ++bitIndex;
    }
    return bitIndex;
}

void BfsarWriter::printTree(const std::shared_ptr<BfsarWriter::LUTNode> &node, const std::string &prefix, bool isLeft) {
    std::cout << prefix << (isLeft ? "├─" : "└─");

    if (node->left && node->right) {
        std::cout << node->charIdx << '*' << node->bitIndex << std::endl;
        printTree(node->right, prefix + (isLeft ? "│ " : "  "), true);
        printTree(node->left, prefix + (isLeft ? "│ " : "  "), false);
    } else {
        std::cout << node->name << std::endl;
    }
}

std::optional<std::shared_ptr<BfsarWriter::LUTNode>>
BfsarWriter::createLUT(std::vector<const std::string *> &names) {
    std::shared_ptr<LUTNode> root = std::make_shared<LUTNode>();
    if (names.size() == 1) {
        root->name = *names[0];
        return root;
    }
    std::stack<std::pair<std::shared_ptr<LUTNode>, const std::span<const std::string *>>> callStack{};
    callStack.emplace(root, names);
    next:
    while (!callStack.empty()) {
        auto [parent, remaining] = std::move(callStack.top());
        callStack.pop();
        if (remaining.size() == 2) {
            parent->right = std::make_shared<LUTNode>(*remaining[0]);
            parent->left = std::make_shared<LUTNode>(*remaining[1]);
            int chrIdx = 0;
            while (chrIdx < INT32_MAX) {
                uint8_t firstChr = remaining[0]->size() > chrIdx ? remaining[0]->at(chrIdx) : 0;
                uint8_t secondChr = remaining[1]->size() > chrIdx ? remaining[1]->at(chrIdx) : 0;
                if (firstChr != secondChr) {
                    parent->charIdx = chrIdx;
                    parent->bitIndex = getHighestBitIndex(secondChr ^ firstChr);
                    goto next;
                }
                ++chrIdx;
            }
            std::cerr << "Strings must be different!" << std::endl;
            return std::nullopt;
        }
        int chrIdx = 0;
        while (chrIdx < INT32_MAX) {
            for (int i = 1; i < remaining.size(); ++i) {
                uint8_t firstChr = remaining[0]->size() > chrIdx ? remaining[0]->at(chrIdx) : 0;
                uint8_t secondChr = remaining[i]->size() > chrIdx ? remaining[i]->at(chrIdx) : 0;
                if (firstChr == secondChr) continue;
                uint8_t maxDiffBit = 0;
                int maxDiffIdx = i;
                for (int j = i; j < remaining.size(); ++j) {
                    uint8_t bitIndex = getHighestBitIndex(secondChr ^ firstChr);
                    if (bitIndex > maxDiffBit) {
                        maxDiffIdx = j;
                        maxDiffBit = bitIndex;
                    }
                }

                parent->charIdx = chrIdx;
                parent->bitIndex = maxDiffBit;

                if (maxDiffIdx == 1) {
                    parent->right = std::make_shared<LUTNode>(*remaining[0]);
                    parent->left = std::make_shared<LUTNode>();
                    callStack.emplace(parent->left,
                                      std::span<const std::string *>(remaining.begin() + 1, remaining.size() - 1));
                } else if (maxDiffIdx + 1 == remaining.size()) {
                    parent->right = std::make_shared<LUTNode>();
                    parent->left = std::make_shared<LUTNode>(*remaining[maxDiffIdx]);
                    callStack.emplace(parent->right,
                                      std::span<const std::string *>(remaining.begin(), remaining.size() - 1));
                } else {
                    parent->right = std::make_shared<LUTNode>();
                    callStack.emplace(parent->right, std::span<const std::string *>(remaining.begin(), maxDiffIdx));
                    parent->left = std::make_shared<LUTNode>();
                    callStack.emplace(parent->left, std::span<const std::string *>(remaining.begin() + maxDiffIdx,
                                                                                   remaining.size() - maxDiffIdx));
                }
                goto next;
            }
            ++chrIdx;
        }
        std::cerr << "Strings must be different!" << std::endl;
        return std::nullopt;
    }
    return root;
}

void BfsarWriter::writeLUT(const WriteInfo &writeInfo) {
    if (writeInfo.strTable.empty()) {
        m_Stream.writeU32(-1);
        m_Stream.writeU32(0);
        return;
    }
    std::vector<const std::string *> sortableNames = writeInfo.strTable;
    std::sort(sortableNames.begin(), sortableNames.end(), [](const std::string *a, const std::string *b) {
        return *a < *b;
    });
    auto root = createLUT(sortableNames);
    if (!root) abort();
    // Traverse in in-order because it is written that way I think
    std::stack<std::shared_ptr<BfsarWriter::LUTNode>> traverse{};
    std::vector<std::shared_ptr<BfsarWriter::LUTNode>> out{};
    traverse.emplace(*root);
    std::shared_ptr<BfsarWriter::LUTNode> current = *root;
    while (current || !traverse.empty()) {
        while (current) {
            traverse.push(current);
            current = current->left;
        }
        current = traverse.top();
        traverse.pop();

        out.emplace_back(current);

        current = current->right;
    }

    m_Stream.writeU32(std::find(out.begin(), out.end(), *root) - out.begin());
    m_Stream.writeU32(out.size());
    for (auto &node: out) {
        m_Stream.writeU8(!node->name.empty());
        m_Stream.writeNull(1);
        m_Stream.writeU16((node->charIdx << 3) | (~node->bitIndex & 0x7));
        m_Stream.writeU32(node->left ? std::find(out.begin(), out.end(), node->left) - out.begin() : 0xffffffff);
        m_Stream.writeU32(node->right ? std::find(out.begin(), out.end(), node->right) - out.begin() : 0xffffffff);
        uint32_t strIndex = node->name.empty() ? -1 :
                            std::find_if(writeInfo.strTable.begin(), writeInfo.strTable.end(), [&node](const std::string* n) {
                                return *n == node->name;
                            }) -
                            writeInfo.strTable.begin();
        m_Stream.writeU32(strIndex);
        m_Stream.writeU32(strIndex == -1 ? -1 : writeInfo.strIndexedIds[strIndex]);
    }
}

uint32_t BfsarWriter::writeInfoSec(const BfsarContext &context, const WriteInfo &writeInfo) {
    m_Stream.writeU32(0x4f464e49);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    uint32_t start = m_Stream.tell();

    m_Stream.writeU16(0x2100);
    m_Stream.writeNull(2);
    uint32_t siOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2104);
    m_Stream.writeNull(2);
    uint32_t sgiOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2101);
    m_Stream.writeNull(2);
    uint32_t biOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2103);
    m_Stream.writeNull(2);
    uint32_t wiOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2105);
    m_Stream.writeNull(2);
    uint32_t giOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2102);
    m_Stream.writeNull(2);
    uint32_t piOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2106);
    m_Stream.writeNull(2);
    uint32_t fiOffOff = m_Stream.skip(4);

    m_Stream.writeU16(0x220B);
    m_Stream.writeNull(2);
    uint32_t spOffOff = m_Stream.skip(4);

    uint32_t siOff = 0x40;
    m_Stream.seek(start + siOff);
    uint32_t siSize = writeSoundInfo(context, writeInfo);
    uint32_t sgiOff = siOff + siSize;
    m_Stream.seek(start + sgiOff);
    uint32_t sgiSize = writeSoundGroupInfo(context, writeInfo);
    uint32_t biOff = sgiOff + sgiSize;
    m_Stream.seek(start + biOff);
    uint32_t biSize = writeBankInfo(context, writeInfo);
    uint32_t wiOff = biOff + biSize;
    m_Stream.seek(start + wiOff);
    uint32_t wiSize = writeWaveArchiveInfo(context, writeInfo);
    uint32_t giOff = wiOff + wiSize;
    m_Stream.seek(start + giOff);
    uint32_t giSize = writeGroupInfo(context, writeInfo);
    uint32_t piOff = giOff + giSize;
    m_Stream.seek(start + piOff);
    uint32_t piSize = writePlayerInfo(context, writeInfo);
    uint32_t fiOff = piOff + piSize;
    m_Stream.seek(start + fiOff);
    uint32_t fiSize = writeFileInfo(context);
    uint32_t spOff = fiOff + fiSize;
    m_Stream.seek(start + spOff);
    writeSoundArchivePlayerInfo(context);

    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - (start - 0x8);
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);

    m_Stream.seek(siOffOff);
    m_Stream.writeS32(siOff);
    m_Stream.seek(sgiOffOff);
    m_Stream.writeS32(sgiOff);
    m_Stream.seek(biOffOff);
    m_Stream.writeS32(biOff);
    m_Stream.seek(wiOffOff);
    m_Stream.writeS32(wiOff);
    m_Stream.seek(giOffOff);
    m_Stream.writeS32(giOff);
    m_Stream.seek(piOffOff);
    m_Stream.writeS32(piOff);
    m_Stream.seek(fiOffOff);
    m_Stream.writeS32(fiOff);
    m_Stream.seek(spOffOff);
    m_Stream.writeS32(spOff);

    return paddedSize;
}

uint32_t BfsarWriter::writeSoundInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.sounds.size());
    std::vector<uint32_t> sizes{};
    for (auto &snd: context.sounds) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(snd.fileIndex);
        m_Stream.writeU32(snd.playerId);
        m_Stream.writeU8(snd.initialVolume);
        m_Stream.writeU8(snd.remoteFilter);
        m_Stream.writeNull(2);
        switch (snd.subInfo.index()) {
            case 0:
                m_Stream.writeU16(0x2201);
                break;
            case 1:
                m_Stream.writeU16(0x2202);
                break;
            case 2:
                m_Stream.writeU16(0x2203);
                break;
            default:
                abort();
        }
        m_Stream.writeNull(2);
        uint32_t siOffOff = m_Stream.skip(4);
        uint32_t flagOff = m_Stream.skip(4);
        uint32_t flags = 0;
        if (!snd.name.empty()) {
            flags |= 1;
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &snd.name) -
                              writeInfo.strTable.begin());
        }
        if (snd.panInfo) {
            flags |= 1 << 1;
            m_Stream.writeU8(snd.panInfo->mode);
            m_Stream.writeU8(snd.panInfo->curve);
            m_Stream.writeNull(2);
        }
        if (snd.actorPlayerInfo) {
            flags |= 1 << 2;
            m_Stream.writeU8(snd.actorPlayerInfo->playerPriority);
            m_Stream.writeU8(snd.actorPlayerInfo->actorPlayerId);
            m_Stream.writeNull(2);
        }
        if (snd.singlePlayInfo) {
            m_Stream.writeU16(snd.singlePlayInfo->type);
            m_Stream.writeU16(snd.singlePlayInfo->effectiveDuration);
        }
        uint32_t s3dOff;
        if (snd.sound3DInfo) {
            flags |= 1 << 8;
            s3dOff = m_Stream.skip(4);
        }
        if (snd.isFrontBypass.has_value()) {
            flags |= 1 << 17;
            m_Stream.writeU32(*snd.isFrontBypass);
        }
        flags |= 1 << 31;
        m_Stream.writeU32(0);

        if (snd.sound3DInfo) {
            writeSound3DInfo(*snd.sound3DInfo);
            uint32_t tmpOff = m_Stream.tell();

            m_Stream.seek(s3dOff);
            m_Stream.writeU32(0x14 + 4 * std::popcount(flags));
            m_Stream.seek(tmpOff);
        }
        // TODO Padding or data apparently, si offset is 0x3c (5 flags) or 0x44 (6 flags)
        m_Stream.skip(4);
        uint32_t siOff = m_Stream.tell();
        m_Stream.seek(siOffOff);
        m_Stream.writeU32(siOff - es);
        m_Stream.seek(siOff);

        switch (snd.subInfo.index()) {
            case 0:
                writeStreamSoundInfo(get<BfsarStreamSound>(snd.subInfo));
                break;
            case 1:
                writeWaveSoundInfo(get<BfsarWaveSound>(snd.subInfo));
                break;
            case 2:
                writeSequenceSoundInfo(get<BfsarSequenceSound>(snd.subInfo));
                break;
            default:
                abort();
        }
        uint32_t ee = m_Stream.tell();

        m_Stream.seek(flagOff);
        m_Stream.writeU32(flags);

        m_Stream.seek(ee);
        sizes.emplace_back(ee - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.sounds.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.sounds.size(); ++i) {
        m_Stream.writeU16(0x2200);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.sounds.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

void BfsarWriter::writeSound3DInfo(const BfsarSound3D &s3d) {
    m_Stream.writeU32(s3d.flags);
    m_Stream.writeU32(std::bit_cast<uint32_t>(s3d.unkFloat));
    m_Stream.writeU8(s3d.unkBool0);
    m_Stream.writeU8(s3d.unkBool1);
    m_Stream.writeNull(2);
}

void BfsarWriter::writeStreamSoundInfo(const BfsarStreamSound &stm) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU16(stm.validTracks);
    m_Stream.writeU16(stm.channelCount);
    m_Stream.writeU16(0x0101);
    m_Stream.writeNull(2);
    m_Stream.writeS32(0x24);
    m_Stream.writeU32(std::bit_cast<uint32_t>(stm.unkFloat));
    m_Stream.writeU16(0x220f);
    m_Stream.writeNull(2);
    uint32_t sendValueOffOff = m_Stream.skip(4);
    m_Stream.writeU16(0);
    m_Stream.writeNull(2);
    m_Stream.writeS32(-1);
    m_Stream.writeU32(stm.unk);
    m_Stream.writeU32(stm.trackInfo.size());
    for (int i = 0; i < stm.trackInfo.size(); ++i) {
        m_Stream.writeU16(0x220e);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * stm.trackInfo.size() + 0x28 * i);
    }
    for (auto &ti: stm.trackInfo) {
        m_Stream.writeU8(ti.unk0);
        m_Stream.writeU8(ti.unk1);
        m_Stream.writeU8(ti.unk2);
        m_Stream.writeU8(ti.unk3);
        m_Stream.writeU16(0x0100);
        m_Stream.writeNull(2);
        m_Stream.writeS32(0x18);
        m_Stream.writeU16(0x220f);
        m_Stream.writeNull(2);
        m_Stream.writeS32(0x20);
        m_Stream.writeU8(ti.unk4);
        m_Stream.writeU8(ti.unk5);
        m_Stream.writeNull(2);
        m_Stream.writeU32(ti.trackChannelInfo.channels);
        m_Stream.writeU8(ti.trackChannelInfo.channelIndexL);
        m_Stream.writeU8(ti.trackChannelInfo.channelIndexR);
        m_Stream.writeNull(2);
        m_Stream.writeU32(0x7f);
        m_Stream.writeNull(4);
    }
    uint32_t sendValueOff = m_Stream.tell();
    m_Stream.writeU32(0x7f);
    m_Stream.seek(sendValueOffOff);
    m_Stream.writeU32(sendValueOff - start);

    m_Stream.seek(sendValueOff + 4);
}

void BfsarWriter::writeWaveSoundInfo(const BfsarWaveSound &wav) {
    m_Stream.writeU32(wav.archiveId);
    m_Stream.writeU32(wav.unk);
    if (wav.prioInfo) {
        m_Stream.writeU32(1);
        m_Stream.writeU32(wav.prioInfo->channelPrio);
        m_Stream.writeU32(wav.prioInfo->isReleasePrioFix);
    } else {
        m_Stream.writeU32(0);
    }
}

void BfsarWriter::writeSequenceSoundInfo(const BfsarSequenceSound &seq) {
    m_Stream.writeU16(0x0100);
    m_Stream.writeNull(2);
    uint32_t bIdTOffOff = m_Stream.skip(4);
    m_Stream.writeU32(seq.validTracks);
    uint32_t flagOff = m_Stream.skip(4);
    uint32_t flags = 0;
    if (seq.startOffset) {
        flags |= 1;
        m_Stream.writeU32(*seq.startOffset);
    }
    if (seq.prioInfo) {
        flags |= 2;
        m_Stream.writeU8(seq.prioInfo->channelPrio);
        m_Stream.writeU8(seq.prioInfo->isReleasePrioFix);
        m_Stream.writeNull(2);
    }

    m_Stream.writeU32(seq.bankIds.size());
    for (auto id : seq.bankIds) {
        m_Stream.writeU32(id);
    }
    uint32_t end = m_Stream.tell();

    m_Stream.seek(bIdTOffOff);
    m_Stream.writeU32(0x10 + 4 * std::popcount(flags));

    m_Stream.seek(flagOff);
    m_Stream.writeU32(flags);

    m_Stream.seek(end);
}

uint32_t BfsarWriter::writeSoundGroupInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.soundGroups.size());
    std::vector<uint32_t> sizes{};
    for (auto &sg: context.soundGroups) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(sg.startId);
        m_Stream.writeU32(sg.endId);
        m_Stream.writeU16(0x0100);
        m_Stream.writeNull(2);
        m_Stream.writeS32(0x20);
        if (sg.waveArcIdTable) {
            m_Stream.writeU16(0x2205);
            m_Stream.writeNull(2);
            m_Stream.writeU32(0x24 + 0x4 * sg.fileIndices.size());
        } else {
            m_Stream.writeU16(0);
            m_Stream.writeNull(2);
            m_Stream.writeS32(-1);
        }
        if (!sg.name.empty()) {
            m_Stream.writeU32(1);
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &sg.name) -
                              writeInfo.strTable.begin());
        } else {
            m_Stream.writeU32(0);
            m_Stream.writeNull(4);
        }
        m_Stream.writeU32(sg.fileIndices.size());
        for (auto &fileIndex: sg.fileIndices) {
            m_Stream.writeU32(fileIndex);
        }
        if (sg.waveArcIdTable) {
            m_Stream.writeU16(0x0100);
            m_Stream.writeNull(2);
            m_Stream.writeS32(0xc);
            m_Stream.writeNull(4);
            m_Stream.writeU32(sg.waveArcIdTable->size());
            for (auto id : *sg.waveArcIdTable) {
                m_Stream.writeU32(id);
            }
        }
        sizes.emplace_back(m_Stream.tell() - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.soundGroups.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.soundGroups.size(); ++i) {
        m_Stream.writeU16(0x2204);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.soundGroups.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

uint32_t BfsarWriter::writeBankInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.banks.size());
    std::vector<uint32_t> sizes{};
    for (auto &bnk: context.banks) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(bnk.fileIndex);
        m_Stream.writeU16(0x0100);
        m_Stream.writeNull(2);
        m_Stream.writeS32(0x14);
        if (!bnk.name.empty()) {
            m_Stream.writeU32(1);
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &bnk.name) -
                              writeInfo.strTable.begin());
        } else {
            m_Stream.writeU32(0);
        }
        m_Stream.writeU32(0);
        sizes.emplace_back(m_Stream.tell() - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.banks.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.banks.size(); ++i) {
        m_Stream.writeU16(0x2206);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.banks.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

uint32_t BfsarWriter::writeWaveArchiveInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.waveArchives.size());
    std::vector<uint32_t> sizes{};
    for (auto &war: context.waveArchives) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(war.fileIndex);
        m_Stream.writeU32(war.unk);
        uint32_t flagOff = m_Stream.skip(4);
        uint32_t flags = 0;
        if (!war.name.empty()) {
            flags |= 1;
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &war.name) -
                              writeInfo.strTable.begin());
        }
        if (war.waveCount) {
            flags |= 2;
            m_Stream.writeU32(*war.waveCount);
        }
        uint32_t ee = m_Stream.tell();

        m_Stream.seek(flagOff);
        m_Stream.writeU32(flags);

        m_Stream.seek(ee);

        sizes.emplace_back(ee - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.waveArchives.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.waveArchives.size(); ++i) {
        m_Stream.writeU16(0x2207);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.waveArchives.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

uint32_t BfsarWriter::writeGroupInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.groups.size());
    std::vector<uint32_t> sizes{};
    for (auto &grp: context.groups) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(grp.fileIndex);

        if (!grp.name.empty()) {
            m_Stream.writeU32(1);
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &grp.name) -
                              writeInfo.strTable.begin());
        } else {
            m_Stream.writeU32(0);
        }
        sizes.emplace_back(m_Stream.tell() - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.groups.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.groups.size(); ++i) {
        m_Stream.writeU16(0x2208);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.groups.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

uint32_t BfsarWriter::writePlayerInfo(const BfsarContext &context, const WriteInfo &writeInfo) {
    uint32_t table = m_Stream.skip(4 + 8 * context.players.size());
    std::vector<uint32_t> sizes{};
    for (auto &plr: context.players) {
        uint32_t es = m_Stream.tell();
        m_Stream.writeU32(plr.playableSoundLimit);
        uint32_t flagOff = m_Stream.skip(4);
        uint32_t flags = 0;
        if (!plr.name.empty()) {
            flags |= 1;
            m_Stream.writeU32(std::find(writeInfo.strTable.begin(), writeInfo.strTable.end(), &plr.name) -
                              writeInfo.strTable.begin());
        }
        if (plr.playerHeapSize) {
            flags |= 2;
            m_Stream.writeU32(*plr.playerHeapSize);
        }
        uint32_t ee = m_Stream.tell();
        m_Stream.seek(flagOff);
        m_Stream.writeU32(flags);
        m_Stream.seek(ee);
        sizes.emplace_back(ee - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.players.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.players.size(); ++i) {
        m_Stream.writeU16(0x2209);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.players.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

uint32_t BfsarWriter::writeFileInfo(const BfsarContext &context) {
    // Order: STP, STM (external), SEQ, WSD, BNK, WAR
    uint32_t table = m_Stream.skip(4 + 8 * context.fileInfo.size());
    std::vector<uint32_t> sizes{};
    uint32_t nextFileOff = 0x18;
    for (auto &fileInfo: context.fileInfo) {
        uint32_t es = m_Stream.tell();
        if (fileInfo.isExternal()) {
            m_Stream.writeU16(0x220d);
        } else {
            m_Stream.writeU16(0x220c);
        }
        m_Stream.writeNull(2);
        m_Stream.writeS32(0xc);
        m_Stream.writeNull(4);
        if (fileInfo.isExternal()) {
            auto extInfo = fileInfo.getExternal();
            m_Stream.writeBuffer(
                    std::span{reinterpret_cast<const uint8_t *>(extInfo.name.data()), extInfo.name.size()});
            m_Stream.fillToAlign(4);
        } else if (fileInfo.isInternal()) {
            auto intInfo = fileInfo.getInternal();
            m_Stream.writeU16(0x1f00);
            m_Stream.writeNull(2);
            m_Stream.writeS32(nextFileOff);
            m_Stream.writeU32(intInfo.data.size());

            uint32_t alignment = intInfo.data.size() % 0x20;
            if (alignment != 0) {
                alignment = 0x20 - alignment;
            }
            nextFileOff += intInfo.data.size() + alignment;

            m_Stream.writeU16(0x0100);
            m_Stream.writeNull(2);
            m_Stream.writeS32(0x14);
            m_Stream.writeU32(0);
        } else if (fileInfo.isInternalNull()) {
            auto gt = fileInfo.getInternalNull().groupTable;
            m_Stream.writeNull(4);
            m_Stream.writeS32(-1);
            m_Stream.writeU32(-1);
            m_Stream.writeU16(0x0100);
            m_Stream.writeNull(2);
            m_Stream.writeS32(0x14);
            m_Stream.writeU32(gt.size());
            for (auto i: gt) {
                m_Stream.writeU32(i);
            }
        }
        sizes.emplace_back(m_Stream.tell() - es);
    }
    uint32_t endSize = m_Stream.tell();
    m_Stream.seek(table);
    m_Stream.writeU32(context.fileInfo.size());
    uint32_t acc = 0;
    for (int i = 0; i < context.fileInfo.size(); ++i) {
        m_Stream.writeU16(0x220a);
        m_Stream.writeNull(2);
        m_Stream.writeS32(4 + 8 * context.fileInfo.size() + acc);
        acc += sizes[i];
    }
    return endSize - table;
}

void BfsarWriter::writeSoundArchivePlayerInfo(const BfsarContext &context) {
    auto sarInf = context.sarPlayer;
    m_Stream.writeU16(sarInf.sequenceLimit);
    m_Stream.writeU16(sarInf.sequenceTrackLimit);
    m_Stream.writeU16(sarInf.streamLimit);
    m_Stream.writeU16(sarInf.unkLimit);
    m_Stream.writeU16(sarInf.streamChannelLimit);
    m_Stream.writeU16(sarInf.waveLimit);
    m_Stream.writeU16(sarInf.unkWaveLimit);
    m_Stream.writeU8(sarInf.streamBufferTimes);
    m_Stream.writeU8(sarInf.isAdvancedWave);
}

uint32_t BfsarWriter::writeFileSec(const BfsarContext &context) {
    std::vector<std::span<const uint8_t>> files;
    for (auto &fileInfo: context.fileInfo) {
        if (fileInfo.isInternal()) {
            files.emplace_back(fileInfo.getInternal().data);
        }
    }
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x454c4946);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.fillToAlign(0x20);
    for (auto &data: files) {
        m_Stream.writeBuffer(data);
        m_Stream.fillToAlign(0x20);
    }
    m_Stream.fillToAlign(0x20);
    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}
