#pragma once

#include "Enums.hpp"
#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace fsext2 {
struct Inode;

class DiskIOManager;

class DirectoryEntry;

class Directory;

class File;

// class SymbolicLink;

template <InodeType Type> struct TypeList;

template <class T>
concept DirectoryEntryType = std::derived_from<T, DirectoryEntry> &&
                             std::same_as<decltype(T::type), const InodeType> &&
                             std::same_as<typename TypeList<T::type>::type, T>;

template <class T> using type_t = decltype(T::type);

class DirectoryEntry {
public:
  virtual ~DirectoryEntry();

  DirectoryEntry(const DirectoryEntry &other) noexcept;

  DirectoryEntry(DirectoryEntry &&other) noexcept;

  const InodeType getType() const noexcept;

  const InodePermissions getPermissions() const noexcept;

  bool operator==(const DirectoryEntry &other) const {
    return inode == other.inode;
  }

  const std::string &getName() const { return entryName; }

protected:
  DirectoryEntry(DiskIOManager &reader, const std::string &name,
                 std::uint32_t inode);

  DiskIOManager &reader;
  std::string entryName;
  std::uint32_t inode;
  const Inode *inodeData;

  virtual void parseDataBlock(std::uint32_t) = 0;

  void parseSinglyIndirectDataBlock(std::uint32_t block);

  void parseDoublyIndirectDataBlock(std::uint32_t block);

  void parseTriplyIndirectDataBlock(std::uint32_t block);
};

class Directory : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::Directory;

  Directory(DiskIOManager &reader, const std::string &name,
            std::uint32_t inode);

  virtual ~Directory() noexcept = default;

  Directory(Directory &&) noexcept = default;

  Directory(const Directory &) noexcept = default;

  void updateDirectoryEntries();

  std::vector<std::string> getDirectoryEntires() const {
    std::vector<std::string> keys;
    for (auto &&key : entries | std::views::keys) {
      keys.push_back(key);
    }
    std::ranges::sort(keys);
    return keys;
  }

  std::unique_ptr<DirectoryEntry>
  getDirectoryEntry(const std::string &name) noexcept;

  std::unique_ptr<fsext2::DirectoryEntry>addDirectoryEntry(const std::string&name,
                                                  InodeType type);

private:
  struct DirEntryData {
    std::uint32_t inode;
    std::uint32_t offset;
    std::uint32_t block;
    InodeType type;
  };

  std::unordered_map<std::string, DirEntryData>::iterator last;

  std::unordered_map<std::string, DirEntryData> entries;

  virtual void parseDataBlock(std::uint32_t block) override;
};

class File : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::File;

  File(DiskIOManager &reader, const std::string &name, std::uint32_t inode);

  virtual ~File() = default;

  File(File &&) noexcept = default;

  File(const File &) noexcept = default;

  const std::string &getData() const { return data; }

  const std::string &updateData();

  const bool putData(const std::string &data);

  const bool putData(const std::vector<std::uint8_t> &data);

  const bool appendData(const std::string &data);

  const bool appendData(const std::vector<std::uint8_t> &data);

private:
  virtual void parseDataBlock(std::uint32_t block) override;

  std::string data;
};

// class SymbolicLink : public DirectoryEntry {
// public:
//   static const inline InodeType type = InodeType::SymbolicLink;
//
//   SymbolicLink(DiskIOManger &reader, const std::string &name, std::uint32_t
//   inode);
//
//   virtual ~SymbolicLink() = default;
//
//   const std::string &getTarget() const { return target; }
//   const std::string &updateTarget();
//
// private:
//   virtual void parseDataBlock(std::uint32_t block) override;
//   std::string target;
// };

template <> struct TypeList<InodeType::Directory> {
  using type = Directory;
};

template <> struct TypeList<InodeType::File> {
  using type = File;
};

// template <> struct TypeList<InodeType::SymbolicLink> {
//   using type = SymbolicLink;
// };
} // namespace fsext2