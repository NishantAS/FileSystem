#pragma once

#include "Enums.hpp"
#include <stack>
#include <string>
#include <unordered_map>

namespace fsext2 {
struct Inode;

class Disk;

class DirectoryEntry {
public:
  DirectoryEntry(Disk &disk, const std::string &name, std::size_t inode,
                 std::stack<std::string> parent);

  virtual ~DirectoryEntry() {}

  virtual const InodeType getType() const noexcept {
    return InodeType::Unknown;
  }

  const InodePermissions getPermissions() const noexcept;

  bool operator==(const DirectoryEntry &other) const {
    return inode == other.inode;
  }

protected:
  Disk &disk;
  std::string name;
  std::stack<std::string> path;
  std::uint32_t inode;
  const Inode &inodeData;

  virtual void parseDataBlock(std::uint32_t) {
    throw std::exception("Please implement parsing for the inode type");
  }

  void parseSinglyIndirectDataBlock(std::uint32_t block);

  void parseDoublyIndirectDataBlock(std::uint32_t block);

  void parseTriplyIndirectDataBlock(std::uint32_t block);
};
} // namespace fsext2