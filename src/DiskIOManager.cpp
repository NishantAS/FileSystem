#include "DiskIOManager.hpp"
#include "pch.hpp"
#include "utils.hpp"

static std::size_t findFirstFreeOfSize(const std::vector<std::uint8_t> &bitmap,
                                       std::size_t size) {
  std::size_t count = 0;
  for (std::size_t i = 0; i < bitmap.size(); ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      if (bitmap[i] bitand (1 << j))
        count = 0;
      else {
        count++;
        if (count == size)
          return i * 8 + j;
      }
    }
  }
  return std::numeric_limits<std::size_t>::max();
}

static auto findFree(const std::vector<std::uint8_t> &bitmap,
                     std::uint32_t max) {
  std::vector<std::uint32_t> blocks;
  for (std::size_t i = 0; i < bitmap.size(); ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      if (blocks.size() == max)
        return blocks;
      if (not(bitmap[i] bitand (1 << j))) {
        blocks.push_back(i * 8 + j);
      }
    }
  }
  return blocks;
}

static std::vector<std::uint32_t>
indirectBlockPointersForBlocks(std::vector<std::uint32_t> &blocks,
                               fsext2::DiskIOManager &io) {
  auto count = 1 + (blocks.size() * sizeof(std::uint32_t) - 1) /
                       io.getConfig()->blockSize;
  std::vector<std::uint32_t> blockPointers;
  auto contingous =
      io.writeToContiguousBlocks(reinterpret_cast<const char *>(blocks.data()),
                                 blocks.size() * sizeof(std::uint32_t));
  if (contingous) {
    blockPointers.reserve(count);
    for (auto i = 0ull; i < count; ++i) {
      blockPointers.push_back(i + contingous);
    }
  } else {
    blockPointers = io.writeToNonContiguousBlocks(
        reinterpret_cast<const char *>(blocks.data()),
        blocks.size() * sizeof(std::uint32_t));
  }

  return blockPointers;
}

static void addBlocksToBlockPointer(std::uint32_t blockPointer,
                                    std::vector<std::uint32_t> &blocks,
                                    fsext2::DiskIOManager &io) {
  std::vector<std::uint32_t> blockPointers{};
  blockPointers.resize(io.getConfig()->blockSize / sizeof(std::uint32_t));
  io.readBlock(blockPointer, reinterpret_cast<char *>(blockPointers.data()),
               blockPointers.size());
  for (auto &block : blockPointers) {
    if (!block) {
      block = blocks.back();
      blocks.pop_back();
      if (!blocks.size())
        break;
    }
  }
  io.updateBlock(blockPointer, reinterpret_cast<char *>(blockPointers.data()),
                 blockPointers.size());
}

fsext2::DiskIOManager::DiskIOManager(const std::string &path)
    : image(path, std::ios::binary | std::ios::in | std::ios::out), config(),
      groupDescriptors({}) {
  if (!image.is_open())
    throw std::runtime_error("Failed to open disk image");

  this->readSuperBlock();
  this->readGroupDescriptors();
}

fsext2::DiskIOManager::~DiskIOManager() {
  if (image.is_open())
    image.close();
}

void fsext2::DiskIOManager::readBlock(std::uint32_t block, char *buffer,
                                      std::size_t size, std::size_t offset) {
  if (!size)
    size = config.blockSize;

  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.read(buffer, size);
}

std::uint32_t fsext2::DiskIOManager::writeToContiguousBlocks(
    const char *buffer, std::size_t size, std::uint32_t preferredGroup) {
  auto group = preferredGroup;
  auto numberOfBlocks = 1 + ((size - 1) / config.blockSize);
  do {
    if (groupDescriptors[group].freeBlocksCount < numberOfBlocks)
      continue;
    auto blocksBitMap = readBlockUsageBitmap(group);
    auto block = findFirstFreeOfSize(blocksBitMap, numberOfBlocks);
    if (block != std::numeric_limits<std::size_t>::max()) {
      for (int i = block; i < block + numberOfBlocks; ++i) {
        blocksBitMap[i / 8] |= (1 << (i % 8));
      }
      block += group * config.primarySuperBlock.blocksPerGroup;
      writeBlock(block, buffer, size);
      writeBlockUsageBitmap(group, blocksBitMap);
      return block;
    }
    group = (group + 1) % config.groupCount;
  } while (group != preferredGroup);
  return -1;
}

