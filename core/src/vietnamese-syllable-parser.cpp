#include "vietnamese-syllable-parser.h"

#include <string>
#include <string_view>

namespace goxvi::detail {
namespace {

using std::wstring_view;

// Complete onsets — required once the nucleus starts. Empty onset is valid.
constexpr wstring_view kFullOnsets[] = {
    L"b", L"c", L"ch", L"d", L"đ" /*đ*/, L"g", L"gh", L"gi", L"h", L"k",
    L"kh", L"l", L"m", L"n", L"ng", L"ngh", L"nh", L"p", L"ph", L"qu", L"r",
    L"s", L"t", L"th", L"tr", L"v", L"x"};

// Maximal nuclei (marked forms + raw typeable intermediates like "ie", "uo",
// "ưo", "uu" that later modifier keys turn into real nuclei). A nucleus is
// prefix-valid iff it is a prefix of one of these.
constexpr wstring_view kMaxNuclei[] = {
    L"ai", L"ao", L"au", L"ay",
    L"âu" /*âu*/, L"ây" /*ây*/, L"ă" /*ă*/,
    L"eo", L"êu" /*êu*/,
    L"ia", L"iêu" /*iêu*/, L"ieu", L"iu",
    L"oai", L"oay", L"oă" /*oă*/, L"oeo", L"oi",
    L"ôi" /*ôi*/, L"ơi" /*ơi*/, L"oo",
    L"ua", L"uây" /*uây*/, L"uê" /*uê*/, L"ui",
    L"ưi" /*ưi*/, L"uôi" /*uôi*/, L"uơ" /*uơ*/,
    L"ưo" /*ưo raw (nguwowif)*/, L"ươi" /*ươi*/,
    L"ươu" /*ươu*/, L"uou", L"uu", L"ưu" /*ưu*/,
    L"uya", L"uyê" /*uyê*/, L"uye", L"uyu", L"ưa" /*ưa*/,
    L"yêu" /*yêu*/, L"yeu"};

// Codas — every prefix of a valid coda is itself in this set.
constexpr wstring_view kCodas[] = {L"c",  L"ch", L"m",  L"n",
                                   L"ng", L"nh", L"p",  L"t"};

bool isFullOnset(wstring_view s) {
  if (s.empty()) return true;
  for (auto o : kFullOnsets)
    if (s == o) return true;
  return false;
}

bool isPrefixOnset(wstring_view s) {
  if (isFullOnset(s)) return true;
  return s == L"q";  // only non-full prefix (of "qu")
}

bool isPrefixNucleus(wstring_view s) {
  for (auto n : kMaxNuclei)
    if (s.size() <= n.size() && n.compare(0, s.size(), s) == 0) return true;
  return false;
}

bool isCoda(wstring_view s) {
  for (auto c : kCodas)
    if (s == c) return true;
  return false;
}

}  // namespace

bool isVowelBase(wchar_t b) {
  switch (b) {
    case L'a': case L'e': case L'i': case L'o': case L'u': case L'y':
    case kABreve: case kACirc: case kECirc: case kOCirc: case kOHorn:
    case kUHorn:
      return true;
    default:
      return false;
  }
}

bool isMarkedVowel(wchar_t b) {
  switch (b) {
    case kABreve: case kACirc: case kECirc: case kOCirc: case kOHorn:
    case kUHorn:
      return true;
    default:
      return false;
  }
}

SyllableParts parseSyllable(const Letter* letters, int count, bool strict) {
  SyllableParts p;
  std::wstring s;
  s.reserve(count);
  for (int i = 0; i < count; ++i) s.push_back(letters[i].base);
  const wstring_view sv = s;

  // Onset. Specials: "qu" (u belongs to the onset) and "gi" (i belongs to the
  // onset only when another vowel follows — otherwise i is the nucleus: gì,
  // gìn, gỉ — M2).
  int on = 0;
  if (count >= 3 && s[0] == L'g' && s[1] == L'i' && isVowelBase(s[2])) {
    on = 2;
  } else if (count >= 2 && s[0] == L'q' && s[1] == L'u') {
    on = 2;
  } else {
    while (on < count && !isVowelBase(s[on])) ++on;
  }

  p.onsetLen = on;
  if (on == count) {  // all-consonant word so far
    p.valid = !strict || isPrefixOnset(sv.substr(0, on));
    return p;
  }
  // A nucleus has started. Strict: the onset must be a complete Vietnamese
  // onset. Relaxed: any leading consonants are accepted so the tone/diacritic
  // still lands on the vowel (w, z, f... as onset — was → wá).
  if (strict && !isFullOnset(sv.substr(0, on))) return p;

  // Nucleus: run of vowels.
  int nu = on;
  while (nu < count && isVowelBase(s[nu])) ++nu;
  p.nucleusLen = nu - on;
  if (p.nucleusLen == 0) {  // consonant right after onset (e.g. "qun")
    p.valid = !strict;      // relaxed: accept structurally (no vowel to mark)
    return p;
  }
  if (strict && !isPrefixNucleus(sv.substr(on, p.nucleusLen))) return p;

  // Coda: everything after the nucleus. Strict rejects a vowel after the coda
  // starts or a non-coda consonant cluster; relaxed folds the tail into the
  // coda and accepts it.
  if (strict) {
    for (int i = nu; i < count; ++i)
      if (isVowelBase(s[i])) return p;  // vowel after coda started
  }
  p.codaLen = count - nu;
  if (strict && p.codaLen > 0 && !isCoda(sv.substr(nu))) return p;

  p.valid = true;
  return p;
}

}  // namespace goxvi::detail
