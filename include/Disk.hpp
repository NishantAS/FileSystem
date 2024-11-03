#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

#include "DirectoryEntry.hpp"
#include "Naviagtor.hpp"

namespace fsext2 {
class DiskIOManager;

class Config;

class Disk {
  friend class fsext2::DirectoryEntry;
  friend class fsext2::Directory;
  friend class fsext2::File;
  //friend class fsext2::SymbolicLink;

public:
  Disk(const std::string &disk_path);

  ~Disk() noexcept;

  std::string dumpe2fs() noexcept;

  Navigator getNavigator() noexcept;

private:
  DiskIOManager *reader;
  const Config *config;
};
} // namespace fsext2