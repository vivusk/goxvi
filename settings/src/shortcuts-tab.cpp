#include "tabs.h"

#include <windows.h>

#include <commctrl.h>

#include <string>
#include <vector>

#include "goxvi/engine-types.h"

namespace goxvi::settings::ui {

namespace {

constexpr wchar_t kClass[] = L"GoxviShortcutsTab";

enum : int {
  kList = 2001,
  kTrigger = 2002,
  kExpansion = 2003,
  kCommit = 2004,
  kDelete = 2005,
};

struct State {
  PageContext ctx;
  HWND list = nullptr;
  HWND trigger = nullptr;
  HWND expansion = nullptr;
  HWND commit = nullptr;
  int editing = -1;      // index into config().shortcuts, or -1 = "Thêm" mode
  bool filtering = false;  // reentrancy guard for the a-z trigger filter
};

State* stateOf(HWND h) {
  return reinterpret_cast<State*>(GetWindowLongPtrW(h, GWLP_USERDATA));
}

std::wstring windowText(HWND h) {
  const int n = GetWindowTextLengthW(h);
  std::wstring s(static_cast<size_t>(n), L'\0');
  if (n) GetWindowTextW(h, s.data(), n + 1);
  return s;
}

std::wstring trim(std::wstring s) {
  const auto notSpace = [](wchar_t c) { return c != L' ' && c != L'\t'; };
  while (!s.empty() && !notSpace(s.back())) s.pop_back();
  size_t i = 0;
  while (i < s.size() && !notSpace(s[i])) ++i;
  return s.substr(i);
}

// Triggers must be lowercase a-z (the engine only matches raw lowercase keys);
// drop everything else so the user can't create a dead entry.
std::wstring filterTrigger(const std::wstring& in) {
  std::wstring out;
  out.reserve(in.size());
  for (wchar_t c : in) {
    if (c >= L'a' && c <= L'z') out.push_back(c);
    else if (c >= L'A' && c <= L'Z') out.push_back(static_cast<wchar_t>(c + 32));
  }
  return out;
}

// Collapse duplicate triggers to one row (last expansion wins, first position
// kept) — mirrors the engine's "later entry wins" + the old app's upsert.
void dedupByTrigger(std::vector<goxvi::ShortcutEntry>& v) {
  std::vector<goxvi::ShortcutEntry> out;
  for (const auto& e : v) {
    bool merged = false;
    for (auto& o : out) {
      if (o.trigger == e.trigger) {
        o.expansion = e.expansion;
        merged = true;
        break;
      }
    }
    if (!merged) out.push_back(e);
  }
  v = std::move(out);
}

void reloadList(State* s) {
  ListView_DeleteAllItems(s->list);
  const auto& v = s->ctx.store->config().shortcuts;
  for (size_t i = 0; i < v.size(); ++i) {
    LVITEMW it = {};
    it.mask = LVIF_TEXT;
    it.iItem = static_cast<int>(i);
    it.pszText = const_cast<LPWSTR>(v[i].trigger.c_str());
    ListView_InsertItem(s->list, &it);
    ListView_SetItemText(s->list, static_cast<int>(i), 1,
                         const_cast<LPWSTR>(v[i].expansion.c_str()));
  }
}

void resetEditBar(State* s) {
  s->editing = -1;
  SetWindowTextW(s->trigger, L"");
  SetWindowTextW(s->expansion, L"");
  SetWindowTextW(s->commit, L"Thêm");
}

void loadRowIntoEditBar(State* s, int index) {
  const auto& v = s->ctx.store->config().shortcuts;
  if (index < 0 || index >= static_cast<int>(v.size())) return;
  s->editing = index;
  SetWindowTextW(s->trigger, v[index].trigger.c_str());
  SetWindowTextW(s->expansion, v[index].expansion.c_str());
  SetWindowTextW(s->commit, L"Cập nhật");  // Esc still cancels edit (no button)
  SetFocus(s->expansion);
}

void commit(State* s) {
  const std::wstring trig = trim(windowText(s->trigger));
  const std::wstring exp = trim(windowText(s->expansion));
  if (trig.empty() || exp.empty()) {
    if (s->ctx.status)
      SetWindowTextW(s->ctx.status,
                     L"Cần nhập cả viết tắt lẫn nội dung thay thế.");
    return;
  }
  auto& v = s->ctx.store->config().shortcuts;
  if (s->editing >= 0 && s->editing < static_cast<int>(v.size())) {
    v[static_cast<size_t>(s->editing)] = {trig, exp};
  } else {
    v.push_back({trig, exp});
  }
  dedupByTrigger(v);
  s->ctx.store->save();
  reloadList(s);
  resetEditBar(s);
  notifySaved(s->ctx);
  SetFocus(s->trigger);
}

void deleteSelected(State* s) {
  const int sel = ListView_GetNextItem(s->list, -1, LVNI_SELECTED);
  auto& v = s->ctx.store->config().shortcuts;
  if (sel < 0 || sel >= static_cast<int>(v.size())) return;
  v.erase(v.begin() + sel);
  s->ctx.store->save();
  reloadList(s);
  resetEditBar(s);
  notifySaved(s->ctx);
}

// Subclass on the two edits: Enter commits, Esc cancels an in-progress edit.
LRESULT CALLBACK editSubclass(HWND h, UINT msg, WPARAM w, LPARAM l,
                              UINT_PTR, DWORD_PTR ref) {
  auto* s = reinterpret_cast<State*>(ref);
  if (msg == WM_GETDLGCODE) return DLGC_WANTALLKEYS;  // receive Enter/Esc
  if (msg == WM_KEYDOWN) {
    if (w == VK_RETURN) { commit(s); return 0; }
    if (w == VK_ESCAPE && s->editing >= 0) { resetEditBar(s); return 0; }
  }
  return DefSubclassProc(h, msg, w, l);
}

void buildControls(HWND page, State* s) {
  const PageContext& c = s->ctx;

  // Chiều cao trang co theo page area (cửa sổ thu thấp cho tab Tổng quan): ListView
  // + nút Xoá luôn khít đáy dù kClientH đổi. Client rect physical -> logical (child()
  // scale lại). List lấp từ dưới hàng edit tới ngay trên nút Xoá.
  RECT prc;
  GetClientRect(page, &prc);
  const int pageH = MulDiv(prc.bottom, 96, c.dpi);
  const int pageW = MulDiv(prc.right, 96, c.dpi);
  const int delH = 28, pad = 6, listTop = 46;
  const int delY = pageH - delH - pad;
  const int listH = delY - pad - listTop;

  // Bề rộng co theo page (form hẹp): nút Thêm/Cập nhật neo phải, ô "Thay bằng" giãn
  // lấp khoảng giữa, ListView fill ngang.
  const int btnW = 64, trigW = 52;
  const int btnX = pageW - 8 - btnW;
  const int expX = 14 + trigW + 6;
  const int expW = btnX - 6 - expX;
  const int listW = pageW - 22;

  // Edit bar on ONE row: trigger + expansion + Thêm/Cập nhật (no Huỷ button;
  // Esc cancels an in-progress edit).
  s->trigger = child(page, L"EDIT", L"", ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP,
                     14, 10, trigW, 26, kTrigger, c);
  SendMessageW(s->trigger, EM_SETLIMITTEXT, goxvi::kMaxRawKeys, 0);
  SendMessageW(s->trigger, EM_SETCUEBANNER, TRUE,
               reinterpret_cast<LPARAM>(L"trc"));

  s->expansion = child(page, L"EDIT", L"",
                       ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, expX, 10, expW,
                       26, kExpansion, c);
  // Cap the expansion so one entry can't balloon config.json past the shared
  // read cap (kMaxConfigFileBytes) — the TIP would silently stop reloading.
  SendMessageW(s->expansion, EM_SETLIMITTEXT, goxvi::kMaxShortcutExpansionChars,
               0);
  SendMessageW(s->expansion, EM_SETCUEBANNER, TRUE,
               reinterpret_cast<LPARAM>(L"trước"));

  s->commit = child(page, L"BUTTON", L"Thêm", BS_DEFPUSHBUTTON | WS_TABSTOP, btnX,
                    9, btnW, 28, kCommit, c);

  s->list = child(page, WC_LISTVIEWW, L"",
                  LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER |
                      WS_TABSTOP,
                  14, listTop, listW, listH, kList, c);
  ListView_SetExtendedListViewStyle(
      s->list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

  LVCOLUMNW col = {};
  col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  col.pszText = const_cast<LPWSTR>(L"Viết tắt");
  col.cx = scale(62, c.dpi);
  col.iSubItem = 0;
  ListView_InsertColumn(s->list, 0, &col);
  col.pszText = const_cast<LPWSTR>(L"Thay bằng");
  col.cx = scale(listW - 62 - 20, c.dpi);  // fill phần còn lại (chừa ~20 cho scrollbar)
  col.iSubItem = 1;
  ListView_InsertColumn(s->list, 1, &col);

  child(page, L"BUTTON", L"Xoá mục đã chọn", WS_TABSTOP, 14, delY, 150, delH,
        kDelete, c);

  // Route Enter/Esc from the edits through our handlers.
  SetWindowSubclass(s->trigger, editSubclass, 1, reinterpret_cast<DWORD_PTR>(s));
  SetWindowSubclass(s->expansion, editSubclass, 1,
                    reinterpret_cast<DWORD_PTR>(s));

  reloadList(s);
}

void onTriggerChanged(State* s) {
  if (s->filtering) return;
  const std::wstring cur = windowText(s->trigger);
  const std::wstring filtered = filterTrigger(cur);
  if (filtered == cur) return;
  s->filtering = true;
  SetWindowTextW(s->trigger, filtered.c_str());
  SendMessageW(s->trigger, EM_SETSEL, filtered.size(), filtered.size());
  s->filtering = false;
}

LRESULT CALLBACK wndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
  State* s = stateOf(h);
  switch (msg) {
    case WM_CREATE: {
      auto* cs = reinterpret_cast<CREATESTRUCTW*>(l);
      s = new State{};
      s->ctx = *static_cast<PageContext*>(cs->lpCreateParams);
      SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(s));
      buildControls(h, s);
      return 0;
    }
    case WM_COMMAND:
      if (!s) break;
      switch (LOWORD(w)) {
        case kCommit: commit(s); return 0;
        case kDelete: deleteSelected(s); return 0;
        case kTrigger:
          if (HIWORD(w) == EN_CHANGE) onTriggerChanged(s);
          return 0;
      }
      return 0;
    case WM_NOTIFY: {
      if (!s) break;
      auto* nm = reinterpret_cast<NMHDR*>(l);
      if (nm->idFrom == kList && nm->code == LVN_ITEMCHANGED) {
        auto* ch = reinterpret_cast<NMLISTVIEW*>(l);
        if ((ch->uChanged & LVIF_STATE) && (ch->uNewState & LVIS_SELECTED))
          loadRowIntoEditBar(s, ch->iItem);
      }
      return 0;
    }
    case WM_CTLCOLORSTATIC:
      // Windows default dialog colour (COLOR_3DFACE) - no white.
      SetBkMode(reinterpret_cast<HDC>(w), TRANSPARENT);
      return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_3DFACE));
    case WM_NCDESTROY:
      delete s;
      SetWindowLongPtrW(h, GWLP_USERDATA, 0);
      return 0;
  }
  return DefWindowProcW(h, msg, w, l);
}

void ensureClass() {
  static bool registered = false;
  if (registered) return;
  WNDCLASSW wc = {};
  wc.lpfnWndProc = wndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
  wc.lpszClassName = kClass;
  RegisterClassW(&wc);
  registered = true;
}

}  // namespace

HWND createShortcutsTab(HWND parent, const PageContext& ctx, RECT area) {
  ensureClass();
  PageContext local = ctx;
  return CreateWindowExW(0, kClass, nullptr, WS_CHILD | WS_CLIPCHILDREN,
                         area.left, area.top, area.right - area.left,
                         area.bottom - area.top, parent, nullptr,
                         GetModuleHandleW(nullptr), &local);
}

}  // namespace goxvi::settings::ui
