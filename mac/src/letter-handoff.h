#pragma once

#include <optional>

// TB-007: Telegram, after sending a pasted image, routes the first key to a
// transient client (no caret) that Goxvi hands the key back to; the app then
// types it into the real chat field itself, bypassing IMK. The field's own
// controller starts at the 2nd key and never saw the 1st, so "dd" gave "dd".
// The transient controller leaves the letter here (process-wide, main thread);
// the next client's first key takes it and resumes the word with it, provided
// that letter is right before its caret. Pure C++ (unit-tested).
namespace goxvi_mac {

class LetterHandoff {
 public:
  // Covers a slow 2nd key (measured 0.53 s between "d" and "d"); the real
  // guard is that the letter must sit right before the new client's caret.
  static constexpr double kMaxAgeSeconds = 3.0;

  void put(wchar_t letter, double now) {
    letter_ = letter;
    at_ = now;
  }

  // The letter if put within kMaxAgeSeconds; always clears (one shot).
  std::optional<wchar_t> take(double now) {
    const wchar_t letter = letter_;
    letter_ = 0;
    if (letter == 0 || now < at_ || now - at_ > kMaxAgeSeconds) return std::nullopt;
    return letter;
  }

 private:
  wchar_t letter_ = 0;
  double at_ = 0;
};

}  // namespace goxvi_mac
