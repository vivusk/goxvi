#pragma once

namespace goxvi::settings {

// Kích hoạt profile TSF của Goxvi làm bộ gõ tiếng Việt đang dùng cho PHIÊN của
// user hiện tại. Gọi khi chạy `Goxvi.exe --activate-tip` (từ checkbox trang cuối
// của installer, chạy trong ngữ cảnh user qua runasoriginaluser). Trả về true
// nếu ActivateProfile thành công.
bool activateGoxviTip();

}  // namespace goxvi::settings
