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

class Directory : public DirectoryEntry {
public:
  Directory(Disk &disk, const std::string &name, std::size_t inode,
            std::stack<std::string> parent)
      : DirectoryEntry(disk, name, inode, parent), entries() {
    updateDirectoryEntries();
  }

  const std::unordered_map<std::string, DirectoryEntry> &
  updateDirectoryEntries();

  const auto &getDirectoryEntires() const { return entries; }

  virtual const InodeType getType() const noexcept override {
    return InodeType::Directory;
  }

private:
  std::unordered_map<std::string, DirectoryEntry> entries;

  virtual void parseDataBlock(std::uint32_t block) override;
};

class File : public DirectoryEntry {
public:
  File(Disk &disk, const std::string &name, std::size_t inode,
       std::stack<std::string> parent)
      : DirectoryEntry(disk, name, inode, parent) {
    updateData();
  }

  virtual const InodeType getType() const noexcept override {
    return InodeType::File;
  }

  const std::string &getData() const { return data; }

  const std::string &updateData();

private:
  virtual void parseDataBlock(std::uint32_t block) override;

  std::string data;
};

class SymbolicLink : public DirectoryEntry {
public:
  SymbolicLink(Disk &disk, const std::string &name, std::size_t inode,
               std::stack<std::string> parent)
      : DirectoryEntry(disk, name, inode, parent) {
    updateData();
  }
  virtual const InodeType getType() const noexcept override {
    return InodeType::SymbolicLink;
  }

  const std::string &getData() const { return data; }
  const std::string &updateData();

private:
  virtual void parseDataBlock(std::uint32_t block) override;
  std::string data;
};
} // namespace fsext2