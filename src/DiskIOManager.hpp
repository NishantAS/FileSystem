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
class DiskIOManager {
public:
  DiskIOManager(const std::string &path);

  ~DiskIOManager();

  void readBlock(std::uint32_t block, char *buffer, std::size_t size = 0,
                 std::size_t offset = 0);

  std::uint32_t writeToContiguousBlocks(const char *buffer, std::size_t size,
                                        std::uint32_t preferredGroup = 0);

  std::vector<std::uint32_t>
  writeToNonContiguousBlocks(const char *buffer, std::size_t size,
                             std::uint32_t preferredGroup = 0);

  void updateBlock(std::uint32_t block, const char *buffer, std::size_t size,
                   std::size_t offset = 0);

  void clearBlock(std::uint32_t block);

  std::string readString(std::uint32_t block, std::uint32_t maxSize,
                         std::uint32_t offset = 0);

  Inode readInode(std::uint32_t inode);

  std::uint32_t addInode(Inode inode, std::uint32_t preferredGroup = 0);

  void updateInode(std::uint32_t inode, Inode inodeData);

  Inode addBlocksToInode(std::uint32_t inode, std::vector<std::uint32_t> blocks);

  void removeInode(std::uint32_t inode);

  const Config *const getConfig() const { return &this->config; }

  const std::vector<GroupDescriptor> &getGroupDescriptors() const {
    return this->groupDescriptors;
  }

  const std::vector<std::uint8_t> readBlockUsageBitmap(std::uint16_t group);

  const std::vector<std::uint8_t> readInodeUsageBitmap(std::uint16_t group);

private:
  std::fstream image;
  Config config;
  std::vector<GroupDescriptor> groupDescriptors;

  void readSuperBlock();

  void readGroupDescriptors();

  void writeBlock(std::uint32_t block, const char *buffer, std::size_t size,
                  std::size_t offset = 0);

  void updateDirectoriesCount(std::uint16_t group,
                              std::uint16_t directoriesUsed);

  void writeBlockUsageBitmap(std::uint16_t group,
                             const std::vector<std::uint8_t> &bitmap);

  void writeInodeUsageBitmap(std::uint16_t group,
                             const std::vector<std::uint8_t> &bitmap);
};
} // namespace fsext2
