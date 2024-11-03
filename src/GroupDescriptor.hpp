#pragma once

#include <cstdint>

namespace fsext2 {
struct alignas(32) GroupDescriptor {
  std::uint32_t blockUsageBitmapBlock;
  std::uint32_t inodeUsageBitmapBlock;
  std::uint32_t inodeTableBlock;
  std::uint16_t freeBlocksCount;
  std::uint16_t freeInodesCount;
  std::uint16_t directoriesCount;
  std::uint16_t padding;
  std::uint32_t reserved[3];
};
} // namespace fsext2