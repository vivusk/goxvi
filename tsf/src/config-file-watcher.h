#pragma once

#include <windows.h>

#include <string>

#include "goxvi/engine-types.h"

// Lazy config reload (phase 5): no threads, no IPC — the TIP lives inside a
// foreign process's apartment. Poll at word start, throttled to one mtime
// stat per 2 seconds; reload only when the file's write time changed.
class ConfigFileWatcher {
 public:
  ConfigFileWatcher();

  // Returns true and fills `config` only when a new value should be applied
  // (file appeared/changed → parsed content; file vanished → defaults).
  // Read/parse failures keep the previous config and retry next poll.
  bool pollConfig(goxvi::EngineConfig& config);

  const std::wstring& configPath() const { return configPath_; }

 private:
  std::wstring configPath_;  // %APPDATA%\Goxvi\config.json ("" if unresolved)
  ULONGLONG lastCheckTick_ = 0;
  FILETIME lastWriteTime_ = {};
  bool fileWasPresent_ = false;
  bool firstPoll_ = true;

  static constexpr ULONGLONG kThrottleMs = 2000;
};
