//
// Created by cookieso on 03.12.24.
//

#include <stack>
#include "BfsarWriter.h"

// TODO Everything
BfsarWriter::BfsarWriter(MemoryResource &resource, const BfsarContext &context) : m_Stream(resource) {
    writeHeader(context);
}

void BfsarWriter::writeHeader(const BfsarContext &context) {
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
    uint32_t strgSize = writeStrg(context);
    uint32_t infoOff = 0x40 + strgSize;
    m_Stream.seek(infoOff);
    uint32_t infoSize = writeInfo(context);
    uint32_t fileOff = infoOff + infoSize;
    m_Stream.seek(fileOff);
    uint32_t fileSize = writeFile(context);
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

uint32_t BfsarWriter::writeStrg(const BfsarContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x47525453);
    uint32_t sizeOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2400);
    m_Stream.writeNull(2);
    m_Stream.writeU32(0x10);

    m_Stream.writeU16(0x2401);
    m_Stream.writeNull(2);
    uint32_t lutOffOff = m_Stream.skip(4);

    // TODO Names vector: sorted by id, Sound (Order: Stream, Wave, Sequence), Sound Group, Bank, Group, Player
    std::vector<std::string> names;
    writeStrTable(names);
    int32_t lutOff = m_Stream.tell() - start;
    writeLUT(names);
    m_Stream.fillToAlign(0x20);
    uint32_t total = m_Stream.tell();

    m_Stream.seek(lutOffOff);
    m_Stream.writeS32(lutOff);
    return total - start;
}

