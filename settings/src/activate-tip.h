#pragma once

namespace goxvi::settings {

// Cài Goxvi làm bộ gõ tiếng Việt của user hiện tại, trọn bộ 3 bước:
// 1. InstallLayoutOrTip  — ghi BỀN vào language list (HKCU User Profile). Thiếu
//    bước này Goxvi chỉ hiện ở Win+Space chứ không có trong Settings/không thể
//    là default (RegisterProfile enabled-by-default KHÔNG tự thêm vào list).
// 2. SetDefaultLayoutOrTip — đặt làm kiểu gõ MẶC ĐỊNH (override) để process mới
//    (kể cả game) khởi động với Goxvi thay vì bàn phím ENG/US.
// 3. ActivateProfile FORSESSION — có hiệu lực ngay, khỏi Win+Space.
// Gọi khi chạy `Goxvi.exe --activate-tip` (checkbox trang cuối installer, ngữ
// cảnh user qua runasoriginaluser). Trả về true nếu bước 1 + 3 thành công
// (bước 2 best-effort — môi trường quản trị có thể chặn override).
bool activateGoxviTip();

// Gỡ Goxvi khỏi language list của user (InstallLayoutOrTip + ILOT_UNINSTALL).
// Gọi khi chạy `Goxvi.exe --deactivate-tip` (uninstaller, best-effort: nếu
// uninstall chạy bằng tài khoản admin khác thì chỉ dọn được cho user đó).
bool deactivateGoxviTip();

}  // namespace goxvi::settings
