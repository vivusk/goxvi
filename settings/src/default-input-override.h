#pragma once

#include <string>

namespace goxvi::settings {

// "Override for default input method" (Settings > Typing > Advanced keyboard
// settings) = value `InputMethodOverride` dưới
// HKCU\Control Panel\International\User Profile. Đây mới là thứ quyết định
// kiểu gõ MẶC ĐỊNH cho process mới. KIỂM CHỨNG THỰC NGHIỆM (0.0.4 field
// report "vào list nhưng không thành default"): SetDefaultLayoutOrTip trả OK
// nhưng KHÔNG ghi value này — muốn Goxvi là default phải set tường minh.

// Đọc value hiện tại ("" nếu chưa có override — tức "Use language list").
std::wstring readDefaultInputMethodOverride();

// Đảm bảo override == tip: đã đúng thì thôi; chưa thì set qua cmdlet chính
// chủ Set-WinDefaultInputMethodOverride (không tự chọc registry ghi) rồi
// verify lại qua registry (read-only). Log từng bước. true = override đã là
// Goxvi.
bool ensureDefaultInputMethodOverride(const std::wstring& tip);

// Uninstall: nếu override đang trỏ Goxvi thì xoá value (về "Use language
// list") — để nguyên sẽ thành override ma trỏ TIP đã gỡ. Xoá value trực tiếp
// là ngoại lệ có chủ đích của quy tắc "không chọc User Profile": cmdlet không
// có tham số clear đáng tin, và chỉ xoá khi value đúng là của Goxvi.
void clearDefaultInputMethodOverrideIfGoxvi(const std::wstring& tip);

}  // namespace goxvi::settings
