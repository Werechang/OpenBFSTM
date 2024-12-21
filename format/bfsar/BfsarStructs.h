//
// Created by cookieso on 27.11.24.
//

#pragma once

#include <cstdint>

struct BfsarInternalFile {
    std::span<const uint8_t> data;
};

struct BfsarInternalNullFile {
    std::vector<uint32_t> groupTable;
};

struct BfsarExternalFile {
    std::string name;
};

struct BfsarFileInfo {
    std::variant<std::monostate, BfsarInternalFile, BfsarInternalNullFile, BfsarExternalFile> info = std::monostate{};

    bool isExternal() const {
        return std::holds_alternative<BfsarExternalFile>(info);
    }

    const BfsarExternalFile& getExternal() const {
        return get<BfsarExternalFile>(info);
    }

    bool isInternal() const {
        return std::holds_alternative<BfsarInternalFile>(info);
    }

    const BfsarInternalFile& getInternal() const {
        return get<BfsarInternalFile>(info);
    }

    bool isInternalNull() const {
        return std::holds_alternative<BfsarInternalNullFile>(info);
    }

    const BfsarInternalNullFile& getInternalNull() const {
        return get<BfsarInternalNullFile>(info);
    }
};

struct BfsarEmbeddedData {
    std::string name;
    uint32_t fileIndex;
};

struct BfsarPrioInfo {
    uint8_t channelPrio;
    bool isReleasePrioFix;
};

struct BfsarPan {
    uint8_t mode;
    uint8_t curve;
};

struct BfsarActorPlayer {
    uint8_t playerPriority;
    uint8_t actorPlayerId;
};

struct BfsarSinglePlay {
    uint16_t type;
    uint16_t effectiveDuration;
};

struct BfsarSound3D {
    uint32_t flags;
    float unkFloat;
    bool unkBool0;
    bool unkBool1;
};

struct BfsarTrackChannelInfo {
    uint32_t channels;
    // Might be the other way around since I didn't check lol
    uint8_t channelIndexL;
    uint8_t channelIndexR;
};

struct BfsarTrackInfo {
    uint8_t unk0;
    uint8_t unk1;
    uint8_t unk2;
    bool unk3;
    BfsarTrackChannelInfo trackChannelInfo;
    uint8_t unk4;
    uint8_t unk5;
};

struct BfsarStreamSound {
    uint16_t validTracks;
    uint16_t channelCount;
    float unkFloat;
    uint32_t unk;
    std::vector<BfsarTrackInfo> trackInfo;
};

struct BfsarWaveSound {
    uint32_t archiveId;
    uint32_t unk;
    std::optional<BfsarPrioInfo> prioInfo = std::nullopt;
};

struct BfsarSequenceSound {
    uint32_t validTracks;
    std::vector<uint32_t> bankIds{};
    std::optional<uint32_t> startOffset = std::nullopt;
    std::optional<BfsarPrioInfo> prioInfo = std::nullopt;
};

struct BfsarSound : public BfsarEmbeddedData {
    uint32_t playerId;
    uint8_t initialVolume;
    uint8_t remoteFilter;
    std::variant<BfsarStreamSound, BfsarWaveSound, BfsarSequenceSound> subInfo;
    std::optional<BfsarPan> panInfo = std::nullopt;
    std::optional<BfsarActorPlayer> actorPlayerInfo = std::nullopt;
    std::optional<BfsarSinglePlay> singlePlayInfo = std::nullopt;
    std::optional<BfsarSound3D> sound3DInfo = std::nullopt;
    std::optional<bool> isFrontBypass = std::nullopt;
};

struct BfsarSoundGroup {
    std::string name;
    std::vector<uint32_t> fileIndices;
    std::optional<std::vector<uint32_t>> waveArcIdTable = std::nullopt;
    uint32_t startId;
    uint32_t endId;
};

struct BfsarBank : public BfsarEmbeddedData {
    std::vector<uint32_t> waveArcIdTable;
};

struct BfsarWaveArchive : public BfsarEmbeddedData {
    uint32_t unk;
    std::optional<uint32_t> waveCount = std::nullopt;
};

struct BfsarGroup : public BfsarEmbeddedData {
};

struct BfsarPlayer {
    std::string name;
    uint32_t playableSoundLimit;
    std::optional<uint32_t> playerHeapSize = std::nullopt;
};

struct BfsarSoundArchivePlayer {
    uint16_t sequenceLimit;
    uint16_t sequenceTrackLimit;
    uint16_t streamLimit;
    uint16_t unkLimit;
    uint16_t streamChannelLimit;
    uint16_t waveLimit;
    uint16_t unkWaveLimit;
    uint8_t streamBufferTimes;
    bool isAdvancedWave;
};

struct BfsarStringEntry {
    int32_t offset;
    uint32_t size;
};

struct BfsarContext {
    std::vector<BfsarSound> sounds;
    std::vector<BfsarSoundGroup> soundGroups;
    std::vector<BfsarBank> banks;
    std::vector<BfsarWaveArchive> waveArchives;
    std::vector<BfsarGroup> groups;
    std::vector<BfsarPlayer> players;
    std::vector<BfsarFileInfo> fileInfo;
    // TODO Maybe not needed? (Might be calculated)
    BfsarSoundArchivePlayer sarPlayer;
};
