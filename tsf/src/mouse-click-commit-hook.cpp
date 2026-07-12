#include "mouse-click-commit-hook.h"

#include "debug-logging.h"
#include "key-event-handler.h"  // kGoxviInjectedKeyTag

namespace goxvi_mouse {

namespace {

// Hook state is per input thread — matches the TIP's activation model.
thread_local HHOOK g_mouseHook = nullptr;
thread_local std::function<bool()> g_onPointerDown;

// Client-area button downs only: they move the caret / open menus, which is
// what makes IMM hosts cancel the composition. NC/X buttons are left alone.
DWORD buttonDownSendInputFlag(WPARAM message) {
  switch (message) {
    case WM_LBUTTONDOWN: return MOUSEEVENTF_LEFTDOWN;
    case WM_RBUTTONDOWN: return MOUSEEVENTF_RIGHTDOWN;
    case WM_MBUTTONDOWN: return MOUSEEVENTF_MIDDLEDOWN;
    default: return 0;
  }
}

LRESULT CALLBACK mouseHookProc(int code, WPARAM wparam, LPARAM lparam) {
  // HC_ACTION only — HC_NOREMOVE peeks don't consume the message.
  const DWORD downFlag =
      code == HC_ACTION ? buttonDownSendInputFlag(wparam) : 0;
  if (downFlag && g_onPointerDown) {
    const MOUSEHOOKSTRUCT* info =
        reinterpret_cast<const MOUSEHOOKSTRUCT*>(lparam);
    const bool ownReinjected =
        info && info->dwExtraInfo == goxvi_keys::kGoxviInjectedKeyTag;
    if (!ownReinjected && g_onPointerDown()) {
      // The commit's result text travels as POSTED messages, but this click
      // was already pulled from the queue and would reach the window first —
      // the host would cancel the word before the text lands. Eat the click
      // and re-send the same button through the input queue: posted messages
      // outrank input, so the committed word arrives first, then the click.
      // Same mechanism as reinjectKey for keyboard terminators.
      GOXVI_LOG(L"click while composing -> commit, re-inject click");
      INPUT input = {};
      input.type = INPUT_MOUSE;
      input.mi.dwFlags = downFlag;  // at the current cursor position
      input.mi.dwExtraInfo = goxvi_keys::kGoxviInjectedKeyTag;
      // Only discard the original button down when the replacement really got
      // queued; if SendInput is blocked (BlockInput, desktop switch) let the
      // original click through — the word is already committed, so at worst
      // the click ordering degrades instead of the click vanishing.
      if (SendInput(1, &input, sizeof(INPUT)) == 1) return 1;
    }
  }
  return CallNextHookEx(nullptr, code, wparam, lparam);
}

}  // namespace

void installClickCommitHook(std::function<bool()> onPointerDown) {
  removeClickCommitHook();
  g_onPointerDown = std::move(onPointerDown);
  g_mouseHook = SetWindowsHookExW(WH_MOUSE, mouseHookProc, nullptr,
                                  GetCurrentThreadId());
  GOXVI_LOG(L"mouse hook installed=%d", g_mouseHook ? 1 : 0);
}

void removeClickCommitHook() {
  if (g_mouseHook) {
    UnhookWindowsHookEx(g_mouseHook);
    g_mouseHook = nullptr;
  }
  g_onPointerDown = nullptr;
}

}  // namespace goxvi_mouse