std::vector<std::uint32_t> fsext2::DiskIOManager::writeToNonContiguousBlocks(
    const char *buffer, std::size_t size, std::uint32_t preferredGroup) {
  auto group = preferredGroup;
  auto numberOfBlocks = 1 + ((size - 1) / config.blockSize);
  if (config.primarySuperBlock.freeBlockCount < numberOfBlocks) {
    return {};
  }
  std::vector<std::uint32_t> blocks;
  blocks.reserve(numberOfBlocks);
  do {
    auto blocksBitMap = readBlockUsageBitmap(group);
    auto localBlocks = findFree(blocksBitMap, numberOfBlocks - blocks.size());
    for (auto block : localBlocks) {
      blocksBitMap[block / 8] |= (1 << (block % 8));
      block += group * config.primarySuperBlock.blocksPerGroup;
      writeBlock(block, buffer, size);
      blocks.push_back(block);
      size -= config.blockSize;
      buffer += config.blockSize;
    }
    writeBlockUsageBitmap(group, blocksBitMap);
    group = (group + 1) % config.groupCount;
  } while (blocks.size() < numberOfBlocks);
  return blocks;
}

void fsext2::DiskIOManager::updateBlock(std::uint32_t block, const char *buffer,
                                        std::size_t size, std::size_t offset) {
  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.write(buffer, size);
}

void fsext2::DiskIOManager::clearBlock(std::uint32_t block) {
  auto group = block / config.primarySuperBlock.blocksPerGroup;
  auto index = block % config.primarySuperBlock.blocksPerGroup;
  auto blocksBitMap = readBlockUsageBitmap(group);
  blocksBitMap[index / 8] &= ~(1 << (index % 8));
  writeBlockUsageBitmap(group, blocksBitMap);
}

std::string fsext2::DiskIOManager::readString(std::uint32_t block,
                                              std::uint32_t maxSize,
                                              std::uint32_t offset) {
  std::stringbuf buffer;
  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.get(buffer, '\0');
  return buffer.str().substr(0, maxSize);
}

fsext2::Inode fsext2::DiskIOManager::readInode(std::uint32_t inode) {
  const auto group = (inode - 1) / config.primarySuperBlock.inodesPerGroup;
  const auto index = (inode - 1ull) % config.primarySuperBlock.inodesPerGroup;
  const auto block_offset = (index * config.inodeSize) / config.blockSize;
  const auto block = groupDescriptors[group].inodeTableBlock + block_offset;
  const auto index_offset = index % (config.blockSize / config.inodeSize);

  Inode buffer = Inode();
  readBlock(static_cast<std::uint32_t>(block),
            reinterpret_cast<char *>(&buffer), sizeof(buffer),
            index_offset * config.inodeSize);
  return buffer;
}

std::uint32_t fsext2::DiskIOManager::addInode(Inode inodeData,
                                              std::uint32_t preferredGroup) {
  auto group = preferredGroup;
  do {
    if (groupDescriptors[group].freeInodesCount < 1)
      continue;
    auto inodesBitMap = readInodeUsageBitmap(group);
    const auto index = findFirstFreeOfSize(inodesBitMap, 1);
    if (index != std::numeric_limits<std::size_t>::max()) {
      inodesBitMap[index / 8] |= (1 << (index % 8));
      const auto inode =
          group * config.primarySuperBlock.inodesPerGroup + index + 1;
      const auto block_offset = (index * config.inodeSize) / config.blockSize;
      const auto block = groupDescriptors[group].inodeTableBlock + block_offset;
      const auto index_offset = index % (config.blockSize / config.inodeSize);
      writeBlock(block, reinterpret_cast<char *>(&inodeData), sizeof(inodeData),
                 index_offset * config.inodeSize);
      writeInodeUsageBitmap(group, inodesBitMap);
      if (inodeData.type == InodeType::Directory) {
        updateDirectoriesCount(group, 1);
      }
      return inode;
    }
  } while (group != preferredGroup);
  return -1;
}

void fsext2::DiskIOManager::updateInode(std::uint32_t inode, Inode inodeData) {
  const auto group = (inode - 1) / config.primarySuperBlock.inodesPerGroup;
  const auto index = (inode - 1ull) % config.primarySuperBlock.inodesPerGroup;
  const auto block_offset = (index * config.inodeSize) / config.blockSize;
  const auto block = groupDescriptors[group].inodeTableBlock + block_offset;
  const auto index_offset = index % (config.blockSize / config.inodeSize);

  writeBlock(block, reinterpret_cast<char *>(&inodeData), sizeof(Inode),
             index_offset * config.inodeSize);
}

