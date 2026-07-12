#include "config-store.h"

#include <windows.h>

#include <shlobj.h>

#include <string>

#include "app-container-acl.h"
#include "goxvi/config-json-serializer.h"

namespace goxvi::settings {

namespace {

std::wstring roamingGoxviDir() {
  PWSTR appData = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
                                  &appData))) {
    return {};
  }
  std::wstring dir(appData);
  CoTaskMemFree(appData);
  return dir + L"\\Goxvi";
}

// Same tolerant read the TIP uses: share read/write/delete, cap size.
bool readFileUtf8(const std::wstring& path, std::string& out) {
  HANDLE file = CreateFileW(
      path.c_str(), GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size = {};
  bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 &&
            size.QuadPart <= goxvi::kMaxConfigFileBytes;  // same cap as the TIP
  if (ok) {
    out.resize(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    ok = ReadFile(file, out.data(), static_cast<DWORD>(out.size()), &read,
                  nullptr) &&
         read == out.size();
  }
  CloseHandle(file);
  return ok;
}

bool writeFileUtf8(const std::wstring& path, const std::string& data) {
  HANDLE file =
      CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                  FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  const bool ok = WriteFile(file, data.data(),
                            static_cast<DWORD>(data.size()), &written,
                            nullptr) &&
                  written == data.size();
  CloseHandle(file);
  return ok;
}

}  // namespace

ConfigStore::ConfigStore() : configDir_(roamingGoxviDir()) {
  if (configDir_.empty()) {
    startupWarning_ = L"Khong xac dinh duoc thu muc cau hinh (%APPDATA%).";
    return;
  }
  configPath_ = configDir_ + L"\\config.json";

  // Create the dir + stamp the AppContainer read grant once (H3). CreateDirectory
  // is a no-op if it already exists.
  CreateDirectoryW(configDir_.c_str(), nullptr);
  if (!grantAllAppPackagesRead(configDir_)) {
    startupWarning_ =
        L"Khong cap duoc quyen doc cau hinh cho ung dung UWP (chay quyen han che?).";
  }

  // Tolerant load: any missing/broken file yields engine defaults and is NOT
  // written back until the user actually changes something.
  std::string json;
  if (readFileUtf8(configPath_, json)) {
    config_ = goxvi::parseConfigJson(json);
  }
}

bool ConfigStore::save() {
  if (configPath_.empty()) return false;
  CreateDirectoryW(configDir_.c_str(), nullptr);  // in case it was removed

  const std::string json = goxvi::serializeConfigJson(config_);

  // Atomic: write a temp then rename over config.json so the TIP (which shares
  // the file open) never observes a partial write. One writer (single-instance
  // app) → a pid-tagged temp name is unique enough.
  wchar_t temp[MAX_PATH];
  swprintf_s(temp, L"%s\\config.%lu.tmp", configDir_.c_str(),
             GetCurrentProcessId());
  if (!writeFileUtf8(temp, json)) return false;
  if (!MoveFileExW(temp, configPath_.c_str(),
                   MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
    DeleteFileW(temp);
    return false;
  }
  return true;
}

}  // namespace goxvi::settings
