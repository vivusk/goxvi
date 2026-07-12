#include "config-file-watcher.h"

#include <shlobj.h>

#include <vector>

#include "debug-logging.h"
#include "goxvi/config-json-serializer.h"

namespace {

std::wstring resolveConfigPath() {
  PWSTR appData = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
                                  &appData))) {
    return {};
  }
  std::wstring path(appData);
  CoTaskMemFree(appData);
  return path + L"\\Goxvi\\config.json";
}

// The settings app writes temp-then-rename (atomic), so a successful read is
// always a complete document. Share read+write+delete to never block writers.
bool readFileUtf8(const std::wstring& path, std::string& out) {
  HANDLE file = CreateFileW(
      path.c_str(), GENERIC_READ,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) return false;
  LARGE_INTEGER size = {};
  // Cap MUST match the settings app's (goxvi::kMaxConfigFileBytes): a stricter
  // TIP-side cap would silently reject a config the settings app happily wrote.
  bool ok = GetFileSizeEx(file, &size) && size.QuadPart >= 0 &&
            size.QuadPart <= goxvi::kMaxConfigFileBytes;
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

}  // namespace

ConfigFileWatcher::ConfigFileWatcher() : configPath_(resolveConfigPath()) {}

bool ConfigFileWatcher::pollConfig(goxvi::EngineConfig& config) {
  if (configPath_.empty()) return false;

  const ULONGLONG now = GetTickCount64();
  if (!firstPoll_ && now - lastCheckTick_ < kThrottleMs) return false;
  lastCheckTick_ = now;

  WIN32_FILE_ATTRIBUTE_DATA attributes = {};
  const bool present =
      GetFileAttributesExW(configPath_.c_str(), GetFileExInfoStandard,
                           &attributes) != 0;
  const bool wasFirstPoll = firstPoll_;
  firstPoll_ = false;

  if (!present) {
    if (fileWasPresent_) {  // file deleted → back to defaults
      fileWasPresent_ = false;
      config = goxvi::EngineConfig{};
      GOXVI_LOG(L"config removed, using defaults");
      return true;
    }
    return false;  // never existed: engine defaults already apply
  }

  const bool changed =
      !fileWasPresent_ || wasFirstPoll ||
      CompareFileTime(&attributes.ftLastWriteTime, &lastWriteTime_) != 0;
  if (!changed) return false;

  std::string json;
  if (!readFileUtf8(configPath_, json)) return false;  // retry next poll

  fileWasPresent_ = true;
  lastWriteTime_ = attributes.ftLastWriteTime;
  config = goxvi::parseConfigJson(json);
  GOXVI_LOG(L"config reloaded: shortcuts=%zu", config.shortcuts.size());
  return true;
}