fsext2::Inode
fsext2::DiskIOManager::addBlocksToInode(std::uint32_t inode,
                                        std::vector<std::uint32_t> blocks) {
  auto inodeData = readInode(inode);
  std::ranges::reverse(blocks);
  for (auto &block : inodeData.directBlockPointers) {
    if (!block) {
      block = blocks.back();
      blocks.pop_back();
      if (!blocks.size())
        break;
    }
  }

  if (not blocks.empty()) {
    if (inodeData.singlyIndirectBlockPointer) {
      addBlocksToBlockPointer(inodeData.singlyIndirectBlockPointer, blocks,
                              *this);
      if (not blocks.empty()) {
        std::ranges::reverse(blocks);
        blocks = indirectBlockPointersForBlocks(blocks, *this);
        std::ranges::reverse(blocks);
      }
    } else {
      std::ranges::reverse(blocks);
      blocks = indirectBlockPointersForBlocks(blocks, *this);
      std::ranges::reverse(blocks);
      inodeData.singlyIndirectBlockPointer = blocks.back();
      blocks.pop_back();
    }
    if (not blocks.empty()) {
      if (inodeData.doublyIndirectBlockPointer) {
        addBlocksToBlockPointer(inodeData.doublyIndirectBlockPointer, blocks,
                                *this);
        if (not blocks.empty()) {
          std::ranges::reverse(blocks);
          blocks = indirectBlockPointersForBlocks(blocks, *this);
          std::ranges::reverse(blocks);
        }
      } else {
        std::ranges::reverse(blocks);
        blocks = indirectBlockPointersForBlocks(blocks, *this);
        std::ranges::reverse(blocks);
        inodeData.doublyIndirectBlockPointer = blocks.back();
        blocks.pop_back();
      }

      if (not blocks.empty()) {
        if (inodeData.triplyIndirectBlockPointer) {
          addBlocksToBlockPointer(inodeData.triplyIndirectBlockPointer, blocks,
                                  *this);
        } else {
          std::ranges::reverse(blocks);
          blocks = indirectBlockPointersForBlocks(blocks, *this);
          std::ranges::reverse(blocks);
          inodeData.triplyIndirectBlockPointer = blocks.back();
          blocks.pop_back();
        }

        if (not blocks.empty()) {
          throw;
        }
      }
    }
  }

  updateInode(inode, inodeData);
}

void fsext2::DiskIOManager::removeInode(std::uint32_t inode) {
  const auto group = (inode - 1) / config.primarySuperBlock.inodesPerGroup;
  const auto index = (inode - 1ull) % config.primarySuperBlock.inodesPerGroup;
  auto inodesBitMap = readInodeUsageBitmap(group);
  auto inodeData = readInode(inode);
  auto blocks = blocksFromInode(inodeData, *this, true);
  for (auto block : blocks) {
    clearBlock(block);
  }
  inodesBitMap[index] = false;
  writeInodeUsageBitmap(group, inodesBitMap);
  if (inodeData.type == InodeType::Directory) {
    updateDirectoriesCount(group, -1);
  }
}

const std::vector<std::uint8_t>
fsext2::DiskIOManager::readBlockUsageBitmap(std::uint16_t group) {
  std::vector<std::uint8_t> buffer{};
  if (group == config.groupCount - 1 and
      config.primarySuperBlock.blockCount %
          config.primarySuperBlock.blocksPerGroup) {
    buffer.resize((config.primarySuperBlock.blockCount %
                   config.primarySuperBlock.blocksPerGroup) /
                  8);
  } else {
    buffer.resize(config.primarySuperBlock.blocksPerGroup / 8);
  }

  readBlock(groupDescriptors[group].blockUsageBitmapBlock,
            reinterpret_cast<char *>(buffer.data()),
            buffer.size() * sizeof(decltype(buffer)::value_type));

  return buffer;
}

