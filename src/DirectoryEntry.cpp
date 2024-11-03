#include "DirectoryEntry.hpp"
#include "Config.hpp"
#include "Disk.hpp"
#include "DiskIOManager.hpp"
#include "Inode.hpp"
#include "pch.hpp"

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

InodeType dirEntryInodeTypeToInodeType(const DirectoryEntryInodeType &type) {
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

DirectoryEntryInodeType inodeTypeToDirEntryInodeType(const InodeType &type) {
  switch (type) {
  case InodeType::Unknown:
    return DirectoryEntryInodeType::Unknown;
  case InodeType::File:
    return DirectoryEntryInodeType::File;
  case InodeType::Directory:
    return DirectoryEntryInodeType::Directory;
  case InodeType::CharacterDevice:
    return DirectoryEntryInodeType::CharacterDevice;
  case InodeType::BlockDevice:
    return DirectoryEntryInodeType::BlockDevice;
  case InodeType::FIFO:
    return DirectoryEntryInodeType::FIFO;
  case InodeType::Socket:
    return DirectoryEntryInodeType::Socket;
  case InodeType::SymbolicLink:
    return DirectoryEntryInodeType::SymbolicLink;
  default:
    break;
  }
}
} // namespace fsext2

fsext2::DirectoryEntry::DirectoryEntry(DiskIOManager &reader,
                                       const std::string &name,
                                       std::uint32_t inode)
    : reader(reader), entryName(name), inode(inode),
      inodeData(new Inode(reader.readInode(inode))) {}

fsext2::DirectoryEntry::~DirectoryEntry() {
  if (inodeData)
    delete inodeData;
}

fsext2::DirectoryEntry::DirectoryEntry(const DirectoryEntry &other) noexcept
    : reader(other.reader), entryName(other.entryName), inode(other.inode),
      inodeData(new Inode(*other.inodeData)) {}

fsext2::DirectoryEntry::DirectoryEntry(DirectoryEntry &&other) noexcept
    : reader(other.reader), entryName(std::move(other.entryName)),
      inode(other.inode), inodeData(other.inodeData) {
  other.entryName = "";
  other.inode = 0;
  other.inodeData = nullptr;
}

