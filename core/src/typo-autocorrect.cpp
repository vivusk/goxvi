#include "goxvi/typo-autocorrect.h"

#include <algorithm>
#include <string>
#include <string_view>

#include "telex-transform-rules.h"
#include "typo-autocorrect-internal.h"
#include "vietnamese-syllable-parser.h"
#include "vni-transform-rules.h"

namespace goxvi {
namespace {

// Sorted lowercase English words the corrector must never rewrite (see header).
constexpr std::wstring_view kEnglishBlocklist[] = {
#include "english-blocklist.generated.inc"
};

bool isEnglishBlocked(std::wstring_view lowerRaw) {
  return std::binary_search(std::begin(kEnglishBlocklist),
                            std::end(kEnglishBlocklist), lowerRaw);
}

}  // namespace

namespace detail {

wchar_t toLowerAscii(wchar_t c) {
  return (c >= L'A' && c <= L'Z') ? static_cast<wchar_t>(c - L'A' + L'a') : c;
}

std::wstring toLowerAscii(std::wstring_view s) {
  std::wstring out;
  out.reserve(s.size());
  for (wchar_t c : s) out.push_back(toLowerAscii(c));
  return out;
}

bool isMisplaceableModifier(wchar_t lower, InputMethod method) {
  if (method == InputMethod::Vni) return lower >= L'1' && lower <= L'8';
  switch (lower) {
    case L'w': case L's': case L'f': case L'r': case L'x': case L'j':
      return true;
    default:
      return false;
  }
}

std::optional<WordState> composeStrict(std::wstring_view raw, InputMethod method) {
  WordState w;
  for (wchar_t c : raw) {
    const KeyOutcome outcome = method == InputMethod::Vni
                                   ? applyKeyToWordVni(w, c, /*strict=*/true)
                                   : applyKeyToWord(w, c, /*strict=*/true);
    if (outcome != KeyOutcome::Applied) return std::nullopt;
  }
  const int n = static_cast<int>(w.letters.size());
  const SyllableParts p = parseSyllable(w.letters.data(), n, true);
  if (!p.valid || p.nucleusLen == 0) return std::nullopt;
  return w;
}

std::optional<std::wstring> correctTypoNoBlocklist(std::wstring_view rawKeys,
                                                   const EngineConfig& config) {
  std::optional<std::wstring> fixed =
      correctMisplacedModifierNoBlocklist(rawKeys, config);
  if (!fixed) fixed = correctTransposedCodaNoBlocklist(rawKeys, config);
  return fixed;
}

}  // namespace detail

std::optional<std::wstring> autoCorrectTypo(std::wstring_view rawKeys,
                                            const EngineConfig& config) {
  std::optional<std::wstring> fixed = detail::correctTypoNoBlocklist(rawKeys, config);
  if (fixed && isEnglishBlocked(detail::toLowerAscii(rawKeys))) return std::nullopt;
  return fixed;
}

}  // namespace goxvi
