#pragma once

#include <string>

#include "goxvi/engine-types.h"

// Process-wide config holder of Goxvi.app (PDR D8, plan K5). Read once at
// startup from ~/Library/Application Support/Goxvi/config.json; the settings
// window (phase 5) will update it in memory. Main thread only (IMK + AppKit).
class ConfigStore {
 public:
  static ConfigStore& instance();

  // Missing, oversized (> kMaxConfigFileBytes) or broken file -> defaults.
  void load();

  goxvi::EngineConfig current() const { return config_; }

  // Absolute UTF-8 path of config.json.
  std::string path() const;

 private:
  ConfigStore() = default;
  goxvi::EngineConfig config_;
};
