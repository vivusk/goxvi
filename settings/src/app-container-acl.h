#pragma once

#include <string>

// H3: the TSF TIP runs INSIDE AppContainer host processes (Windows Search,
// Start menu, Store/UWP apps). Those sandboxes can only read files whose DACL
// explicitly grants "ALL APPLICATION PACKAGES" (SID S-1-15-2-1). The settings
// app owns %APPDATA%\Goxvi and must stamp that grant so the TIP can read
// config.json from an AppContainer. Mirrors the old C# GoxviConfigStore grant.
namespace goxvi::settings {

// Grant ALL APPLICATION PACKAGES read+execute (inheritable) on `dir`.
// Idempotent: skips the write when the grant already exists. Best-effort —
// returns false on failure (caller may surface it, but config still works for
// non-sandboxed hosts). Never throws.
bool grantAllAppPackagesRead(const std::wstring& dir);

}  // namespace goxvi::settings