void BfsarWriter::writeStrTable(const std::vector<std::string> &names) {
    m_Stream.writeU32(names.size());
    uint32_t poolOffset = 0;
    for (int i = 0; i < names.size(); ++i) {
        m_Stream.writeU16(0x1f01);
        m_Stream.writeNull(2);
        m_Stream.writeS32(poolOffset + 0xc * names.size() + 0x4);
        m_Stream.writeU32(names[i].size() + 1);
    }
    for (auto &name: names) {
        m_Stream.writeBuffer(std::span(reinterpret_cast<const uint8_t *>(name.data()), name.size()));
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

std::optional<std::shared_ptr<BfsarWriter::LUTNode>> BfsarWriter::createLUT(const std::vector<std::string> &names) {
    std::shared_ptr<LUTNode> root = std::make_shared<LUTNode>();
    if (names.size() == 1) {
        root->name = names[0];
        return root;
    }
    std::stack<std::pair<std::shared_ptr<LUTNode>, std::span<const std::string>>> callStack{};
    callStack.emplace(root, names);
    next:
    while (!callStack.empty()) {
        auto [parent, remaining] = std::move(callStack.top());
        callStack.pop();
        if (remaining.size() == 2) {
            parent->right = std::make_shared<LUTNode>(remaining[0]);
            parent->left = std::make_shared<LUTNode>(remaining[1]);
            for (int chrIdx = 0; chrIdx < remaining[0].size(); ++chrIdx) {
                if ((remaining[1].size() == chrIdx + 1) || (remaining[0][chrIdx] != remaining[1][chrIdx])) {
                    parent->charIdx = chrIdx;
                    parent->bitIndex = getHighestBitIndex(remaining[1][chrIdx] ^ remaining[0][chrIdx]);
                    goto next;
                }
            }
            return std::nullopt;
        }
        for (int chrIdx = 0; chrIdx < remaining[0].size(); ++chrIdx) {
            for (int i = 1; i < remaining.size(); ++i) {
                if (remaining[i].size() > chrIdx && remaining[0][chrIdx] == remaining[i][chrIdx]) continue;
                uint8_t maxDiffBit = 0;
                int maxDiffIdx = i;
                for (int j = i; j < remaining.size(); ++j) {
                    uint8_t bitIndex = getHighestBitIndex(remaining[j][chrIdx] ^ remaining[0][chrIdx]);
                    if (bitIndex > maxDiffBit) {
                        maxDiffIdx = j;
                        maxDiffBit = bitIndex;
                    }
                }

                parent->charIdx = chrIdx;
                parent->bitIndex = maxDiffBit;

                if (maxDiffIdx == 1) {
                    parent->right = std::make_shared<LUTNode>(remaining[0]);
                    parent->left = std::make_shared<LUTNode>();
                    callStack.emplace(parent->left,
                                      std::span<const std::string>(remaining.begin() + 1, remaining.size() - 1));
                } else if (maxDiffIdx + 1 == remaining.size()) {
                    parent->right = std::make_shared<LUTNode>();
                    parent->left = std::make_shared<LUTNode>(remaining[maxDiffIdx]);
                    callStack.emplace(parent->right,
                                      std::span<const std::string>(remaining.begin(), remaining.size() - 1));
                } else {
                    parent->right = std::make_shared<LUTNode>();
                    callStack.emplace(parent->right, std::span<const std::string>(remaining.begin(), maxDiffIdx));
                    parent->left = std::make_shared<LUTNode>();
                    callStack.emplace(parent->left, std::span<const std::string>(remaining.begin() + maxDiffIdx,
                                                                                 remaining.size() - maxDiffIdx));
                }
                goto next;
            }
        }
        std::cerr << "Strings must be different!" << std::endl;
        return std::nullopt;
    }
    return root;
}

void BfsarWriter::writeLUT(const std::vector<std::string> &names) {
    if (names.empty()) {
        m_Stream.writeU32(-1);
        m_Stream.writeU32(0);
    }
    std::vector<std::string> sortableNames = names;
    std::sort(sortableNames.begin(), sortableNames.end());
    auto root = createLUT(sortableNames);
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

    for (auto &node: out) {
        m_Stream.writeU8(!node->name.empty());
        m_Stream.writeNull(1);
        m_Stream.writeU16((node->charIdx << 3) | ~node->bitIndex);
        m_Stream.writeU32(node->left ? std::find(out.begin(), out.end(), node->left) - out.begin() : 0xffffffff);
        m_Stream.writeU32(node->right ? std::find(out.begin(), out.end(), node->right) - out.begin() : 0xffffffff);
        m_Stream.writeU32(
                node->name.empty() ? 0xffffffff : std::find(names.begin(), names.end(), node->name) - names.begin());
        //m_Stream.writeU32();
    }
}

uint32_t BfsarWriter::writeInfo(const BfsarContext &context) {
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x4f464e49);
    uint32_t paddedSizeOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2100);
    m_Stream.writeNull(2);
    uint32_t siOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2104);
    m_Stream.writeNull(2);
    uint32_t sgiOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2101);
    m_Stream.writeNull(2);
    uint32_t biOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2103);
    m_Stream.writeNull(2);
    uint32_t wiOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2105);
    m_Stream.writeNull(2);
    uint32_t giOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2102);
    m_Stream.writeNull(2);
    uint32_t piOff = m_Stream.skip(4);

    m_Stream.writeU16(0x2106);
    m_Stream.writeNull(2);
    uint32_t fiOff = m_Stream.skip(4);

    m_Stream.writeU16(0x220B);
    m_Stream.writeNull(2);
    uint32_t spOff = m_Stream.skip(4);

    // TODO

    uint32_t paddedSize = m_Stream.tell() - start;
    m_Stream.seek(paddedSizeOff);
    m_Stream.writeU32(paddedSize);
    return paddedSize;
}

