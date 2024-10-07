#pragma once

#include "Config.hpp"
#include "Disk.hpp"
#include "GroupDescriptor.hpp"
#include "Inode.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fsext2 {
class DiskReader {
public:
  DiskReader(const std::string &path);

  ~DiskReader();

  void readBlock(std::uint32_t block, char *buffer, std::size_t size = 0,
                 std::size_t offset = 0);

  std::string readString(std::uint32_t block, std::uint32_t maxSize,
                         std::uint32_t offset = 0);

  Inode readInode(std::uint32_t inode);

  const Config *const getConfig() const { return &this->config; }

  const std::vector<GroupDescriptor> &getGroupDescriptors() const {
    return this->groupDescriptors;
  }

  const std::uint16_t readFreeBlockCount(std::uint16_t group);

  const std::uint16_t readFreeInodeCount(std::uint16_t group);

  const std::uint16_t readDirectoriesCount(std::uint16_t group);

  const std::vector<bool> readBlockUsageBitmap(std::uint16_t group);

  const std::vector<bool> readInodeUsageBitmap(std::uint16_t group);

private:
  std::ifstream image;
  Config config;
  std::uint32_t GDToffset;
  std::vector<GroupDescriptor> groupDescriptors;

  void readSuperBlock();

  void readGroupDescriptors();
};
} // namespace fsext2

inline fsext2::DiskReader::DiskReader(const std::string &path)
    : image(path, std::ios::binary | std::ios::in), config(), GDToffset(2),
      groupDescriptors({}) {
  if (!image.is_open())
    throw std::exception("Failed to open disk image");

  this->readSuperBlock();
  this->readGroupDescriptors();
}

inline fsext2::DiskReader::~DiskReader() {
  if (image.is_open())
    image.close();
}

inline void fsext2::DiskReader::readBlock(std::uint32_t block, char *buffer,
                                          std::size_t size,
                                          std::size_t offset) {
  if (!size)
    size = config.blockSize;

  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.read(buffer, size);
}

inline std::string fsext2::DiskReader::readString(std::uint32_t block,
                                                  std::uint32_t maxSize,
                                                  std::uint32_t offset) {
  std::stringbuf buffer;
  image.seekg(block * config.blockSize + offset, std::ios::beg);
  image.get(buffer, '\0');
  return buffer.str().substr(0, maxSize);
}

inline fsext2::Inode fsext2::DiskReader::readInode(std::uint32_t inode) {
  const auto group_number =
      (inode - 1) / config.primarySuperBlock.inodesPerGroup;
  const auto index = (inode - 1) % config.primarySuperBlock.inodesPerGroup;
  const auto block_offset =
      (static_cast<unsigned long long>(index) * config.inodeSize) /
      config.blockSize;
  const auto block =
      groupDescriptors[group_number].inodeTableBlock + block_offset;
  const auto offset = index % (config.blockSize / config.inodeSize);

  Inode buffer = Inode();
  readBlock(block, reinterpret_cast<char *>(&buffer), sizeof(buffer),
            offset * config.inodeSize);
  return buffer;
}

inline const std::uint16_t
fsext2::DiskReader::readFreeBlockCount(std::uint16_t group) {
  std::uint16_t count{};
  readBlock(GDToffset, reinterpret_cast<char *>(&count), sizeof(count),
            12 + group * 32);
  return count;
}

inline const std::uint16_t
fsext2::DiskReader::readFreeInodeCount(std::uint16_t group) {
  std::uint16_t count{};
  readBlock(GDToffset, reinterpret_cast<char *>(&count), sizeof(count),
            14 + group * 32);
  return count;
}

inline const std::uint16_t
fsext2::DiskReader::readDirectoriesCount(std::uint16_t group) {
  std::uint16_t count{};
  readBlock(GDToffset, reinterpret_cast<char *>(&count), sizeof(count),
            16 + group * 32);
  return count;
}

