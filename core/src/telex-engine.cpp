#include "goxvi/telex-engine.h"

#include "telex-transform-rules.h"
#include "tone-placement-rules.h"
#include "vietnamese-syllable-parser.h"
#include "vni-transform-rules.h"

namespace goxvi {

using detail::KeyOutcome;
using detail::Letter;
using detail::Tone;
using detail::WordState;

namespace {
bool isAsciiLetter(wchar_t c) {
  return (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z');
}
bool isAsciiDigit(wchar_t c) { return c >= L'0' && c <= L'9'; }

bool isAcceptedKey(wchar_t c, InputMethod method) {
  return isAsciiLetter(c) || (method == InputMethod::Vni && isAsciiDigit(c));
}

// Telex and VNI share WordState/KeyOutcome; only the key→transform mapping
// differs (letters vs digits) — see telex-transform-rules.cpp / vni-transform-rules.cpp.
KeyOutcome applyKey(WordState& w, wchar_t ch, InputMethod method) {
  return method == InputMethod::Vni ? detail::applyKeyToWordVni(w, ch)
                                    : detail::applyKeyToWord(w, ch);
}
}  // namespace

struct TelexEngine::Impl {
  enum class State { Empty, Composing, Foreign, Literal };

  EngineConfig config;
  State state = State::Empty;
  wchar_t rawKeys[kMaxRawKeys] = {};
  int rawCount = 0;
  WordState word;
  std::wstring literalBuffer;  // display buffer while in Literal state
  bool rawExact = true;  // false after Literal backspace drifts raw (M1)

  std::wstring rawString() const { return {rawKeys, rawKeys + rawCount}; }

  std::wstring renderWord() const {
    const int n = static_cast<int>(word.letters.size());
    std::wstring out;
    out.reserve(n);
    int toneIdx = -1;
    if (word.tone != Tone::None) {
      const auto parts = detail::parseSyllable(word.letters.data(), n);
      toneIdx = detail::tonePlacementIndex(word.letters.data(), n, parts,
                                           config.toneStyle);
    }
    for (int i = 0; i < n; ++i) {
      const Letter& l = word.letters[i];
      out.push_back(i == toneIdx ? detail::vowelWithTone(l.base, word.tone, l.upper)
                                 : detail::letterChar(l.base, l.upper));
    }
    return out;
  }

  void rebuildWordFromRaw() {
    word = {};
    // Prefixes of a Composing buffer can never hit Undo/Invalid (those would
    // have flipped the state when the key was first typed), so outcomes are
    // ignored here.
    for (int i = 0; i < rawCount; ++i) applyKey(word, rawKeys[i], config.inputMethod);
  }

  void clear() {
    state = State::Empty;
    rawCount = 0;
    word = {};
    literalBuffer.clear();
    rawExact = true;
  }
};

TelexEngine::TelexEngine(EngineConfig cfg) : impl_(std::make_unique<Impl>()) {
  impl_->config = cfg;
}

TelexEngine::~TelexEngine() = default;

KeyResult TelexEngine::processKey(wchar_t ch) {
  Impl& im = *impl_;
  if (!isAcceptedKey(ch, im.config.inputMethod)) return {};

  // Literal's display buffer can outgrow the raw count (undo inserts), so
  // cap on whichever is larger — both stay within kMaxRawKeys (R2-M1).
  const size_t used = im.literalBuffer.size() > static_cast<size_t>(im.rawCount)
                          ? im.literalBuffer.size()
                          : static_cast<size_t>(im.rawCount);
  if (used >= kMaxRawKeys) return {true, true, currentDisplay()};  // R2-M1

  switch (im.state) {
    case Impl::State::Literal:
      im.rawKeys[im.rawCount++] = ch;  // keep raw for Esc cancel-to-raw
      im.literalBuffer.push_back(ch);
      return {true, false, im.literalBuffer};

    case Impl::State::Foreign:
      im.rawKeys[im.rawCount++] = ch;
      return {true, false, im.rawString()};

    case Impl::State::Empty:
      im.state = Impl::State::Composing;
      [[fallthrough]];
    case Impl::State::Composing: {
      im.rawKeys[im.rawCount++] = ch;
      switch (applyKey(im.word, ch, im.config.inputMethod)) {
        case KeyOutcome::Applied:
          return {true, false, im.renderWord()};
        case KeyOutcome::UndoToLiteral:
          im.state = Impl::State::Literal;
          im.literalBuffer = im.renderWord();
          return {true, false, im.literalBuffer};
        case KeyOutcome::Invalid:
          im.state = Impl::State::Foreign;  // display reverts to raw (H1)
          return {true, false, im.rawString()};
      }
      return {};  // unreachable
    }
  }
  return {};
}

bool TelexEngine::processBackspace(std::wstring& newDisplay) {
  Impl& im = *impl_;
  switch (im.state) {
    case Impl::State::Empty:
      return false;

    case Impl::State::Foreign: {  // pop 1:1 on the raw buffer
      --im.rawCount;
      if (im.rawCount == 0) {
        im.clear();
        newDisplay.clear();
        return true;
      }
      // Deleting the offending key(s) should resume Vietnamese typing: if the
      // remaining raw buffer replays cleanly, drop back to Composing and show
      // the transformed word (nhuwg + BS → như). Any Invalid/Undo along the
      // replay keeps Foreign (display stays raw).
      WordState replay;
      bool fullyVietnamese = true;
      for (int i = 0; i < im.rawCount; ++i) {
        if (applyKey(replay, im.rawKeys[i], im.config.inputMethod) != KeyOutcome::Applied) {
          fullyVietnamese = false;
          break;
        }
      }
      if (fullyVietnamese) {
        im.word = std::move(replay);
        im.state = Impl::State::Composing;
        newDisplay = im.renderWord();
      } else {
        newDisplay = im.rawString();
      }
      return true;
    }

    case Impl::State::Literal:  // pop display buffer, never recompute (M3)
      im.literalBuffer.pop_back();
      if (im.rawCount > 0) --im.rawCount;  // best-effort raw tracking for Esc
      im.rawExact = false;  // raw may now drift from display (M1) → no gõ tắt
      newDisplay = im.literalBuffer;
      if (im.literalBuffer.empty()) im.clear();
      return true;

    case Impl::State::Composing: {
      const size_t target = im.renderWord().size() - 1;
      while (im.rawCount > 0) {
        --im.rawCount;
        im.rebuildWordFromRaw();
        if (im.renderWord().size() <= target) break;
      }
      if (im.rawCount == 0) {
        im.clear();
        newDisplay.clear();
      } else {
        newDisplay = im.renderWord();
      }
      return true;
    }
  }
  return false;
}

std::wstring TelexEngine::currentDisplay() const {
  const Impl& im = *impl_;
  switch (im.state) {
    case Impl::State::Empty: return {};
    case Impl::State::Composing: return im.renderWord();
    case Impl::State::Foreign: return im.rawString();
    case Impl::State::Literal: return im.literalBuffer;
  }
  return {};
}

std::wstring TelexEngine::rawTypedKeys() const { return impl_->rawString(); }

bool TelexEngine::rawKeysExact() const { return impl_->rawExact; }

void TelexEngine::reset() { impl_->clear(); }

void TelexEngine::setConfig(EngineConfig cfg) { impl_->config = cfg; }

}  // namespace goxvi
