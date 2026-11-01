#include "sim/shared_memory_region.h"

#include <windows.h>

#include <cstdint>

#include "common/str_cat.h"

namespace can_bridge {

SharedMemoryRegion::~SharedMemoryRegion() { Close(); }

Status SharedMemoryRegion::OpenOrCreate(const std::string& name, size_t size) {
  Close();
  const uint64_t size64 = size;
  mapping_ =
      CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                         static_cast<DWORD>(size64 >> 32),
                         static_cast<DWORD>(size64 & 0xFFFFFFFF), name.c_str());
  if (mapping_ == nullptr) {
    return Status::Error(
        StrCat("CreateFileMapping(", name, ") failed, error ", GetLastError()));
  }
  created_ = GetLastError() != ERROR_ALREADY_EXISTS;

  view_ = MapViewOfFile(mapping_, FILE_MAP_ALL_ACCESS, 0, 0, size);
  if (view_ == nullptr) {
    const DWORD error = GetLastError();
    Close();
    return Status::Error(
        StrCat("MapViewOfFile(", name, ") failed, error ", error,
               created_ ? "" : " (existing block too small?)"));
  }

  MEMORY_BASIC_INFORMATION info{};
  if (VirtualQuery(view_, &info, sizeof(info)) == 0 || info.RegionSize < size) {
    Close();
    return Status::Error(StrCat("shared memory ", name, " is smaller than ",
                                size, " bytes: structure versions differ"));
  }
  return Status::Ok();
}

void SharedMemoryRegion::Close() {
  if (view_ != nullptr) UnmapViewOfFile(view_);
  if (mapping_ != nullptr) CloseHandle(mapping_);
  view_ = nullptr;
  mapping_ = nullptr;
  created_ = false;
}

}  // namespace can_bridge
