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

const std::unordered_map<std::string, fsext2::DirectoryEntry> &
fsext2::Directory::updateDirectoryEntries() {
  for (const auto dataBlockPtr : inodeData.directBlockPointers) {
    if (dataBlockPtr) {
      this->parseDataBlock(dataBlockPtr);
    }
  }

  if (inodeData.singlyIndirectBlockPointer) {
    this->parseSinglyIndirectDataBlock(inodeData.singlyIndirectBlockPointer);
  }

  if (inodeData.doublyIndirectBlockPointer) {
    this->parseDoublyIndirectDataBlock(inodeData.doublyIndirectBlockPointer);
  }

  if (inodeData.triplyIndirectBlockPointer) {
    this->parseTriplyIndirectDataBlock(inodeData.triplyIndirectBlockPointer);
  }

  return entries;
}

void fsext2::Directory::parseDataBlock(std::uint32_t block) {
  std::uint32_t entry_inode = 0;
  std::uint16_t rec_len = 0;
  std::uint8_t len = 0;
  std::uint32_t type{};
  std::size_t size = disk.config->blockSize;
  while (size - rec_len) {
    disk.reader->readBlock(block, reinterpret_cast<char *>(&entry_inode),
                           sizeof(entry_inode));
    disk.reader->readBlock(block, reinterpret_cast<char *>(&rec_len),
                           sizeof(rec_len), sizeof(entry_inode));
    disk.reader->readBlock(block, reinterpret_cast<char *>(&len), sizeof(len),
                           sizeof(entry_inode) + sizeof(rec_len));
    disk.reader->readBlock(block, reinterpret_cast<char *>(&type), sizeof(type),
                           sizeof(entry_inode) + sizeof(rec_len) + sizeof(len));
    std::string entry_name =
        disk.reader->readString(block, sizeof(entry_inode) + sizeof(rec_len) +
                                           sizeof(len) + sizeof(type));
    switch (type) {
    case 1:
      entries.emplace(entry_name, File(disk, entry_name, entry_inode, path));
      break;
    case 2:
      entries.emplace(entry_name,
                      Directory(disk, entry_name, entry_inode, path));
      break;
    case 7:
      entries.emplace(entry_name,
                      SymbolicLink(disk, entry_name, entry_inode, path));
      break;
    case 0:
      [[fallthrough]];
    case 3:
      [[fallthrough]];
    case 4:
      [[fallthrough]];
    case 5:
      [[fallthrough]];
    case 6:
      throw std::runtime_error("unimplemented");
      break;
    default:
      throw std::runtime_error("unknown type");
      break;
    }
  }
}

const fsext2::InodePermissions
fsext2::DirectoryEntry::getPermissions() const noexcept {
  return inodeData.permissions;
}

const std::string &fsext2::File::updateData() {
  for (const auto dataBlockPtr : inodeData.directBlockPointers) {
    if (dataBlockPtr) {
      this->parseDataBlock(dataBlockPtr);
    }
  }

  if (inodeData.singlyIndirectBlockPointer) {
    this->parseSinglyIndirectDataBlock(inodeData.singlyIndirectBlockPointer);
  }

  if (inodeData.doublyIndirectBlockPointer) {
    this->parseDoublyIndirectDataBlock(inodeData.doublyIndirectBlockPointer);
  }

  if (inodeData.triplyIndirectBlockPointer) {
    this->parseTriplyIndirectDataBlock(inodeData.triplyIndirectBlockPointer);
  }

  return data;
}

void fsext2::File::parseDataBlock(std::uint32_t block) {
  data += disk.reader->readString(block);
}

const std::string &fsext2::SymbolicLink::updateData() {
  if (inodeData.size < 60) {
    data = std::string(
        reinterpret_cast<const char *>(&inodeData.directBlockPointers[0]),
        inodeData.size);
  } else {
    for (const auto dataBlockPtr : inodeData.directBlockPointers) {
      if (dataBlockPtr) {
        this->parseDataBlock(dataBlockPtr);
      }
    }
    if (inodeData.singlyIndirectBlockPointer) {
      this->parseSinglyIndirectDataBlock(inodeData.singlyIndirectBlockPointer);
    }
    if (inodeData.doublyIndirectBlockPointer) {
      this->parseDoublyIndirectDataBlock(inodeData.doublyIndirectBlockPointer);
    }
    if (inodeData.triplyIndirectBlockPointer) {
      this->parseTriplyIndirectDataBlock(inodeData.triplyIndirectBlockPointer);
    }
  }

  return data;
}

void fsext2::SymbolicLink::parseDataBlock(std::uint32_t block) {
  data += disk.reader->readString(block);
}
