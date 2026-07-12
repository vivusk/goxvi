#include "tone-placement-rules.h"

namespace goxvi::detail {
namespace {

// Rows: base vowel; columns: None, Acute, Grave, Hook, Tilde, Dot.
struct ToneRow {
  wchar_t base;
  wchar_t lower[6];
  wchar_t upper[6];
};

constexpr ToneRow kToneTable[] = {
    {L'a',
     {0x0061, 0x00E1, 0x00E0, 0x1EA3, 0x00E3, 0x1EA1},
     {0x0041, 0x00C1, 0x00C0, 0x1EA2, 0x00C3, 0x1EA0}},
    {kABreve,
     {0x0103, 0x1EAF, 0x1EB1, 0x1EB3, 0x1EB5, 0x1EB7},
     {0x0102, 0x1EAE, 0x1EB0, 0x1EB2, 0x1EB4, 0x1EB6}},
    {kACirc,
     {0x00E2, 0x1EA5, 0x1EA7, 0x1EA9, 0x1EAB, 0x1EAD},
     {0x00C2, 0x1EA4, 0x1EA6, 0x1EA8, 0x1EAA, 0x1EAC}},
    {L'e',
     {0x0065, 0x00E9, 0x00E8, 0x1EBB, 0x1EBD, 0x1EB9},
     {0x0045, 0x00C9, 0x00C8, 0x1EBA, 0x1EBC, 0x1EB8}},
    {kECirc,
     {0x00EA, 0x1EBF, 0x1EC1, 0x1EC3, 0x1EC5, 0x1EC7},
     {0x00CA, 0x1EBE, 0x1EC0, 0x1EC2, 0x1EC4, 0x1EC6}},
    {L'i',
     {0x0069, 0x00ED, 0x00EC, 0x1EC9, 0x0129, 0x1ECB},
     {0x0049, 0x00CD, 0x00CC, 0x1EC8, 0x0128, 0x1ECA}},
    {L'o',
     {0x006F, 0x00F3, 0x00F2, 0x1ECF, 0x00F5, 0x1ECD},
     {0x004F, 0x00D3, 0x00D2, 0x1ECE, 0x00D5, 0x1ECC}},
    {kOCirc,
     {0x00F4, 0x1ED1, 0x1ED3, 0x1ED5, 0x1ED7, 0x1ED9},
     {0x00D4, 0x1ED0, 0x1ED2, 0x1ED4, 0x1ED6, 0x1ED8}},
    {kOHorn,
     {0x01A1, 0x1EDB, 0x1EDD, 0x1EDF, 0x1EE1, 0x1EE3},
     {0x01A0, 0x1EDA, 0x1EDC, 0x1EDE, 0x1EE0, 0x1EE2}},
    {L'u',
     {0x0075, 0x00FA, 0x00F9, 0x1EE7, 0x0169, 0x1EE5},
     {0x0055, 0x00DA, 0x00D9, 0x1EE6, 0x0168, 0x1EE4}},
    {kUHorn,
     {0x01B0, 0x1EE9, 0x1EEB, 0x1EED, 0x1EEF, 0x1EF1},
     {0x01AF, 0x1EE8, 0x1EEA, 0x1EEC, 0x1EEE, 0x1EF0}},
    {L'y',
     {0x0079, 0x00FD, 0x1EF3, 0x1EF7, 0x1EF9, 0x1EF5},
     {0x0059, 0x00DD, 0x1EF2, 0x1EF6, 0x1EF8, 0x1EF4}},
};

const ToneRow* findRow(wchar_t base) {
  for (const auto& row : kToneTable)
    if (row.base == base) return &row;
  return nullptr;
}

}  // namespace

Tone toneForKey(wchar_t k) {
  switch (k) {
    case L's': return Tone::Acute;
    case L'f': return Tone::Grave;
    case L'r': return Tone::Hook;
    case L'x': return Tone::Tilde;
    case L'j': return Tone::Dot;
    default: return Tone::None;
  }
}

int tonePlacementIndex(const Letter* letters, int count, const SyllableParts& parts,
                       ToneStyle style) {
  if (!parts.valid || parts.nucleusLen == 0) {
    for (int i = count - 1; i >= 0; --i)
      if (isVowelBase(letters[i].base)) return i;
    return -1;
  }
  const int start = parts.onsetLen;
  const int len = parts.nucleusLen;
  for (int i = start + len - 1; i >= start; --i)
    if (isMarkedVowel(letters[i].base)) return i;
  if (parts.codaLen > 0) return start + len - 1;
  if (len == 1) return start;
  if (len >= 3) return start + len - 2;                     // penultimate
  return style == ToneStyle::Old ? start : start + 1;       // 2 vowels
}

wchar_t vowelWithTone(wchar_t base, Tone tone, bool upper) {
  const ToneRow* row = findRow(base);
  if (!row) return letterChar(base, upper);
  const wchar_t* forms = upper ? row->upper : row->lower;
  return forms[static_cast<int>(tone)];
}

wchar_t letterChar(wchar_t base, bool upper) {
  if (!upper) return base;
  if (base == kDStroke) return L'Đ';  // Đ
  if (const ToneRow* row = findRow(base)) return row->upper[0];
  if (base >= L'a' && base <= L'z') return base - L'a' + L'A';
  return base;
}

}  // namespace goxvi::detail
