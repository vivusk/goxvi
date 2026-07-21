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
// strict = spell-check on (revert bad syllables to raw). strict=false lets
// tones/diacritics apply to any syllable ("gõ dấu tự do").
KeyOutcome applyKey(WordState& w, wchar_t ch, InputMethod method, bool strict) {
  return method == InputMethod::Vni ? detail::applyKeyToWordVni(w, ch, strict)
                                    : detail::applyKeyToWord(w, ch, strict);
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
  // Sticky per-word: set when an ALL-UPPERCASE word (acronym/viết tắt như ĐAL,
  // ĐL, PLÔ) hits a strict Invalid. Such words are typed deliberately, so we
  // suppress the spell-check raw-restore and compose them RELAXED instead —
  // relaxed never yields Invalid and keeps diacritics (PLOO → PLÔ, not "PLOO").
  // Reset by clear() at word end. Rule: restore chỉ áp dụng khi từ có chữ
  // thường; từ toàn hoa giữ nguyên dạng có dấu.
  bool acronymRelaxed = false;

  std::wstring rawString() const { return {rawKeys, rawKeys + rawCount}; }

  // Strict spell-check unless this word is an acronym we chose to relax.
  bool effectiveStrict() const {
    return config.restoreOnInvalid && !acronymRelaxed;
  }

  // Gate for the acronym no-restore path. All must hold on the word as built
  // strictly at the Invalid point:
  //   * ≥2 letters (see below);
  //   * every letter is uppercase (đ/ô count via their case flag) — the acronym
  //     signal; a single lowercase letter keeps the normal raw-restore;
  //   * it carries a Vietnamese diacritic letter (đ ă â ê ô ơ ư), OR it has no
  //     vowel yet.
  // The diacritic clause is what the user wants preserved (ĐAL, ĐL). The
  // no-vowel clause covers acronyms still building their onset (PL… before the
  // oo→ô lands, so PLÔ survives) and consonant runs (ĐHQG). A tone-only, plain
  // acronym (URL → Ủ+L, USB → Ú+B) has a vowel but no diacritic → still reverts
  // to raw, so English acronyms keep the UniKey convention.
  // ≥2 letters: a lone uppercase consonant that goes Invalid is a foreign opener
  // (w/j/f/z) or a capitalized word start ("Windows") — not an acronym; keep it
  // raw so it never gets relaxed and mangled.
  bool shouldRelaxAcronym() const {
    if (word.letters.size() < 2) return false;
    bool hasVowel = false, hasDiacritic = false;
    for (const auto& l : word.letters) {
      if (!l.upper) return false;
      if (detail::isVowelBase(l.base)) hasVowel = true;
      if (l.base == detail::kDStroke || detail::isMarkedVowel(l.base))
        hasDiacritic = true;
    }
    return hasDiacritic || !hasVowel;
  }

  std::wstring renderWord() const {
    // Same strictness as composing so tone placement matches the parse that
    // built the word (relaxed: nucleus of a free-typed syllable — zaajy → zậy).
    return detail::renderWord(word, config.toneStyle, effectiveStrict());
  }

  void rebuildWordFromRaw() {
    word = {};
    // Prefixes of a Composing buffer can never hit Undo/Invalid (those would
    // have flipped the state when the key was first typed), so outcomes are
    // ignored here.
    for (int i = 0; i < rawCount; ++i)
      applyKey(word, rawKeys[i], config.inputMethod, effectiveStrict());
  }

  void clear() {
    state = State::Empty;
    rawCount = 0;
    word = {};
    literalBuffer.clear();
    rawExact = true;
    acronymRelaxed = false;
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
      switch (applyKey(im.word, ch, im.config.inputMethod,
                       im.effectiveStrict())) {
        case KeyOutcome::Applied:
          return {true, false, im.renderWord()};
        case KeyOutcome::UndoToLiteral:
          im.state = Impl::State::Literal;
          // Undo phục hồi ĐÚNG chuỗi phím theo thứ tự user đã gõ (raw, bỏ
          // phím undo vừa nuốt) — KHÔNG lấy renderWord(): rules khai triển
          // transform tại chỗ (â → "aa" ngay cạnh nguyên âm) nên sai thứ tự
          // khi modifier gõ cách quãng sau coda (d,a,t,a → "dât", undo phải
          // ra "data" chứ không phải "daat"). Với phím gõ liền kề (aaa, ddd,
          // cass...) hai cách cho cùng kết quả.
          im.literalBuffer.assign(im.rawKeys, im.rawKeys + im.rawCount - 1);
          return {true, false, im.literalBuffer};
        case KeyOutcome::Invalid:
          // Only reachable under strict (relaxed parse never yields Invalid).
          // All-uppercase words are acronyms the user typed on purpose (ĐAL, ĐL,
          // PLÔ): don't revert to raw — flip this word to relaxed and recompose
          // so the diacritics stick and later keys keep transforming (PLOO→PLÔ).
          if (im.shouldRelaxAcronym()) {
            im.acronymRelaxed = true;
            im.rebuildWordFromRaw();  // now relaxed → never Invalid, keeps state
            return {true, false, im.renderWord()};
          }
          // Otherwise revert the display to the raw keystrokes typed (H1).
          im.state = Impl::State::Foreign;
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
        if (applyKey(replay, im.rawKeys[i], im.config.inputMethod,
                     im.config.restoreOnInvalid) != KeyOutcome::Applied) {
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
