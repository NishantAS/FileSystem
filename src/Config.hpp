#pragma once

#include "Enums.hpp"
#include "SuperBlock.hpp"
#include <chrono>
#include <cstdint>

namespace fsext2 {
class Config {
public:
  std::uintmax_t blockSize;
  std::uintmax_t fragmentSize;
  std::uint16_t inodeSize = 128;
  std::uint32_t firstNonReservedInode = 11;
  std::uintmax_t groupCount;
  OptionalFeatures optionalFeatures;
  RequiredFeatures requiredFeatures;
  ReadOnlyFeatures readOnlyFeatures;
  std::string fileSystemId = "<none>";
  std::string_view volumeName;
  std::string_view lastMountPath;
  std::chrono::time_point<std::chrono::system_clock> lastMountTime;
  std::chrono::time_point<std::chrono::system_clock> lastWriteTime;
  std::chrono::time_point<std::chrono::system_clock> lastCheckTime;
  std::uint32_t inodeBlocksPerGroup;
  std::uint32_t reservedBlocksForGDT;
  std::uint32_t overHeadClusters;
  SuperBlock primarySuperBlock;
};
} // namespace fsext2