inline const std::vector<bool>
fsext2::DiskReader::readBlockUsageBitmap(std::uint16_t group) {
  std::vector<bool> ret{};
  std::vector<std::uint8_t> buffer{};
  if (group == config.groupCount - 1 &&
      config.primarySuperBlock.blockCount %
          config.primarySuperBlock.blocksPerGroup) {
    ret.reserve(config.primarySuperBlock.blockCount %
                config.primarySuperBlock.blocksPerGroup);
    buffer.resize((config.primarySuperBlock.blockCount %
                   config.primarySuperBlock.blocksPerGroup) /
                  8);
  } else {
    ret.reserve(config.primarySuperBlock.blocksPerGroup);
    buffer.resize(config.primarySuperBlock.blocksPerGroup / 8);
  }

  readBlock(groupDescriptors[group].blockUsageBitmapBlock,
            reinterpret_cast<char *>(buffer.data()),
            buffer.size() * sizeof(decltype(buffer)::value_type));

  for (auto val : buffer) {
    for (std::size_t i = 0; i < 8; ++i) {
      ret.push_back(val & (1 << i));
    }
  }

  return ret;
}

inline const std::vector<bool>
fsext2::DiskReader::readInodeUsageBitmap(std::uint16_t group) {
  std::vector<bool> ret{};
  std::vector<std::uint8_t> buffer;
  if (group == config.groupCount - 1 &&
      config.primarySuperBlock.inodeCount %
              config.primarySuperBlock.inodesPerGroup !=
          0) {
    ret.reserve(config.primarySuperBlock.inodeCount %
                config.primarySuperBlock.inodesPerGroup);
    buffer.resize((config.primarySuperBlock.inodeCount %
                   config.primarySuperBlock.inodesPerGroup) /
                  8);
  } else {
    ret.reserve(config.primarySuperBlock.inodesPerGroup);
    buffer.resize(config.primarySuperBlock.inodesPerGroup / 8);
  }

  readBlock(groupDescriptors[group].inodeUsageBitmapBlock,
            reinterpret_cast<char *>(buffer.data()),
            buffer.size() * sizeof(decltype(buffer)::value_type));

  for (auto val : buffer) {
    for (std::size_t i = 0; i < 8; ++i) {
      ret.push_back(val & (1 << i));
    }
  }
  return ret;
}

inline void fsext2::DiskReader::readSuperBlock() {
  image.seekg(1024, std::ios::beg);
  image.read(reinterpret_cast<char *>(&config.primarySuperBlock),
             sizeof(config.primarySuperBlock));
  image.seekg(1024 - sizeof(config.primarySuperBlock), std::ios::cur);

  if (config.primarySuperBlock.magic != 0xEF53)
    throw std::exception("Not an ext2 filesystem");

  config.blockSize = std::uintmax_t(1024)
                     << config.primarySuperBlock.logBlockSize;
  config.fragmentSize = std::uintmax_t(1024)
                        << config.primarySuperBlock.logFragmentSize;
  config.groupCount = std::ceil(1.f * config.primarySuperBlock.blockCount /
                                config.primarySuperBlock.blocksPerGroup);
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

  config.inodeBlocksPerGroup =
      (config.inodeSize * config.primarySuperBlock.inodesPerGroup) /
      config.blockSize;
}

inline void fsext2::DiskReader::readGroupDescriptors() {
  if (config.blockSize != 1024) {
    image.seekg(config.blockSize - 2048, std::ios::cur);
    GDToffset = 1;
  }
  groupDescriptors.reserve(config.groupCount);

  for (std::size_t i = 0; i < config.groupCount; ++i) {
    GroupDescriptor groupDescriptor{};
    image.read(reinterpret_cast<char *>(&groupDescriptor),
               sizeof(groupDescriptor));
    groupDescriptors.push_back(groupDescriptor);
    image.seekg(20, std::ios::cur);
  }
  auto offset = image.tellg();
  config.reservedBlocksForGDT = groupDescriptors[0].blockUsageBitmapBlock -
                                offset / config.blockSize - GDToffset + 1;
  config.overHeadClusters = (groupDescriptors[0].inodeTableBlock +
                             config.inodeBlocksPerGroup - GDToffset + 1) *
                                config.groupCount +
                            GDToffset - 1;
}
