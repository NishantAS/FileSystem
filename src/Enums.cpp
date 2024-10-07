#include "Enums.hpp"

#include <string>

const std::string fsext2::to_string(OptionalFeatures feature) {
  std::string result{};
  if (feature & OptionalFeatures::has_journal)
    result += "has_journal ";
  if (feature & OptionalFeatures::ext_attr)
    result += "ext_attr ";
  if (feature & OptionalFeatures::resize_inode)
    result += "resize_inode ";
  if (feature & OptionalFeatures::dir_index)
    result += "dir_index";

  if (*(result.end() - 1) == ' ')
    result.pop_back();

  return result;
}

const std::string fsext2::to_string(RequiredFeatures feature) {
  std::string result{};
  if (feature & RequiredFeatures::Compressed)
    result += "Compressed ";
  if (feature & RequiredFeatures::filetype)
    result += "filetype";

  if (*(result.end() - 1) == ' ')
    result.pop_back();

  return result;
}

const std::string fsext2::to_string(ReadOnlyFeatures feature) {
  std::string result{};
  if (feature & ReadOnlyFeatures::sparse_super)
    result += "sparse_super ";
  if (feature & ReadOnlyFeatures::large_file)
    result += "large_file ";
  if (feature & ReadOnlyFeatures::DirContentsAreBT)
    result += "DirContentsAreBT";

  if (*(result.end() - 1) == ' ')
    result.pop_back();

  return result;
}

const std::string_view fsext2::to_string(CreatorOS os) {
  switch (os) {
  case fsext2::CreatorOS::Linux:
    return "Linux";
  case fsext2::CreatorOS::HURD:
    return "HURD";
  case fsext2::CreatorOS::MASIX:
    return "MASIX";
  case fsext2::CreatorOS::FreeBSD:
    return "FreeBSD";
  case fsext2::CreatorOS::Lites:
    return "Lites";
  default:
    return "Unknown";
  }
}

const std::string_view fsext2::to_string(FileSystemState state) {
  switch (state) {
  case fsext2::FileSystemState::clean:
    return "clean";
  case fsext2::FileSystemState::errors:
    return "errors";
  default:
    return "Unknown";
  }
}

const std::string_view fsext2::to_string(ErrorHandlingMethod method) {
  switch (method) {
  case fsext2::ErrorHandlingMethod::ignore:
    return "ignore";
  case fsext2::ErrorHandlingMethod::remount_readonly:
    return "remount_readonly";
  case fsext2::ErrorHandlingMethod::panic:
    return "panic";
  default:
    return "Unknown";
  }
}

const std::string_view fsext2::to_string(InodeType type) {
  switch (type) {
  case fsext2::InodeType::Unknown:
    return "Unknown";
  case fsext2::InodeType::FIFO:
    return "FIFO";
  case fsext2::InodeType::CharacterDevice:
    return "CharacterDevice";
  case fsext2::InodeType::Directory:
    return "Directory";
  case fsext2::InodeType::BlockDevice:
    return "BlockDevice";
  case fsext2::InodeType::File:
    return "File";
  case fsext2::InodeType::SymbolicLink:
    return "SymbolicLink";
  case fsext2::InodeType::Socket:
    return "Socket";
  default:
    return "Unknown";
  }
}

const std::string fsext2::to_string(InodePermissions permissions) {
  std::string result{};
  result.reserve(9);
  if (permissions & InodePermissions::OwnerRead)
    result += "r";
  else
    result += "-";

  if (permissions & InodePermissions::OwnerWrite)
    result += "w";
  else
    result += "-";

  if (permissions & InodePermissions::OwnerExecute)
    result += "x";
  else
    result += "-";

  if (permissions & InodePermissions::GroupRead)
    result += "r";
  else
    result += "-";

  if (permissions & InodePermissions::GroupWrite)
    result += "w";
  else
    result += "-";

  if (permissions & InodePermissions::GroupExecute)
    result += "x";
  else
    result += "-";

  if (permissions & InodePermissions::OwnerRead)
    result += "r";
  else
    result += "-";

  if (permissions & InodePermissions::OwnerWrite)
    result += "w";
  else
    result += "-";

  if (permissions & InodePermissions::OwnerExecute)
    result += "x";
  else
    result += "-";

  return result;
}

const std::string fsext2::to_string(InodeFlags flags) {
  std::string result{};
  if (flags & InodeFlags::secure_deletion)
    result += "secure_deletion ";
  if (flags & InodeFlags::keep_copy)
    result += "keep_copy ";
  if (flags & InodeFlags::compress)
    result += "compress ";
  if (flags & InodeFlags::sync_update)
    result += "sync_update ";
  if (flags & InodeFlags::immutable)
    result += "immutable ";
  if (flags & InodeFlags::append_only)
    result += "append_only ";
  if (flags & InodeFlags::no_dump)
    result += "no_dump ";
  if (flags & InodeFlags::no_atime)
    result += "no_atime ";
  if (flags & InodeFlags::hash_dir)
    result += "hash_dir ";
  if (flags & InodeFlags::afs_dir)
    result += "afs_dir ";
  if (flags & InodeFlags::journal_data)
    result += "journal_data";

  if (*(result.end() - 1) == ' ')
    result.pop_back();

  return result;
}
