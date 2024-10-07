#pragma once

#include <cstdint>

namespace fsext2 {
enum InodePermissions : std::uint16_t;
enum class InodeType : std::uint16_t;
enum InodeFlags : std::uint32_t;

struct Inode {
  InodePermissions permissions : 12;
  InodeType type : 4;
  std::uint16_t userID;
  std::uint32_t size;
  std::uint32_t accessTime;
  std::uint32_t creationTime;
  std::uint32_t modificationTime;
  std::uint32_t deletionTime;
  std::uint16_t groupID;
  std::uint16_t hardLinks;
  std::uint32_t sectorCount;
  InodeFlags flags;
  union {
    struct {
      std::uint32_t reserved;
    } linux;
    struct {
      std::uint32_t translator;
    } hurd;
    struct {
      std::uint32_t reserved;
    } masix;
  } osSpecificValue1;
  std::uint32_t directBlockPointers[12];
  std::uint32_t singlyIndirectBlockPointer;
  std::uint32_t doublyIndirectBlockPointer;
  std::uint32_t triplyIndirectBlockPointer;
  std::uint32_t generationNumber;
  std::uint32_t extendedAttributeBlock;
  std::uint32_t sizeUpper;
  std::uint32_t fragmentBlockAddress;
  union {
    struct {
      std::uint8_t fragmentNumber;
      std::uint8_t fragmentSize;
      std::uint16_t padding;
      std::uint16_t uidHigh;
      std::uint16_t gidHigh;
      std::uint32_t reserved;
    } linux;
    struct {
      std::uint8_t fragmentNumber;
      std::uint8_t fragmentSize;
      std::uint16_t typesPermissionsHigh;
      std::uint16_t translator;
      std::uint32_t authorUid;
    } hurd;
    struct {
      std::uint8_t fragmentNumber;
      std::uint8_t fragmentSize;
      std::uint16_t reserved1;
      std::uint16_t reserved2;
      std::uint16_t reserved3;
      std::uint32_t reserved4;
    } masix;
  } osSpecificValue2;
};
} // namespace fsext2
