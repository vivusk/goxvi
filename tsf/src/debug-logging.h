#pragma once

// Debug-only logging via OutputDebugString (view with DebugView/DbgView).
// MUST compile out in Release: an IME that logs keystrokes in production is
// keylogger-shaped code (phase 4 security requirement).
#ifdef _DEBUG

#include <windows.h>

#include <cstdio>

template <typename... Args>
inline void GoxviLogImpl(const wchar_t* fmt, Args... args) {
  wchar_t buf[512];
  swprintf_s(buf, fmt, args...);
  wchar_t line[560];
  swprintf_s(line, L"[goxvi] %s\n", buf);
  OutputDebugStringW(line);
}

#define GOXVI_LOG(...) GoxviLogImpl(__VA_ARGS__)

#else

#define GOXVI_LOG(...) ((void)0)

#endif
