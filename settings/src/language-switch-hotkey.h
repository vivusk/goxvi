#pragma once

namespace goxvi::settings {

// Đặt hotkey chuyển ngôn ngữ nhập liệu của Windows thành Ctrl+Shift (thói quen
// UniKey: Ctrl+Shift đảo ENG <-> VIE) cho user hiện tại, hiệu lực NGAY (không
// cần đăng nhập lại). Gọi từ helper `--activate-tip` của installer. Best-effort:
// trả false nếu không ghi được registry (không chặn phần cài kiểu gõ).
bool setCtrlShiftLanguageSwitchHotkey();

}  // namespace goxvi::settings
