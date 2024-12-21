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
        explicit LUTNode(const std::string& name) : name(name) {};
        uint32_t charIdx = 0;
        uint32_t bitIndex = 0;
        std::string name;
        std::shared_ptr<LUTNode> left;
        std::shared_ptr<LUTNode> right;
    };

    struct WriteInfo {
        std::vector<const std::string*> strTable;
        // Exactly matches strings in string table
        std::vector<uint32_t> strIndexedIds;
    };

    void writeHeader(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeStrgSec(const WriteInfo &writeInfo);

    void writeStrTable(const std::vector<const std::string*> &names);

    void writeLUT(const WriteInfo& writeInfo);

    static std::optional<std::shared_ptr<LUTNode>> createLUT(std::vector<const std::string*>& names);

    void printTree(const std::shared_ptr<BfsarWriter::LUTNode> &node, const std::string &prefix, bool isLeft);

    uint32_t writeInfoSec(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeSoundInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    void writeSound3DInfo(const BfsarSound3D& s3d);

    void writeStreamSoundInfo(const BfsarStreamSound& stm);

    void writeWaveSoundInfo(const BfsarWaveSound& wav);

    void writeSequenceSoundInfo(const BfsarSequenceSound& seq);

    uint32_t writeSoundGroupInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeBankInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeWaveArchiveInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeGroupInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writePlayerInfo(const BfsarContext& context, const WriteInfo& writeInfo);

    uint32_t writeFileInfo(const BfsarContext &context);

    void writeSoundArchivePlayerInfo(const BfsarContext& context);

    uint32_t writeFileSec(const BfsarContext &context);

    OutMemoryStream m_Stream;
};
