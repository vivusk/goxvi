#pragma once

#include <string>

#include "goxvi/engine-types.h"

// Load/save %APPDATA%\Goxvi\config.json — the exact same file the TSF DLL polls.
// Reuses core's parseConfigJson/serializeConfigJson so the JSON schema stays a
// single source of truth (no DTO duplication). Saves are atomic (temp + rename)
// so the TIP never reads a half-written file; on first use the Goxvi directory
// is granted AppContainer read (H3). Never throws.
namespace goxvi::settings {

class ConfigStore {
 public:
  ConfigStore();

  const goxvi::EngineConfig& config() const { return config_; }
  goxvi::EngineConfig& config() { return config_; }

  // Persist the current config atomically. Returns false on I/O failure.
  bool save();

  const std::wstring& configPath() const { return configPath_; }
  // Non-empty when the AppContainer read-grant failed at startup (rare).
  const std::wstring& startupWarning() const { return startupWarning_; }

 private:
  std::wstring configDir_;       // %APPDATA%\Goxvi ("" if unresolved)
  std::wstring configPath_;      // configDir_ + \config.json
  std::wstring startupWarning_;  // surfaced by the UI when non-empty
  goxvi::EngineConfig config_;   // in-memory, edited by the pages
};

}  // namespace goxvi::settings