const std::vector<std::uint8_t>
fsext2::DiskIOManager::readInodeUsageBitmap(std::uint16_t group) {
  std::vector<std::uint8_t> buffer;
  if (group == config.groupCount - 1 and
      config.primarySuperBlock.inodeCount %
              config.primarySuperBlock.inodesPerGroup !=
          0) {
    buffer.resize((config.primarySuperBlock.inodeCount %
                   config.primarySuperBlock.inodesPerGroup) /
                  8);
  } else {
    buffer.resize(config.primarySuperBlock.inodesPerGroup / 8);
  }

  readBlock(groupDescriptors[group].inodeUsageBitmapBlock,
            reinterpret_cast<char *>(buffer.data()),
            buffer.size() * sizeof(decltype(buffer)::value_type));
  return buffer;
}

void fsext2::DiskIOManager::readSuperBlock() {
  image.seekg(1024, std::ios::beg);
  image.read(reinterpret_cast<char *>(&config.primarySuperBlock),
             sizeof(config.primarySuperBlock));

  if (config.primarySuperBlock.magic != 0xEF53)
    throw std::runtime_error("Not an ext2 filesystem");

  config.blockSize = std::uintmax_t(1024)
                     << config.primarySuperBlock.logBlockSize;
  config.fragmentSize = std::uintmax_t(1024)
                        << config.primarySuperBlock.logFragmentSize;
  config.groupCount = static_cast<std::uintmax_t>(
      std::ceil(1.f * config.primarySuperBlock.blockCount /
                config.primarySuperBlock.blocksPerGroup));
  config.lastMountTime =
      std::chrono::system_clock::duration(
          std::chrono::seconds(config.primarySuperBlock.lastMountTime)) +
      std::chrono::time_point<std::chrono::system_clock>();
  config.lastWriteTime =
      std::chrono::system_clock::duration(
          std::chrono::seconds(config.primarySuperBlock.lastWriteTime)) +
      std::chrono::time_point<std::chrono::system_clock>();
  config.lastCheckTime =
      std::chrono::system_clock::duration(
          std::chrono::seconds(config.primarySuperBlock.lastCheckTime)) +
      std::chrono::time_point<std::chrono::system_clock>();
  if (config.primarySuperBlock.versionMajor >= 1) {
    config.firstNonReservedInode =
        config.primarySuperBlock.extended.firstNonReservedInode;
    config.inodeSize = config.primarySuperBlock.extended.inodeSize;
    config.optionalFeatures =
        config.primarySuperBlock.extended.optionalFeatures;
    config.requiredFeatures =
        config.primarySuperBlock.extended.requiredFeatures;
    config.readOnlyFeatures =
        config.primarySuperBlock.extended.readOnlyFeatures;
    std::ostringstream oss{};
    oss << std::hex << int(config.primarySuperBlock.extended.fileSystemId[0])
        << int(config.primarySuperBlock.extended.fileSystemId[1])
        << int(config.primarySuperBlock.extended.fileSystemId[2])
        << int(config.primarySuperBlock.extended.fileSystemId[3]) << '-'
        << int(config.primarySuperBlock.extended.fileSystemId[4])
        << int(config.primarySuperBlock.extended.fileSystemId[5]) << '-'
        << int(config.primarySuperBlock.extended.fileSystemId[6])
        << int(config.primarySuperBlock.extended.fileSystemId[7]) << '-'
        << int(config.primarySuperBlock.extended.fileSystemId[8])
        << int(config.primarySuperBlock.extended.fileSystemId[9]) << '-'
        << int(config.primarySuperBlock.extended.fileSystemId[10])
        << int(config.primarySuperBlock.extended.fileSystemId[11])
        << int(config.primarySuperBlock.extended.fileSystemId[12])
        << int(config.primarySuperBlock.extended.fileSystemId[13])
        << int(config.primarySuperBlock.extended.fileSystemId[14])
        << int(config.primarySuperBlock.extended.fileSystemId[15]);

    config.fileSystemId = oss.str();
    config.volumeName = std::string_view(reinterpret_cast<char *>(
        &config.primarySuperBlock.extended.volumeName[0]));
    config.lastMountPath = std::string_view(reinterpret_cast<char *>(
        &config.primarySuperBlock.extended.lastMountPath[0]));
  }

  config.inodeBlocksPerGroup = static_cast<std::uint32_t>(
      (config.inodeSize * config.primarySuperBlock.inodesPerGroup) /
      config.blockSize);

  if (not(config.requiredFeatures bitand filetype))
    throw std::runtime_error("Filetype feature is required");

  if (config.blockSize != 1024) {
    config.GDTOffset = 1;
  }
}

