//
// Created by cookieso on 27.11.24.
//

#pragma once

#include <cstdint>

struct BfsarInternalFile {
    int32_t offset;
    uint32_t size;
};

struct BfsarExternalFile {
    std::string name;
};

struct BfsarFileInfo {
    std::variant<BfsarInternalFile, BfsarExternalFile> info;

    bool isExternal() const {
        return info.index() == 1;
    }

    bool isInternal() const {
        return info.index() == 0;
    }
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

struct BfsarStreamSound {
    uint16_t validTracks;
    uint16_t channelCount;
    uint32_t unk;
};

struct BfsarWaveSound {
    uint32_t archiveId;
    std::optional<BfsarPrioInfo> prioInfo;
};

struct BfsarSequenceSound {
    uint32_t validTracks;
    std::vector<uint32_t> bankIds{};
    std::optional<uint32_t> startOffset;
    std::optional<BfsarPrioInfo> prioInfo{};
};

struct BfsarSound {
    std::string name;
    BfsarFileInfo fileData;
    uint32_t playerId;
    uint8_t initialVolume;
    uint8_t remoteFilter;
    std::variant<BfsarStreamSound, BfsarWaveSound, BfsarSequenceSound> subInfo;
    std::optional<BfsarPan> panInfo;
    std::optional<BfsarActorPlayer> actorPlayerInfo;
    std::optional<BfsarSinglePlay> singlePlayInfo;
    std::optional<BfsarSound3D> sound3DInfo;
    std::optional<bool> isFrontBypass;
};

struct BfsarSoundGroup {
    std::string name;
    std::vector<BfsarFileInfo> fileData;
    uint32_t startId;
    uint32_t endId;
};

struct BfsarBank {
    std::string name;
    BfsarFileInfo fileData;
};

struct BfsarWaveArchive {
    std::string name;
    BfsarFileInfo fileData;
    std::optional<uint32_t> waveCount;
};

/**
 * If file data empty: bfgrp is external
 */
struct BfsarGroup {
    std::string name;
    std::optional<BfsarFileInfo> fileData;
};

struct BfsarPlayer {
    std::string name;
    uint32_t playableSoundLimit;
    std::optional<uint32_t> playerHeapSize;
};

struct BfsarSoundArchivePlayer {
    uint16_t sequenceLimit;
    uint16_t sequenceTrackLimit;
    uint16_t streamLimit;
    uint16_t unkLimit;
    uint16_t streamChannelLimit;
    uint16_t waveLimit;
    uint16_t unkWaveLimit;
};

struct BfsarStringEntry {
    int32_t offset;
    uint32_t size;
};

struct BfsarReadContext {
    std::vector<BfsarSound> sounds;
    std::vector<BfsarSoundGroup> soundGroups;
    std::vector<BfsarBank> banks;
    std::vector<BfsarWaveArchive> waveArchives;
    std::vector<BfsarGroup> groups;
    std::vector<BfsarPlayer> players;
    // TODO Maybe not needed? (Might be calculated)
    BfsarSoundArchivePlayer sarPlayer;
};