void BfsarWriter::writeSoundInfo(const BfsarContext &context) {
    for (auto &snd : context.sounds) {
        m_Stream.writeU32();
        m_Stream.writeU32(0x04000000);
        m_Stream.writeU8(snd.initialVolume);
        m_Stream.writeU8(0);
        m_Stream.writeNull(2);
        switch (snd.subInfo.index()) {
            case 0: m_Stream.writeU16(0x2201);
                break;
            case 1: m_Stream.writeU16(0x2202);
                break;
            case 2: m_Stream.writeU16(0x2203);
                break;
            default:
                abort();
        }
        m_Stream.writeNull(2);
        m_Stream.writeS32();
        uint32_t flagOff = m_Stream.skip(4);
        uint32_t flags = 0;
        if (!snd.name.empty()) {
            flags |= 1;
            //m_Stream.writeU32();
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
        if (snd.sound3DInfo) {
            flags |= 1 << 8;
            m_Stream.writeS32();
        }
        if (snd.isFrontBypass.has_value()) {
            flags |= 1 << 17;
            m_Stream.writeU32(*snd.isFrontBypass);
        }
        flags |= 1 << 31;
        m_Stream.writeNull(4);

        m_Stream.seek(flagOff);
        m_Stream.writeU32(flags);
    }
}

void BfsarWriter::writeStreamSoundInfo(const BfsarStreamSound &stm) {
    m_Stream.writeU16(stm.validTracks);
    m_Stream.writeU16(stm.channelCount);
    m_Stream.writeU16(0x0101);
    m_Stream.writeNull(2);
    m_Stream.writeS32();
    m_Stream.writeU32(std::bit_cast<uint32_t>(1.0f));
    m_Stream.writeU16(0x220f);
    m_Stream.writeNull(2);
    m_Stream.writeS32();
    m_Stream.writeU16(0x2210);
    m_Stream.writeNull(2);
    m_Stream.writeS32();
    m_Stream.writeU32();
}

void BfsarWriter::writeWaveSoundInfo(const BfsarWaveSound &wav) {
    m_Stream.writeU32(wav.archiveId);
    m_Stream.writeU32(1);
    if (wav.prioInfo) {
        m_Stream.writeU32(1);
        m_Stream.writeU32(wav.prioInfo->channelPrio);
        m_Stream.writeU32(wav.prioInfo->isReleasePrioFix);
    }
}

void BfsarWriter::writeSequenceSoundInfo(const BfsarSequenceSound &seq) {
    m_Stream.writeU16(0x0100);
    m_Stream.writeNull(2);
    m_Stream.writeS32();
    m_Stream.writeU32(seq.validTracks);
    uint32_t flagOff = m_Stream.skip(4);
    uint32_t flags = 0;
    if (seq.startOffset) {
        flags |= 1;
        m_Stream.writeU32(*seq.startOffset);
    }
    if (seq.prioInfo) {
        flags |= 2;
        m_Stream.writeU32(seq.prioInfo->channelPrio);
        m_Stream.writeU32(seq.prioInfo->isReleasePrioFix);
    }
    m_Stream.seek(flagOff);
    m_Stream.writeU32(flags);
}

void BfsarWriter::writeSoundGroupInfo(const BfsarContext &context) {
    for (auto &sg: context.soundGroups) {
        m_Stream.writeU32(sg.startId);
        m_Stream.writeU32(sg.endId);
        m_Stream.writeU16(0x0100);
        m_Stream.writeNull(2);
        //m_Stream.writeS32();
        // TODO
    }
}

void BfsarWriter::writeBankInfo(const BfsarContext &context) {
    for (auto &bnk: context.banks) {
        //m_Stream.writeU32();
        m_Stream.writeU16(0x0100);
        m_Stream.writeNull(2);
        //m_Stream.writeS32();
        if (!bnk.name.empty()) {
            m_Stream.writeU32(1);
            //m_Stream.writeU32();
        }
    }
}

void BfsarWriter::writeWaveArchiveInfo(const BfsarContext &context) {
    for (auto &war: context.waveArchives) {
        //m_Stream.writeU32();
        //m_Stream.writeU32();
        uint32_t flagOff = m_Stream.skip(4);
        uint32_t flags = 0;
        if (!war.name.empty()) {
            flags |= 1;
            //m_Stream.writeU32();
        }
        if (war.waveCount) {
            flags |= 2;
            m_Stream.writeU32(war.waveCount.value());
        }
        m_Stream.seek(flagOff);
        m_Stream.writeU32(flags);
    }
}

void BfsarWriter::writeGroupInfo(const BfsarContext &context) {
    for (auto &grp: context.groups) {
        //m_Stream.writeU32();
        if (!grp.name.empty()) {
            m_Stream.writeU32(1);
            //m_Stream.writeU32();
        }
    }
}

void BfsarWriter::writePlayerInfo(const BfsarContext &context) {
    for (auto &plr: context.players) {
        m_Stream.writeU32(plr.playableSoundLimit);
        if (!plr.name.empty()) {
            m_Stream.writeU32(1);
            //m_Stream.writeU32();
        }
    }
}

void BfsarWriter::writeFileInfo(const BfsarContext &context) {
    // Order: STP, STM (external), SEQ, WSD, BNK, WAR
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
    m_Stream.writeU8(1);
    m_Stream.writeU8(0);
}

uint32_t BfsarWriter::writeFile(const BfsarContext &context) {
    // TODO Fill vector
    std::vector<std::span<const uint8_t>> files;
    uint32_t start = m_Stream.tell();
    m_Stream.writeU32(0x454c4946);
    uint32_t paddedSizeOff = m_Stream.skip(4);
    m_Stream.fillToAlign(0x40);
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
