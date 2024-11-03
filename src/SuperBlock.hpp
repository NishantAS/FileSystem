#pragma once
#include <cstdint>

namespace fsext2 {
enum class FileSystemState : std::uint16_t;
enum class ErrorHandlingMethod : std::uint16_t;
enum class CreatorOS : std::uint32_t;
enum OptionalFeatures : std::uint32_t;
enum RequiredFeatures : std::uint32_t;
enum ReadOnlyFeatures : std::uint32_t;

struct SuperBlock {
  friend class DiskIOManager;
  std::uint32_t inodeCount;
  std::uint32_t blockCount;
  std::uint32_t reservedBlockCount;
  std::uint32_t freeBlockCount;
  std::uint32_t freeInodeCount;
  std::uint32_t blockNumber;
  std::uint32_t logBlockSize;
  std::uint32_t logFragmentSize;
  std::uint32_t blocksPerGroup;
  std::uint32_t fragmentsPerGroup;
  std::uint32_t inodesPerGroup;
  std::uint32_t lastMountTime;
  std::uint32_t lastWriteTime;
  std::uint16_t mountCount;
  std::uint16_t maxMountCount;
  std::uint16_t magic;
  FileSystemState state;
  ErrorHandlingMethod errorMethod;
  std::uint16_t versionMinor;
  std::uint32_t lastCheckTime;
  std::uint32_t checkInterval;
  CreatorOS creatorOS;
  std::uint32_t versionMajor;
  std::uint16_t uid;
  std::uint16_t gid;

private:
  struct {
    std::uint32_t firstNonReservedInode;
    std::uint16_t inodeSize;
    std::uint16_t blockGroupNumber;
    OptionalFeatures optionalFeatures;
    RequiredFeatures requiredFeatures;
    ReadOnlyFeatures readOnlyFeatures;
    std::uint8_t fileSystemId[16];
    std::uint8_t volumeName[16];
    std::uint8_t lastMountPath[64];
    std::uint32_t compressionAlgorithms;
    std::uint8_t preallocatedBlocksForFiles;
    std::uint8_t preallocatedBlocksForDirectories;
    std::uint16_t unusedPadding;
    std::uint8_t journalID[16];
    std::uint32_t journalInode;
    std::uint32_t journalDevice;
    std::uint32_t orphanInodeListHead;
  } extended;
};
} // namespace fsext2