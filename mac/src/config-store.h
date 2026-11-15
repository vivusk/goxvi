#pragma once

#include <cstdint>
#include <string>

#include "goxvi/engine-types.h"

// Process-wide config holder of Goxvi.app (PDR D8, plan K5). Read once at
// startup from ~/Library/Application Support/Goxvi/config.json; the input menu
// (phase 4) and settings window (phase 5) change it via update(). No file
// watcher. Main thread only (IMK + AppKit).
class ConfigStore {
 public:
  static ConfigStore& instance();

  // Missing, oversized (> kMaxConfigFileBytes) or broken file -> defaults.
  void load();

  const goxvi::EngineConfig& current() const { return config_; }

  // Bumped by every update(); controllers compare it to pick up the new config
  // at the start of their next word.
  std::uint64_t revision() const { return revision_; }

  // Applies `config` in memory (always) and writes config.json atomically
  // (temp-then-rename), creating the folder. Returns false if the write failed.
  bool update(goxvi::EngineConfig config);

  // Absolute UTF-8 path of config.json.
  std::string path() const;

 private:
  ConfigStore() = default;
  goxvi::EngineConfig config_;
  std::uint64_t revision_ = 0;
};
