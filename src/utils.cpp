#include "utils.hpp"
#include "pch.hpp"

namespace fsext2 {
void blocksFromBlockPointer(std::uint32_t singlyIndirectBlockPointer,
                            DiskIOManager &io,
                            std::vector<std::uint32_t> &ret) {
  std::vector<std::uint32_t> blockPointers{};
  blockPointers.resize(io.getConfig()->blockSize / sizeof(std::uint32_t));
  io.readBlock(singlyIndirectBlockPointer,
               reinterpret_cast<char *>(blockPointers.data()),
               blockPointers.size());
  for (const auto block : blockPointers) {
    if (block) {
      ret.push_back(block);
    }
  }
}

std::vector<std::uint32_t> blocksFromInode(Inode inode, DiskIOManager &io,
                                           bool includeBlockPointers) {
  std::vector<std::uint32_t> ret{};
  for (const auto dataBlockPtr : inode.directBlockPointers) {
    if (dataBlockPtr) {
      ret.push_back(dataBlockPtr);
    }
  }

  if (inode.singlyIndirectBlockPointer) {
    if (includeBlockPointers) {
      ret.push_back(inode.singlyIndirectBlockPointer);
    }
    blocksFromBlockPointer(inode.singlyIndirectBlockPointer, io, ret);
  }

  if (inode.doublyIndirectBlockPointer) {
    if (includeBlockPointers) {
      ret.push_back(inode.doublyIndirectBlockPointer);
    }
    std::vector<std::uint32_t> singlyIndirectBlockPointers{};
    blocksFromBlockPointer(inode.doublyIndirectBlockPointer, io,
                           singlyIndirectBlockPointers);
    for (const auto singlyIndirectBlockPointer : singlyIndirectBlockPointers) {
      if (includeBlockPointers) {
        ret.push_back(singlyIndirectBlockPointer);
      }
      blocksFromBlockPointer(singlyIndirectBlockPointer, io, ret);
    }
  }

  if (inode.triplyIndirectBlockPointer) {
    if (includeBlockPointers) {
      ret.push_back(inode.triplyIndirectBlockPointer);
    }
    std::vector<std::uint32_t> doublyIndirectBlockPointers{};
    blocksFromBlockPointer(inode.doublyIndirectBlockPointer, io,
                           doublyIndirectBlockPointers);
    for (const auto doublyIndirectBlockPointer : doublyIndirectBlockPointers) {
      if (includeBlockPointers) {
        ret.push_back(doublyIndirectBlockPointer);
      }
      std::vector<std::uint32_t> singlyIndirectBlockPointers{};
      blocksFromBlockPointer(doublyIndirectBlockPointer, io,
                             singlyIndirectBlockPointers);
      for (const auto singlyIndirectBlockPointer :
           singlyIndirectBlockPointers) {
        if (includeBlockPointers) {
          ret.push_back(singlyIndirectBlockPointer);
        }
        blocksFromBlockPointer(singlyIndirectBlockPointer, io, ret);
      }
    }
  }

  return ret;
}
} // namespace fsext2