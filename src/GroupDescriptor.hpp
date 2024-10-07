#pragma once

#include <cstdint>

namespace fsext2 {
struct GroupDescriptor {
  std::uint32_t blockUsageBitmapBlock;
  std::uint32_t inodeUsageBitmapBlock;
  std::uint32_t inodeTableBlock;
};
} // namespace fsext2