#include "DirectoryEntry.hpp"
#include "Config.hpp"
#include "Disk.hpp"
#include "DiskReader.hpp"
#include "Inode.hpp"
#include <exception>

namespace fsext2 {
enum class DirectoryEntryInodeType : std::uint8_t {
  Unknown = 0,
  File,
  Directory,
  CharacterDevice,
  BlockDevice,
  FIFO,
  Socket,
  SymbolicLink,
};

InodeType diskEntryInodeTypeToInodeType(const DirectoryEntryInodeType &type) {
  switch (type) {
  case DirectoryEntryInodeType::Unknown:
    return InodeType::Unknown;
  case DirectoryEntryInodeType::File:
    return InodeType::File;
  case DirectoryEntryInodeType::Directory:
    return InodeType::Directory;
  case DirectoryEntryInodeType::CharacterDevice:
    return InodeType::CharacterDevice;
  case DirectoryEntryInodeType::BlockDevice:
    return InodeType::BlockDevice;
  case DirectoryEntryInodeType::FIFO:
    return InodeType::FIFO;
  case DirectoryEntryInodeType::Socket:
    return InodeType::Socket;
  case DirectoryEntryInodeType::SymbolicLink:
    return InodeType::SymbolicLink;
  default:
    return InodeType(type);
  }
}
} // namespace fsext2

fsext2::DirectoryEntry::DirectoryEntry(Disk &disk, const std::string &name,
                                       std::size_t inode)
    : disk(disk), name(name), inode(inode),
      inodeData(new Inode(disk.reader->readInode(inode))) {}

fsext2::DirectoryEntry::DirectoryEntry(const DirectoryEntry &other) noexcept
    : disk(other.disk), name(other.name), inode(other.inode),
      inodeData(new Inode(*other.inodeData)) {}

fsext2::DirectoryEntry::DirectoryEntry(DirectoryEntry &&other) noexcept
    : disk(other.disk), name(other.name), inode(other.inode),
      inodeData(other.inodeData) {
  other.name = "";
  other.inode = 0;
  other.inodeData = nullptr;
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

const std::unordered_map<std::string, fsext2::DirectoryEntry> &
fsext2::Directory::updateDirectoryEntries() {
  for (const auto dataBlockPtr : inodeData->directBlockPointers) {
    if (dataBlockPtr) {
      this->parseDataBlock(dataBlockPtr);
    }
  }

  if (inodeData->singlyIndirectBlockPointer) {
    this->parseSinglyIndirectDataBlock(inodeData->singlyIndirectBlockPointer);
  }

  if (inodeData->doublyIndirectBlockPointer) {
    this->parseDoublyIndirectDataBlock(inodeData->doublyIndirectBlockPointer);
  }

  if (inodeData->triplyIndirectBlockPointer) {
    this->parseTriplyIndirectDataBlock(inodeData->triplyIndirectBlockPointer);
  }
  return entries;
}

fsext2::Directory::Directory(const DirectoryEntry &dir)
    : DirectoryEntry(dir), entries() {
  updateDirectoryEntries();
}

void fsext2::Directory::parseDataBlock(std::uint32_t block) {
  std::uint32_t entry_inode = 0;
  std::uint16_t rec_len = 0;
  std::uint8_t len = 0;
  std::uint8_t type_or_size{};
  std::size_t offset = 0;
  while (disk.config->blockSize - offset) {
    disk.reader->readBlock(block, reinterpret_cast<char *>(&entry_inode),
                           sizeof(entry_inode), offset);
    disk.reader->readBlock(block, reinterpret_cast<char *>(&rec_len),
                           sizeof(rec_len), sizeof(entry_inode) + offset);
    disk.reader->readBlock(block, reinterpret_cast<char *>(&len), sizeof(len),
                           sizeof(entry_inode) + sizeof(rec_len) + offset);
    disk.reader->readBlock(
        block, reinterpret_cast<char *>(&type_or_size), sizeof(type_or_size),
        sizeof(entry_inode) + sizeof(rec_len) + sizeof(len) + offset);
    std::string entry_name = disk.reader->readString(
        block, rec_len - 8,
        sizeof(entry_inode) + sizeof(rec_len) + sizeof(len) +
            sizeof(type_or_size) + offset);

    if (disk.config->requiredFeatures & filetype) {

      entries.emplace(entry_name,
                      DirEntryData{entry_inode,
                                   diskEntryInodeTypeToInodeType(
                                       DirectoryEntryInodeType(type_or_size))});
    } else {
      entries.emplace(entry_name, DirEntryData{entry_inode});
    }
    offset += rec_len;
  }
}

const fsext2::InodeType fsext2::DirectoryEntry::getType() const noexcept {
  return inodeData->type;
}

const fsext2::InodePermissions
fsext2::DirectoryEntry::getPermissions() const noexcept {
  return inodeData->permissions;
}

const std::string &fsext2::File::updateData() {
  for (const auto dataBlockPtr : inodeData->directBlockPointers) {
    if (dataBlockPtr) {
      this->parseDataBlock(dataBlockPtr);
    }
  }

  if (inodeData->singlyIndirectBlockPointer) {
    this->parseSinglyIndirectDataBlock(inodeData->singlyIndirectBlockPointer);
  }

  if (inodeData->doublyIndirectBlockPointer) {
    this->parseDoublyIndirectDataBlock(inodeData->doublyIndirectBlockPointer);
  }

  if (inodeData->triplyIndirectBlockPointer) {
    this->parseTriplyIndirectDataBlock(inodeData->triplyIndirectBlockPointer);
  }

  return data;
}

void fsext2::File::parseDataBlock(std::uint32_t block) {
  data += disk.reader->readString(block, disk.config->blockSize);
}

fsext2::File::File(const DirectoryEntry &file) : DirectoryEntry(file), data() {
  updateData();
}

const std::string &fsext2::SymbolicLink::updateData() {
  if (inodeData->size < 60) {
    data = std::string(
        reinterpret_cast<const char *>(&inodeData->directBlockPointers[0]),
        inodeData->size);
  } else {
    for (const auto dataBlockPtr : inodeData->directBlockPointers) {
      if (dataBlockPtr) {
        this->parseDataBlock(dataBlockPtr);
      }
    }
    if (inodeData->singlyIndirectBlockPointer) {
      this->parseSinglyIndirectDataBlock(inodeData->singlyIndirectBlockPointer);
    }
    if (inodeData->doublyIndirectBlockPointer) {
      this->parseDoublyIndirectDataBlock(inodeData->doublyIndirectBlockPointer);
    }
    if (inodeData->triplyIndirectBlockPointer) {
      this->parseTriplyIndirectDataBlock(inodeData->triplyIndirectBlockPointer);
    }
  }

  return data;
}

void fsext2::SymbolicLink::parseDataBlock(std::uint32_t block) {
  data += disk.reader->readString(block, disk.config->blockSize);
}
