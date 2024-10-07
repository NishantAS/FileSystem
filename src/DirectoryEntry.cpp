#include "DirectoryEntry.hpp"
#include "Config.hpp"
#include "Disk.hpp"
#include "DiskReader.hpp"
#include "Inode.hpp"
#include <exception>

fsext2::DirectoryEntry::DirectoryEntry(Disk &disk, const std::string &name,
                                       std::size_t inode,
                                       std::stack<std::string> parent_path)
    : disk(disk), name(name), inode(inode),
      inodeData(disk.reader->readInode(inode)), path(std::move(parent_path)) {
  if (inodeData.type != getType()) {
    throw std::exception("Inode type mismatch");
  }

  if (name == ".." && !path.empty()) {
    path.pop();
    this->name = path.top();
  } else if (name != ".") {
    path.push(name);
  } else {
    this->name = path.top();
  }
}

void fsext2::DirectoryEntry::parseSinglyIndirectDataBlock(
    std::uint32_t singlyIndirectBlock) {
  std::vector<std::uint32_t> blockPointers;
  blockPointers.resize(disk.config->blockSize / sizeof(std::uint32_t));
  disk.reader->readBlock(singlyIndirectBlock,
                         reinterpret_cast<char *>(blockPointers.data()),
                         blockPointers.size());
  for (const auto block : blockPointers) {
    if (block) {
      parseDataBlock(block);
    }
  }
}

void fsext2::DirectoryEntry::parseDoublyIndirectDataBlock(
    std::uint32_t doublyIndirectBlock) {
  std::vector<std::uint32_t> singlyIndirectBlockPointers;
  singlyIndirectBlockPointers.resize(disk.config->blockSize /
                                     sizeof(std::uint32_t));
  disk.reader->readBlock(
      doublyIndirectBlock,
      reinterpret_cast<char *>(singlyIndirectBlockPointers.data()),
      singlyIndirectBlockPointers.size());
  for (const auto singlyIndirectBlock : singlyIndirectBlockPointers) {
    if (singlyIndirectBlock) {
      parseSinglyIndirectDataBlock(singlyIndirectBlock);
    }
  }
}

void fsext2::DirectoryEntry::parseTriplyIndirectDataBlock(
    std::uint32_t triplyIndirectBlock) {
  std::vector<std::uint32_t> doublyIndirectBlockPointers;
  doublyIndirectBlockPointers.resize(disk.config->blockSize /
                                     sizeof(std::uint32_t));
  disk.reader->readBlock(
      triplyIndirectBlock,
      reinterpret_cast<char *>(doublyIndirectBlockPointers.data()),
      doublyIndirectBlockPointers.size());
  for (const auto doublyIndirectBlock : doublyIndirectBlockPointers) {
    if (doublyIndirectBlock) {
      parseDoublyIndirectDataBlock(doublyIndirectBlock);
    }
  }
}
