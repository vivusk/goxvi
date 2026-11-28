#pragma once

// Debug-only diagnostics (TB-007): numbers only — ranges, modes, key kinds,
// object identities. NEVER typed text or key characters (security, same rule
// as GOXVI_LOG on Windows). Compiled out when NDEBUG (Release).
// Read with: log stream --predicate 'subsystem == "vn.goxvi.debug"'
#ifndef NDEBUG
#import <os/log.h>
#define GOXVI_DLOG(...) \
  os_log(os_log_create("vn.goxvi.debug", "imk"), __VA_ARGS__)
#else
#define GOXVI_DLOG(...) ((void)0)
#endif
