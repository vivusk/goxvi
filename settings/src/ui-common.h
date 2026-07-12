#pragma once

#include <windows.h>

#include <string>

#include "config-store.h"

// Small shared helpers for the plain-Win32 settings UI: DPI scaling, a common
// Segoe UI font, and child-control creation. Kept header-only + tiny so each tab
// module stays focused on its own layout/logic.
namespace goxvi::settings::ui {

// Per-page context handed to each tab: the shared config + the UI font so every
// tab renders identically and edits ONE in-memory config (saved on change).
struct PageContext {
  ConfigStore* store;
  HFONT font;
  int dpi;         // resolved once from the top-level window
  HWND status;     // shared status label at the window bottom (may be null)
};

inline int scale(int value, int dpi) { return MulDiv(value, dpi, 96); }

// Set the shared status line to the standard "saved at HH:mm:ss" confirmation.
inline void notifySaved(const PageContext& ctx) {
  if (!ctx.status) return;
  SYSTEMTIME t = {};
  GetLocalTime(&t);
  wchar_t msg[128];
  swprintf_s(msg, L"Đã lưu %02d:%02d:%02d — áp dụng sau ~2 giây.", t.wHour,
             t.wMinute, t.wSecond);
  SetWindowTextW(ctx.status, msg);
}

// A 9pt Segoe UI font at the given DPI (caller owns/DeleteObject).
inline HFONT createUiFont(int dpi) {
  return CreateFontW(-MulDiv(9, dpi, 72), 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                     FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

// Create a child control and apply the shared font. x/y/w/h are in 96-dpi
// logical units and scaled here so every call site stays DPI-agnostic.
inline HWND child(HWND parent, const wchar_t* cls, const wchar_t* text,
                  DWORD style, int x, int y, int w, int h, int id,
                  const PageContext& ctx) {
  HWND h_ = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                            scale(x, ctx.dpi), scale(y, ctx.dpi),
                            scale(w, ctx.dpi), scale(h, ctx.dpi), parent,
                            reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
                            GetModuleHandleW(nullptr), nullptr);
  if (h_) SendMessageW(h_, WM_SETFONT, reinterpret_cast<WPARAM>(ctx.font), TRUE);
  return h_;
}

}  // namespace goxvi::settings::ui