void fsext2::DiskIOManager::readGroupDescriptors() {
  image.seekg(config.blockSize * config.GDTOffset, std::ios::beg);
  groupDescriptors.clear();
  groupDescriptors.resize(config.groupCount);
  image.read(reinterpret_cast<char *>(groupDescriptors.data()),
             groupDescriptors.size() * sizeof(GroupDescriptor));

  auto offset = image.tellg();
  config.reservedBlocksForGDT =
      static_cast<uint32_t>(groupDescriptors[0].blockUsageBitmapBlock -
                            offset / config.blockSize - config.GDTOffset + 1);
  config.overHeadClusters = static_cast<std::uint32_t>(
      (groupDescriptors[0].inodeTableBlock + config.inodeBlocksPerGroup -
       config.GDTOffset + 1) *
          config.groupCount +
      config.GDTOffset - 1);
}

void fsext2::DiskIOManager::writeBlock(std::uint32_t block, const char *buffer,
                                       std::size_t size, std::size_t offset) {
  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.write(buffer, size);
}

void fsext2::DiskIOManager::updateDirectoriesCount(
    std::uint16_t group, std::uint16_t directoriesUsed) {
  auto GDTStart = config.blockSize * config.GDTOffset * 1ui64;
  image.seekg(GDTStart + group * sizeof(GroupDescriptor), std::ios::beg);
  groupDescriptors[group].directoriesCount -= directoriesUsed;
  image.write(reinterpret_cast<char *>(&groupDescriptors[group]),
              sizeof(GroupDescriptor));
}

void fsext2::DiskIOManager::writeBlockUsageBitmap(
    std::uint16_t group, const std::vector<std::uint8_t> &bitmap) {
  auto curr = readBlockUsageBitmap(group);
  for (std::size_t i = 0; i < bitmap.size(); ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      if (bitmap[i] bitand (1 << j) and not(curr[i] bitand (1 << j))) {
        groupDescriptors[group].freeBlocksCount--;
        config.primarySuperBlock.freeBlockCount--;
      } else if (not(bitmap[i] bitand (1 << j)) and curr[i] bitand (1 << j)) {
        groupDescriptors[group].freeBlocksCount++;
        config.primarySuperBlock.freeBlockCount++;
      }
    }
  }
  writeBlock(groupDescriptors[group].blockUsageBitmapBlock,
             reinterpret_cast<const char *>(bitmap.data()), bitmap.size());
  image.seekg(1024, std::ios::beg);
  image.write(reinterpret_cast<char *>(&config.primarySuperBlock),
              sizeof(config.primarySuperBlock));
  auto GDTStart = config.blockSize * config.GDTOffset * 1ui64;
  image.seekg(GDTStart + group * config.blockSize * sizeof(GroupDescriptor),
              std::ios::beg);
  image.write(reinterpret_cast<char *>(&groupDescriptors[group]),
              sizeof(GroupDescriptor));
}

void fsext2::DiskIOManager::writeInodeUsageBitmap(
    std::uint16_t group, const std::vector<std::uint8_t> &bitmap) {
  auto curr = readInodeUsageBitmap(group);
  for (std::size_t i = 0; i < bitmap.size(); ++i) {
    for (std::size_t j = 0; j < 8; ++j) {
      if (bitmap[i] bitand (1 << j) and not(curr[i] bitand (1 << j))) {
        groupDescriptors[group].freeInodesCount--;
        config.primarySuperBlock.freeInodeCount--;
      } else if (not(bitmap[i] bitand (1 << j)) and curr[i] bitand (1 << j)) {
        groupDescriptors[group].freeInodesCount++;
        config.primarySuperBlock.freeInodeCount++;
      }
    }
  }

  writeBlock(groupDescriptors[group].inodeUsageBitmapBlock,
             reinterpret_cast<const char *>(bitmap.data()), bitmap.size());

  image.seekg(1024, std::ios::beg);
  image.write(reinterpret_cast<char *>(&config.primarySuperBlock),
              sizeof(config.primarySuperBlock));

  auto GDTStart = config.blockSize * config.GDTOffset * 1ui64;
  image.seekg(GDTStart + group * config.blockSize * sizeof(GroupDescriptor),
              std::ios::beg);
  image.write(reinterpret_cast<char *>(&groupDescriptors[group]),
              sizeof(GroupDescriptor));
}
