#include "utf8-utf16-conversion.h"

#include <cstddef>
#include <cstdint>

namespace goxvi::detail {
namespace {

constexpr char32_t kReplacement = 0xFFFD;

bool isInvalidCodePoint(char32_t cp) {
  return cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF);  // surrogates illegal
}

void appendUtf16(std::wstring& out, char32_t cp) {
  if (isInvalidCodePoint(cp)) cp = kReplacement;
  if (cp <= 0xFFFF) {
    out.push_back(static_cast<wchar_t>(cp));
  } else {
    cp -= 0x10000;
    out.push_back(static_cast<wchar_t>(0xD800 + (cp >> 10)));
    out.push_back(static_cast<wchar_t>(0xDC00 + (cp & 0x3FF)));
  }
}

void appendUtf8(std::string& out, char32_t cp) {
  if (isInvalidCodePoint(cp)) cp = kReplacement;
  if (cp <= 0x7F) {
    out.push_back(static_cast<char>(cp));
  } else if (cp <= 0x7FF) {
    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else if (cp <= 0xFFFF) {
    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  } else {
    out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
  }
}

}  // namespace

std::wstring utf8ToWide(std::string_view utf8) {
  std::wstring out;
  out.reserve(utf8.size());
  const std::size_t n = utf8.size();
  std::size_t i = 0;
  while (i < n) {
    const unsigned char c0 = static_cast<unsigned char>(utf8[i]);
    char32_t cp = 0;
    int extra = 0;
    char32_t minValue = 0;  // shortest-form lower bound (reject overlong)
    if (c0 < 0x80) {
      cp = c0;
    } else if ((c0 & 0xE0) == 0xC0) {
      cp = c0 & 0x1F;
      extra = 1;
      minValue = 0x80;
    } else if ((c0 & 0xF0) == 0xE0) {
      cp = c0 & 0x0F;
      extra = 2;
      minValue = 0x800;
    } else if ((c0 & 0xF8) == 0xF0) {
      cp = c0 & 0x07;
      extra = 3;
      minValue = 0x10000;
    } else {  // invalid lead byte (continuation or 0xF8+)
      appendUtf16(out, kReplacement);
      ++i;
      continue;
    }

    bool ok = true;
    for (int k = 1; k <= extra; ++k) {
      if (i + k >= n) {
        ok = false;
        break;
      }
      const unsigned char cc = static_cast<unsigned char>(utf8[i + k]);
      if ((cc & 0xC0) != 0x80) {  // not a continuation byte
        ok = false;
        break;
      }
      cp = (cp << 6) | (cc & 0x3F);
    }
    if (!ok || cp < minValue) {  // truncated or overlong → one replacement char
      appendUtf16(out, kReplacement);
      ++i;
      continue;
    }
    appendUtf16(out, cp);
    i += static_cast<std::size_t>(extra) + 1;
  }
  return out;
}

std::string wideToUtf8(std::wstring_view wide) {
  std::string out;
  out.reserve(wide.size() * 2);
  const std::size_t n = wide.size();
  std::size_t i = 0;
  while (i < n) {
    const char32_t unit = static_cast<char16_t>(wide[i]);
    if (unit >= 0xD800 && unit <= 0xDBFF) {  // high surrogate
      if (i + 1 < n) {
        const char32_t lo = static_cast<char16_t>(wide[i + 1]);
        if (lo >= 0xDC00 && lo <= 0xDFFF) {
          const char32_t cp =
              0x10000 + ((unit - 0xD800) << 10) + (lo - 0xDC00);
          appendUtf8(out, cp);
          i += 2;
          continue;
        }
      }
      appendUtf8(out, kReplacement);  // unpaired high surrogate
      ++i;
      continue;
    }
    if (unit >= 0xDC00 && unit <= 0xDFFF) {  // unpaired low surrogate
      appendUtf8(out, kReplacement);
      ++i;
      continue;
    }
    appendUtf8(out, unit);
    ++i;
  }
  return out;
}

}  // namespace goxvi::detail
