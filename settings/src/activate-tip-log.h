#pragma once

namespace goxvi::settings {

// Nhật ký chẩn đoán cho helper --activate-tip / --deactivate-tip. Helper chạy
// KHÔNG cửa sổ từ installer (ExecAsOriginalUser) nên khi một máy cài xong mà
// Goxvi không vào được language list thì file này là bằng chứng duy nhất cho
// biết bước nào hỏng (field report 0.0.3: InstallLayoutOrTip trượt im lặng).
// Ghi vào %APPDATA%\Goxvi\activate-tip.log — tạo mới mỗi lần chạy.
void activateTipLogBegin(const wchar_t* mode);

// Ghi 1 dòng printf-style (tự thêm xuống dòng). Nội dung để ASCII/không dấu —
// mở được bằng mọi editor bất kể codepage. No-op nếu Begin chưa chạy/thất bại.
void activateTipLog(const wchar_t* format, ...);

}  // namespace goxvi::settings
