//
// Created by cookieso on 03.12.24.
//

#pragma once


#include "../../MemoryResource.h"
#include "BfsarStructs.h"

class BfsarWriter {
public:
    explicit BfsarWriter(MemoryResource &resource, const BfsarContext& context);
private:
    struct LUTNode {
        LUTNode() = default;
        explicit LUTNode(std::string name) : name(std::move(name)) {};
        uint32_t charIdx = 0;
        uint32_t bitIndex = 0;
        std::string name;
        std::shared_ptr<LUTNode> left;
        std::shared_ptr<LUTNode> right;
    };

    void writeHeader(const BfsarContext& context);

    uint32_t writeStrg(const BfsarContext& context);

    void writeStrTable(const std::vector<std::string>& names);

    void writeLUT(const std::vector<std::string> &names);

    static std::optional<std::shared_ptr<LUTNode>> createLUT(const std::vector<std::string>& names);

    void printTree(const std::shared_ptr<BfsarWriter::LUTNode> &node, const std::string &prefix, bool isLeft);

    uint32_t writeInfo(const BfsarContext& context);

    void writeSoundInfo(const BfsarContext& context);

    void writeStreamSoundInfo(const BfsarStreamSound& stm);

    void writeWaveSoundInfo(const BfsarWaveSound& wav);

    void writeSequenceSoundInfo(const BfsarSequenceSound& seq);

    void writeSoundGroupInfo(const BfsarContext& context);

    void writeBankInfo(const BfsarContext& context);

    void writeWaveArchiveInfo(const BfsarContext& context);

    void writeGroupInfo(const BfsarContext& context);

    void writePlayerInfo(const BfsarContext& context);

    void writeFileInfo(const BfsarContext& context);

    void writeSoundArchivePlayerInfo(const BfsarContext& context);

    uint32_t writeFile(const BfsarContext& context);

    OutMemoryStream m_Stream;
};
