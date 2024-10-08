#pragma once

#include "Enums.hpp"
#include <algorithm>
#include <concepts>
#include <optional>
#include <ranges>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace fsext2 {
struct Inode;

class Disk;

class DirectoryEntry;

template <InodeType Type> struct TypeList;

template <class T>
concept DirectoryEntryType = std::derived_from<T, DirectoryEntry> &&
                             std::same_as<decltype(T::type), const InodeType> &&
                             std::same_as<typename TypeList<T::type>::type, T>;

template <class T> using type_t = decltype(T::type);

class DirectoryEntry {
public:
  DirectoryEntry(Disk &disk, const std::string &name, std::size_t inode);

  DirectoryEntry(const DirectoryEntry &other) noexcept;

  DirectoryEntry(DirectoryEntry &&other) noexcept;

  virtual ~DirectoryEntry() { delete inodeData; }

  const InodeType getType() const noexcept;

  const InodePermissions getPermissions() const noexcept;

  template <DirectoryEntryType T>
  static std::optional<T> fromEntry(const DirectoryEntry &entry) noexcept {
    if (T::type == entry.getType()) {
      return T(entry);
    } else {
      return {};
    }
  }

  bool operator==(const DirectoryEntry &other) const {
    return inode == other.inode;
  }

  const std::string &getName() const { return name; }

protected:
  Disk &disk;
  std::string name;
  std::uint32_t inode;
  const Inode *inodeData;

  virtual void parseDataBlock(std::uint32_t) {
    throw std::runtime_error("Please implement parsing for the inode type");
  }

  void parseSinglyIndirectDataBlock(std::uint32_t block);

  void parseDoublyIndirectDataBlock(std::uint32_t block);

  void parseTriplyIndirectDataBlock(std::uint32_t block);
};

class Directory : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::Directory;

  void updateDirectoryEntries();

  decltype(auto) getDirectoryEntires() const {
    std::vector<std::string> keys;
    for (auto&& key : entries | std::views::keys) {
      keys.push_back(key);
    }
    std::ranges::sort(keys);
    return keys;
  }

  const std::optional<DirectoryEntry>
  getDirectoryEntry(const std::string &name) {
    if (entries.contains(name))
      return DirectoryEntry(disk, name, entries.at(name).inode);
    else
      return {};
  }

private:
  friend class DirectoryEntry;
  struct DirEntryData {
    std::uint32_t inode;
    std::optional<InodeType> type;
  };

  explicit Directory(const DirectoryEntry &dir);

  std::unordered_map<std::string, DirEntryData> entries;

  virtual void parseDataBlock(std::uint32_t block) override;
};

class File : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::File;

  const std::string &getData() const { return data; }

  const std::string &updateData();

private:
  friend class DirectoryEntry;
  virtual void parseDataBlock(std::uint32_t block) override;

  explicit File(const DirectoryEntry &file);

  std::string data;
};

class SymbolicLink : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::SymbolicLink;

  const std::string &getData() const { return data; }
  const std::string &updateData();

private:
  friend class DirectoryEntry;
  virtual void parseDataBlock(std::uint32_t block) override;
  std::string data;

  explicit SymbolicLink(const DirectoryEntry &link);
};

template <> struct TypeList<InodeType::Directory> {
  using type = Directory;
};

template <> struct TypeList<InodeType::File> {
  using type = File;
};

template <> struct TypeList<InodeType::SymbolicLink> {
  using type = SymbolicLink;
};
} // namespace fsext2