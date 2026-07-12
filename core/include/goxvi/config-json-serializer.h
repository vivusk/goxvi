#pragma once

#include <string>

#include "goxvi/engine-types.h"

// EngineConfig <-> JSON (schema v1) — pure string functions, no file I/O
// (the tsf layer owns the filesystem; this stays unit-testable).
//
//   { "version": 1, "toneStyle": "old", "inputMethod": "telex",
//     "shortcutsEnabled": true,
//     "shortcuts": [ { "trigger": "trc", "expansion": "trước" } ] }
//   toneStyle: "old" | "new"; inputMethod: "telex" | "vni"
//   shortcuts: array of { trigger, expansion } (UTF-8); bad entries skipped.
//
// Parsing is tolerant: broken/empty JSON, missing fields, unknown fields or
// versions all fall back to defaults per field. Never throws.
namespace goxvi {

inline constexpr int kConfigSchemaVersion = 1;

EngineConfig parseConfigJson(const std::string& utf8Json);

std::string serializeConfigJson(const EngineConfig& config);

}  // namespace goxvi
