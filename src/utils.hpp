#pragma once
#include "DiskIOManager.hpp"
#include "Inode.hpp"

namespace fsext2 {

void blocksFromBlockPointer(std::uint32_t singlyIndirectBlockPointer,
                            DiskIOManager &io, std::vector<std::uint32_t> &ret);

std::vector<std::uint32_t> blocksFromInode(Inode inode, DiskIOManager &io,
                                           bool includeBlockPointers = false);
} // namespace fsext2