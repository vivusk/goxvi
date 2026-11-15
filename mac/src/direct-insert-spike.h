#pragma once

// SPIKE (plan 261115-1819 phase 1, K9) — removed in phase 3. Measures whether
// clients honour insertText:replacementRange: when Goxvi types without marked
// text. Runtime flag, default OFF: the marked-text path stays untouched.
#import <Foundation/Foundation.h>

#include <string>

#include "composition-session.h"

// NSUserDefaults bool "GoxviSpikeDirectInsert" (absent = NO).
bool spikeDirectInsertEnabled();
void setSpikeDirectInsertEnabled(bool enabled);

// Diagnostic log lines appended under ~/Library/Application Support/Goxvi/.
// Numbers only (bundle ID, ranges, lengths, verify result) — NEVER typed text.
void spikeLogProbe(id client);                     // ev=probe, on activateServer:
void spikeLogFinish(id client, const char* src, bool composing);  // ev=finish

struct DirectInsertSpike {
  std::wstring onScreen;  // characters of the current word already in the document

  // Replays the outcome as insertText:replacementRange: (no setMarkedText).
  void apply(const goxvi_mac::KeyOutcome& outcome, id client);
  void reset() { onScreen.clear(); }
};
