#pragma once

namespace goxvi::settings {

// Cài Goxvi làm bộ gõ tiếng Việt của user hiện tại, trọn bộ 3 bước:
// 1. InstallLayoutOrTip  — ghi BỀN vào language list (HKCU User Profile). Thiếu
//    bước này Goxvi chỉ hiện ở Win+Space chứ không có trong Settings/không thể
//    là default (RegisterProfile enabled-by-default KHÔNG tự thêm vào list).
//    Kết quả được XÁC MINH qua registry (không tin BOOL trả về — field report
//    0.0.3 trượt im lặng); nếu thiếu thì fallback Set-WinUserLanguageList
//    (PowerShell chính chủ, tự thêm ngôn ngữ Việt nếu máy chưa có) rồi verify
//    lại.
// 2. Kiểu gõ MẶC ĐỊNH cho process mới (kể cả game): SetDefaultLayoutOrTip
//    (giữ cho preload/logon) + `InputMethodOverride` qua cmdlet
//    Set-WinDefaultInputMethodOverride rồi verify registry — kiểm chứng thực
//    nghiệm (field report 0.0.4) SetDefaultLayoutOrTip KHÔNG ghi override,
//    mà dropdown "Override for default input method" đọc đúng value đó.
// 3. ActivateProfile FORSESSION — có hiệu lực ngay, khỏi Win+Space.
// Gọi khi chạy `Goxvi.exe --activate-tip` (installer chạy VÔ ĐIỀU KIỆN ở
// ssPostInstall qua ExecAsOriginalUser — ngữ cảnh user, có chờ + đọc exit
// code). Trả về true CHỈ khi bước 1 VÀ 2 xác minh thành công; bước 3
// best-effort (fail vẫn Win+Space chọn được vì list + default đã đúng). Mọi
// bước ghi %APPDATA%\Goxvi\activate-tip.log để chẩn đoán máy lạ.
bool activateGoxviTip();

// Gỡ Goxvi khỏi language list của user (InstallLayoutOrTip + ILOT_UNINSTALL),
// dọn luôn InputMethodOverride nếu đang trỏ Goxvi (tránh override ma).
// Gọi khi chạy `Goxvi.exe --deactivate-tip` (uninstaller, best-effort: nếu
// uninstall chạy bằng tài khoản admin khác thì chỉ dọn được cho user đó).
bool deactivateGoxviTip();

}  // namespace goxvi::settings
