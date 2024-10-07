#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace fsext2 {

enum OptionalFeatures : std::uint32_t {
  has_journal = 0x0004,
  ext_attr = 0x0008,
  resize_inode = 0x0010,
  dir_index = 0x0020,
};

const std::string to_string(OptionalFeatures feature);

enum RequiredFeatures : std::uint32_t {
  Compressed = 0x1,
  filetype = 0x2,
};

const std::string to_string(RequiredFeatures feature);

enum ReadOnlyFeatures : std::uint32_t {
  sparse_super = 0x1,
  large_file = 0x2,
  DirContentsAreBT = 0x4,
};

const std::string to_string(ReadOnlyFeatures feature);

enum class CreatorOS : std::uint32_t {
  Linux = 0,
  HURD = 1,
  MASIX = 2,
  FreeBSD = 3,
  Lites = 4
};

const std::string_view to_string(CreatorOS os);

enum class FileSystemState : std::uint16_t { clean = 1, errors };

const std::string_view to_string(FileSystemState state);

enum class ErrorHandlingMethod : std::uint16_t {
  ignore = 1,
  remount_readonly,
  panic
};

const std::string_view to_string(ErrorHandlingMethod method);

enum class InodeType : std::uint16_t {
  Unknown = 0x0,
  FIFO = 0x1,
  CharacterDevice = 0x2,
  Directory = 0x4,
  BlockDevice = 0x6,
  File = 0x8,
  SymbolicLink = 0xA,
  Socket = 0xC,
};

const std::string_view to_string(InodeType type);

enum InodePermissions : std::uint16_t {
  SetUID = 0x800,
  SetGID = 0x400,
  StickyBit = 0x200,
  OwnerRead = 0x100,
  OwnerWrite = 0x80,
  OwnerExecute = 0x40,
  GroupRead = 0x20,
  GroupWrite = 0x10,
  GroupExecute = 0x8,
  OtherRead = 0x4,
  OtherWrite = 0x2,
  OtherExecute = 0x1,
};

const std::string to_string(InodePermissions permissions);

enum InodeFlags : std::uint32_t {
  secure_deletion = 0x1,
  keep_copy = 0x2,
  compress = 0x4,
  sync_update = 0x8,
  immutable = 0x10,
  append_only = 0x20,
  no_dump = 0x40,
  no_atime = 0x80,
  hash_dir = 0x10000,
  afs_dir = 0x20000,
  journal_data = 0x40000,
};

const std::string to_string(InodeFlags flags);
} // namespace fsext2