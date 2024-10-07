#include <bitset>
#include <cassert>
#include <chrono>
#include <format>
#include <queue>
#include <ranges>
#include <sstream>
#include <string_view>
#include <vector>

#include "Config.hpp"
#include "Disk.hpp"
#include "DiskReader.hpp"
#include "Enums.hpp"

fsext2::Disk::Disk(const std::string &filename)
    : reader(nullptr), config(nullptr) {
  std::filesystem::path path(filename);
  if (!std::filesystem::exists(path)) {
    throw std::runtime_error("File does not exist");
  }
  reader = new DiskReader(filename);
  config = reader->getConfig();
}

fsext2::Disk::~Disk() {
  if (reader)
    delete reader;
}

std::string fsext2::Disk::dumpe2fs() noexcept {
  std::ostringstream stream{};

  stream << "Filesystem volume name: ";
  if (config->volumeName.empty())
    stream << "<none>";
  else
    stream << config->volumeName;

  stream << "\nLast mounted on: ";
  if (config->lastMountPath.empty())
    stream << "<none>";
  else
    stream << config->lastMountPath;

  stream << "\nFilesystem UUID: " << config->fileSystemId
         << "\nFileSystem Magic Number: 0x" << std::hex
         << config->primarySuperBlock.magic << std::dec
         << "\nFilesystem revision #: "
         << config->primarySuperBlock.versionMajor
         << "\nFilesystem features: " << to_string(config->optionalFeatures)
         << " " << to_string(config->requiredFeatures) << " "
         << to_string(config->readOnlyFeatures)
         << "\nFilesystem state: " << to_string(config->primarySuperBlock.state)
         << "\nErrors behavior: "
         << to_string(config->primarySuperBlock.errorMethod)
         << "\nFilesystem OS type: "
         << to_string(config->primarySuperBlock.creatorOS)
         << "\nInode count: " << config->primarySuperBlock.inodeCount
         << "\nBlock count: " << config->primarySuperBlock.blockCount
         << "\nReserved block count: "
         << config->primarySuperBlock.reservedBlockCount
         << "\nOverhead clusters: " << config->overHeadClusters
         << "\nFree blocks: " << config->primarySuperBlock.freeBlockCount
         << "\nFree inodes: " << config->primarySuperBlock.freeInodeCount
         << "\nFirst block: " << config->primarySuperBlock.blockNumber
         << "\nBlock size: " << config->blockSize
         << "\nFragment size: " << config->fragmentSize
         << "\nReserved GDT blocks: " << config->reservedBlocksForGDT
         << "\nBlocks per group: " << config->primarySuperBlock.blocksPerGroup
         << "\nFragments per group: "
         << config->primarySuperBlock.fragmentsPerGroup
         << "\nInodes per group: " << config->primarySuperBlock.inodesPerGroup
         << "\nInode blocks per group: " << config->inodeBlocksPerGroup
         << "\nLast mount time: "
         << std::format("{0:%a} {0:%b} {0:%d} {0:%T} {0:%Y}",
                        config->lastMountTime)
         << "\nLast write time: "
         << std::format("{0:%a} {0:%b} {0:%d} {0:%T} {0:%Y}",
                        config->lastWriteTime)
         << "\nMount count: " << config->primarySuperBlock.mountCount
         << "\nMaximum mount count: " << config->primarySuperBlock.maxMountCount
         << "\nLast checked: "
         << std::format("{0:%a} {0:%b} {0:%d} {0:%T} {0:%Y}",
                        config->lastCheckTime)
         << "\nCheck interval: " << config->primarySuperBlock.checkInterval
         << "\nReserved blocks uid: " << config->primarySuperBlock.uid
         << "\nReserved blocks gid: " << config->primarySuperBlock.gid
         << "\nFirst inode: " << config->firstNonReservedInode
         << "\nInode size: " << config->inodeSize << "\n\n\n";

  for (auto const [i, groupDescriptor] :
       reader->getGroupDescriptors() | std::views::enumerate) {
    auto startingBlock = i * config->primarySuperBlock.blocksPerGroup +
                         config->primarySuperBlock.blockNumber;
    auto freeBlocksCount = reader->readFreeBlockCount(i);
    auto freeInodesCount = reader->readFreeInodeCount(i);
    auto directoriesCount = reader->readDirectoriesCount(i);
    auto findBits = [](std::vector<bool> const &vec,
                       std::uint32_t const initialBlock) -> std::string {
      std::uint32_t begin{}, end{};
      std::ostringstream oss{};
      while (begin < vec.size()) {
        while (begin < vec.size() && vec[begin])
          ++begin;
        end = begin;
        while (end < vec.size() && !vec[end])
          ++end;
        if (begin < vec.size()) {
          oss << begin + initialBlock;
          if (end - 1 != begin) {
            oss << '-' << end - 1 + initialBlock << ", ";
          } else {
            oss << ", ";
          }
        }
        begin = end;
      }
      auto str = oss.str();
      if (str.ends_with(", "))
        str.erase(str.end() - 2, str.end());
      if (str.empty())
        return "None";
      else
        return std::move(str);
    };

    std::string_view superblockType = i == 0 ? "Primary" : "Backup";
    std::string freeBlocks =
        findBits(reader->readBlockUsageBitmap(i), startingBlock);
    std::string freeInodes =
        findBits(reader->readInodeUsageBitmap(i),
                 config->primarySuperBlock.inodesPerGroup + 1);

    stream << std::format(
        R"(Group {}: (Blocks {}-{})
	{} superblock at {}, Group descriptors at {}-{}
	Reserved GDT blocks at {}-{}
	Block bitmap at {} (+{})
	Inode bitmap at {} (+{})
	Inode table at {}-{} (+{})
	{} free blocks, {} free inodes, {} directories
	Free blocks: {}
	Free inodes: {}
)",
        i, startingBlock,
        std::min(
            static_cast<std::uintmax_t>(
                startingBlock + config->primarySuperBlock.blocksPerGroup - 1),
            static_cast<std::uintmax_t>(config->primarySuperBlock.blockCount)),
        superblockType, startingBlock, startingBlock + 1, startingBlock + 1,
        startingBlock + 2, groupDescriptor.blockUsageBitmapBlock - 1,
        groupDescriptor.blockUsageBitmapBlock,
        groupDescriptor.blockUsageBitmapBlock - startingBlock,
        groupDescriptor.inodeUsageBitmapBlock,
        groupDescriptor.inodeUsageBitmapBlock - startingBlock,
        groupDescriptor.inodeTableBlock,
        groupDescriptor.inodeTableBlock + config->inodeBlocksPerGroup - 1,
        groupDescriptor.inodeTableBlock - startingBlock, freeBlocksCount,
        freeInodesCount, directoriesCount, freeBlocks, freeInodes);
  }

  return stream.str();
}

fsext2::Directory fsext2::Disk::getRoot() noexcept {
  return Directory(*this, "", 2, std::stack<std::string>());
}

std::optional<fsext2::DirectoryEntry>
fsext2::Disk::getEntry(const std::string &path) noexcept {
  if (path[0] != '/')
    return {};

  auto parts = std::stack<DirectoryEntry>();
  parts.push(getRoot());
  std::string part{};
  for (auto c : path | std::views::drop(1)) {
    if (c == '/') {
      auto top = &parts.top();
      if (top->getType() == InodeType::Directory) {
        auto &&entries = dynamic_cast<Directory *>(top)->getDirectoryEntires();
        if (auto it = entries.find(part); it != entries.end()) {
          parts.push(it->second);
        } else {
          return {};
        }
      }
      part.clear();
    } else {
      part += c;
    }
  }

  return parts.top();
}