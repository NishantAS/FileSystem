#pragma once

#include "Enums.hpp"
#include <concepts>
#include <stack>
#include <string>
#include <unordered_map>

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
    throw std::exception("Please implement parsing for the inode type");
  }

  void parseSinglyIndirectDataBlock(std::uint32_t block);

  void parseDoublyIndirectDataBlock(std::uint32_t block);

  void parseTriplyIndirectDataBlock(std::uint32_t block);
};

class Directory : public DirectoryEntry {
public:
  static const inline InodeType type = InodeType::Directory;

  const std::unordered_map<std::string, DirectoryEntry> &
  updateDirectoryEntries();

  const auto &getDirectoryEntires() const { return entries; }

  virtual const InodeType getType() const noexcept override {
    return InodeType::Directory;
  }

private:
  std::unordered_map<std::string, DirectoryEntry> entries;
  friend class DirectoryEntry;

  explicit Directory(const DirectoryEntry &dir);


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