void fsext2::DirectoryEntry::parseSinglyIndirectDataBlock(
    std::uint32_t singlyIndirectBlock) {
  std::vector<std::uint32_t> blockPointers;
  blockPointers.resize(reader.getConfig()->blockSize / sizeof(std::uint32_t));
  reader.readBlock(singlyIndirectBlock,
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
  singlyIndirectBlockPointers.resize(reader.getConfig()->blockSize /
                                     sizeof(std::uint32_t));
  reader.readBlock(doublyIndirectBlock,
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
  doublyIndirectBlockPointers.resize(reader.getConfig()->blockSize /
                                     sizeof(std::uint32_t));
  reader.readBlock(triplyIndirectBlock,
                   reinterpret_cast<char *>(doublyIndirectBlockPointers.data()),
                   doublyIndirectBlockPointers.size());
  for (const auto doublyIndirectBlock : doublyIndirectBlockPointers) {
    if (doublyIndirectBlock) {
      parseDoublyIndirectDataBlock(doublyIndirectBlock);
    }
  }
}

fsext2::Directory::Directory(DiskIOManager &reader, const std::string &name,
                             std::uint32_t inode)
    : DirectoryEntry(reader, name, inode) {
  if (this->inodeData->type != InodeType::Directory) {
    throw std::runtime_error("Inode is not a directory");
  } else {
    updateDirectoryEntries();
  }
}

void fsext2::Directory::updateDirectoryEntries() {
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

std::unique_ptr<fsext2::DirectoryEntry>
fsext2::Directory::getDirectoryEntry(const std::string &name) noexcept {
  if (entries.contains(name))
    switch (entries[name].type) {
    case InodeType::Directory:
      return std::unique_ptr<Directory>(
          new Directory(reader, name, entries[name].inode));
    case InodeType::File:
      return std::unique_ptr<File>(new File(reader, name, entries[name].inode));
    /*case InodeType::SymbolicLink:
      return std::unique_ptr<SymbolicLink>(
          new SymbolicLink(reader, name, entries[name].inode));*/
    default:
      return {};
    }
  else
    return {};
}

std::unique_ptr<fsext2::DirectoryEntry>
fsext2::Directory::addDirectoryEntry(const std::string &name, InodeType type) {
  if (entries.contains(name)) {
    return nullptr;
  }
  struct DirEntry {
    std::uint32_t inode;
    std::uint16_t size;
    std::uint8_t nameLength;
    DirectoryEntryInodeType type;
  };
  Inode inodeData{};
  std::uint32_t inode;
  switch (type) {
  case fsext2::InodeType::Unknown:
    break;
  case fsext2::InodeType::FIFO:
    break;
  case fsext2::InodeType::CharacterDevice:
    break;
  case fsext2::InodeType::Directory: {
    auto block = reader.writeToContiguousBlocks(".", 1);
    inodeData.permissions =
        InodePermissions(OwnerRead | OwnerWrite | OwnerExecute | GroupRead |
                         GroupExecute | OtherRead | OtherExecute);
    inodeData.type = InodeType::Directory;
    inodeData.size = reader.getConfig()->blockSize;
    inodeData.accessTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.creationTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.modificationTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.hardLinks = 1;
    inodeData.sectorCount = 2;
    inodeData.directBlockPointers[0] = block;
    inode = reader.addInode(inodeData);
    DirEntry curr{}, prev{};
    curr.inode = inode;
    curr.size = 12;
    curr.nameLength = 1;
    curr.type = DirectoryEntryInodeType::Directory;

    prev.inode = this->inode;
    prev.size = reader.getConfig()->blockSize - 12;
    prev.nameLength = 2;
    prev.type = DirectoryEntryInodeType::Directory;

    reader.updateBlock(block, reinterpret_cast<char *>(&curr),
                       sizeof(DirEntry));
    reader.updateBlock(block, ".", 1, 8);
    reader.updateBlock(block, reinterpret_cast<char *>(&prev), sizeof(DirEntry),
                       12);
    reader.updateBlock(block, "..", 2, 20);
    break;
  }
  case fsext2::InodeType::BlockDevice:
    break;
  case fsext2::InodeType::File: {
    inodeData.permissions =
        InodePermissions(OwnerRead | OwnerWrite | GroupRead | OtherRead);
    inodeData.type = InodeType::File;
    inodeData.size = 0;
    inodeData.accessTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.creationTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.modificationTime =
        std::chrono::system_clock::now().time_since_epoch().count();
    inodeData.hardLinks = 1;
    inodeData.sectorCount = 2;
    inode = reader.addInode(inodeData);
    break;
  }
  case fsext2::InodeType::SymbolicLink:
    break;
  case fsext2::InodeType::Socket:
    break;
  default:
    return nullptr;
  }

  if (inodeData.creationTime != 0) {
    auto lastSize = 4ull * (1 + (last->first.size() - 1) / 4) + 8;
    auto currSize = 4ull * (1 + (name.size() - 1) / 4) + 8;
    if (last->second.offset + lastSize + currSize >
        reader.getConfig()->blockSize) {
      DirEntry curr{};
      curr.inode = inode;
      curr.nameLength = name.length();
      curr.size = reader.getConfig()->blockSize;
      curr.type = inodeTypeToDirEntryInodeType(type);

      auto block =
          reader.writeToContiguousBlocks(reinterpret_cast<char *>(&curr), 8);
      reader.updateBlock(block, name.data(), name.length(), 8);
      auto [it, success] =
          entries.emplace(name, DirEntryData{inode, 8, block, type});

      last = it;
      reader.addBlocksToInode(this->inode, {block});
    } else {
      DirEntry curr{}, last{};
      last.inode = this->last->second.inode;
      last.nameLength = this->last->first.length();
      last.size = lastSize;
      last.type = inodeTypeToDirEntryInodeType(this->last->second.type);

      curr.inode = inode;
      curr.nameLength = name.length();
      curr.size =
          reader.getConfig()->blockSize - this->last->second.offset - last.size;
      curr.type = inodeTypeToDirEntryInodeType(type);

      reader.updateBlock(this->last->second.block,
                         reinterpret_cast<char *>(&last), sizeof(last),
                         this->last->second.offset);
      reader.updateBlock(this->last->second.block, this->last->first.data(),
                         last.nameLength, this->last->second.offset + 8ull);
      reader.updateBlock(this->last->second.block,
                         reinterpret_cast<char *>(&curr), sizeof(curr),
                         this->last->second.offset + last.size);
      reader.updateBlock(this->last->second.block, name.data(), curr.nameLength,
                         this->last->second.offset + 8ull + last.size);

      auto [it, success] = entries.emplace(
          name, DirEntryData{inode, this->last->second.offset + last.size,
                             this->last->second.block, type});
      this->last = it;
    }
  }

  switch (type) {
  case fsext2::InodeType::Unknown:
    break;
  case fsext2::InodeType::FIFO:
    break;
  case fsext2::InodeType::CharacterDevice:
    break;
  case fsext2::InodeType::Directory:
    return std::unique_ptr<Directory>(new Directory(reader, name, inode));
  case fsext2::InodeType::BlockDevice:
    break;
  case fsext2::InodeType::File:
    break;
  case fsext2::InodeType::SymbolicLink:
    break;
  case fsext2::InodeType::Socket:
    break;
  default:
    break;
  }
  return nullptr;
}

void fsext2::Directory::parseDataBlock(std::uint32_t block) {
  std::uint32_t entry_inode = 0;
  std::uint16_t rec_len = 0;
  std::uint8_t len = 0;
  std::uint8_t type_or_size{};
  std::uint32_t offset = 0;
  while (reader.getConfig()->blockSize - offset) {
    reader.readBlock(block, reinterpret_cast<char *>(&entry_inode),
                     sizeof(entry_inode), offset);
    reader.readBlock(block, reinterpret_cast<char *>(&rec_len), sizeof(rec_len),
                     sizeof(entry_inode) + offset);
    reader.readBlock(block, reinterpret_cast<char *>(&len), sizeof(len),
                     sizeof(entry_inode) + sizeof(rec_len) + offset);
    reader.readBlock(
        block, reinterpret_cast<char *>(&type_or_size), sizeof(type_or_size),
        sizeof(entry_inode) + sizeof(rec_len) + sizeof(len) + offset);
    std::string entry_name =
        reader.readString(block, rec_len - 8,
                          sizeof(entry_inode) + sizeof(rec_len) + sizeof(len) +
                              sizeof(type_or_size) + offset);

    auto [it, success] = entries.emplace(
        entry_name, DirEntryData{entry_inode, offset, block,
                                 dirEntryInodeTypeToInodeType(
                                     DirectoryEntryInodeType(type_or_size))});

    offset += rec_len;
    last = it;
  }
}

const fsext2::InodeType fsext2::DirectoryEntry::getType() const noexcept {
  return inodeData->type;
}

const fsext2::InodePermissions
fsext2::DirectoryEntry::getPermissions() const noexcept {
  return inodeData->permissions;
}

fsext2::File::File(DiskIOManager &reader, const std::string &name,
                   std::uint32_t inode)
    : DirectoryEntry(reader, name, inode) {
  if (this->inodeData->type != InodeType::File) {
    throw std::runtime_error("Inode is not a file");
  } else {
    updateData();
  }
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

const bool fsext2::File::putData(const std::string &data) {
  auto count = 1 + (data.size() - 1) / reader.getConfig()->blockSize;
  std::vector<std::uint32_t> blocks;
  const auto x = reader.writeToContiguousBlocks(data.data(), data.size());
  if (x) {
    blocks.reserve(count);
    for (std::size_t i = x; i - x < count; i++) {
      blocks.push_back(i);
    }
  } else {
    blocks = reader.writeToNonContiguousBlocks(data.data(), data.size());
  }
  if (blocks.empty())
    return false;
  reader.addBlocksToInode(this->inode, blocks);
  return true;
}

const bool fsext2::File::putData(const std::vector<std::uint8_t> &data) {
  auto count = 1 + (data.size() - 1) / reader.getConfig()->blockSize;
  std::vector<std::uint32_t> blocks;
  const auto x = reader.writeToContiguousBlocks(
      reinterpret_cast<const char *>(data.data()), data.size());
  if (x) {
    blocks.reserve(count);
    for (std::size_t i = x; i - x < count; i++) {
      blocks.push_back(i);
    }
  } else {
    blocks = reader.writeToNonContiguousBlocks(
        reinterpret_cast<const char *>(data.data()), data.size());
  }
  if (blocks.empty())
    return false;
  reader.addBlocksToInode(this->inode, blocks);
  return true;
}

void fsext2::File::parseDataBlock(std::uint32_t block) {
  data += reader.readString(block, reader.getConfig()->blockSize);
}

// fsext2::SymbolicLink::SymbolicLink(DiskIOManager &reader, const std::string
// &name,
//                                    std::uint32_t inode)
//     : DirectoryEntry(reader, name, inode) {
//   if (this->inodeData->type != InodeType::SymbolicLink) {
//     throw std::runtime_error("Inode is not a symbolic link");
//   } else {
//     updateTarget();
//   }
// }

// const std::string &fsext2::SymbolicLink::updateTarget() {
//   if (inodeData->size < 60) {
//     target = std::string(
//         reinterpret_cast<const char *>(&inodeData->directBlockPointers[0]),
//         inodeData->size);
//   } else {
//     for (const auto dataBlockPtr : inodeData->directBlockPointers) {
//       if (dataBlockPtr) {
//         this->parseDataBlock(dataBlockPtr);
//       }
//     }
//     if (inodeData->singlyIndirectBlockPointer) {
//       this->parseSinglyIndirectDataBlock(inodeData->singlyIndirectBlockPointer);
//     }
//     if (inodeData->doublyIndirectBlockPointer) {
//       this->parseDoublyIndirectDataBlock(inodeData->doublyIndirectBlockPointer);
//     }
//     if (inodeData->triplyIndirectBlockPointer) {
//       this->parseTriplyIndirectDataBlock(inodeData->triplyIndirectBlockPointer);
//     }
//   }
//
//   return target;
// }
//
// void fsext2::SymbolicLink::parseDataBlock(std::uint32_t block) {
//   target += reader.readString(block, reader.getConfig()->blockSize);
// }

constexpr auto s = std::is_move_constructible_v<fsext2::Directory>;