# Goxvi for macOS — Quyết định thiết kế

Trạng thái: **Đã duyệt**, chưa plan, chưa code.
Ngày: 2026-11-15.

## Mục tiêu

Bộ gõ tiếng Việt cho macOS, **logic gõ giống hệt Goxvi Windows** (Simple Telex, VNI,
Esc khôi phục phím thô, ⌫ tự bật lại tiếng Việt, gõ tắt, tự sửa lỗi đảo phím, kiểu
đặt dấu cũ/mới). Giai đoạn đầu: dùng cá nhân; sau có thể phát hành `.dmg` ngoài App Store.

## Ngoài phạm vi (giai đoạn đầu)

- Hook bàn phím (CGEventTap) / quyền Accessibility.
- Tích hợp tuỳ chọn vào khung Input Sources của System Settings.
- Nhớ trạng thái Việt/Anh riêng từng app.
- App Store / sandbox.

## Các quyết định

| # | Quyết định | Lý do |
|---|---|---|
| D1 | **Chỉ dùng InputMethodKit (IMK)**, không hook. Gặp app lỗi thì ghi nhận rồi mới tính cách xử lý (đổi cách chèn chữ cho app đó, hoặc hook) | API chính thức, không cần quyền Accessibility, process riêng nên bộ gõ crash không kéo app chết theo |
| D2 | **Chữ đang gõ hiện bằng marked text, có gạch chân giống bộ gõ Apple** | Tương thích rộng nhất. Khác bản Win (composition ẩn): chấp nhận |
| D3 | **Dùng lại `goxvi-core` (C++20) nguyên vẹn**; phần Mac chỉ là lớp vỏ mỏng trong thư mục `mac/` của repo `goxvi`, thay cho `tsf/` | Logic một nguồn, test GoogleTest chạy được trên Mac |
| D4 | Lớp vỏ viết bằng **Objective-C++ (`.mm`)**, gọi thẳng core, không cầu nối Swift | Đơn giản nhất: không cần bridging/interop C++ của Swift |
| D5 | **Một input source duy nhất "Goxvi"** trong danh sách của macOS; chọn Telex/VNI trong Settings | Theo yêu cầu. Không tách mục Telex/VNI |
| D6 | **Phím tắt bật/tắt tiếng Việt do bộ gõ tự xử lý** (chế độ Anh = cho phím đi thẳng). Phím mặc định đề xuất **⌃⇧** (giống thói quen Ctrl+Shift bên Win), đổi được trong Settings | Không phải chuyển input source của hệ thống. IMK nhận được sự kiện phím bổ trợ (flagsChanged) nên làm được ⌃⇧ |
| D7 | **Settings là cửa sổ AppKit do chính process bộ gõ mở**, gọi từ mục "Settings…" trong menu input của thanh menu. Menu đó còn có mục chọn nhanh Telex/VNI và bật/tắt Việt | Không thể chèn tuỳ chọn vào System Settings (Apple dùng extension point riêng `com.apple.textinputmethod-services` kèm `Preferences.prefPane`, bên thứ ba không dùng được). Gộp vào một bundle, không cần app Settings riêng |
| D8 | Config: **cùng schema JSON với bản Win**, file tại `~/Library/Application Support/Goxvi/config.json`, đọc/ghi bằng `parseConfigJson`/`serializeConfigJson` có sẵn trong core. Thêm field cho phím tắt bật/tắt | Một schema, không khai báo DTO lại |
| D9 | Build bằng **CMake** (giống Win), ra bundle `Goxvi.app`. Cài cá nhân: copy vào `~/Library/Input Methods/`, ký ad-hoc | Không cần tài khoản Apple Developer khi dùng cá nhân |
| D10 | Phát hành sau này: `.dmg`, ký Developer ID + notarize, cài vào `/Library/Input Methods/` | Gatekeeper chặn bản không notarize trên máy người khác |

## Rủi ro đã biết

- **`wchar_t` trên Mac là 32-bit** (Win là 16-bit). Core đang dùng `std::wstring`/`wchar_t`
  và có `utf8-utf16-conversion.cpp`. Mọi ký tự tiếng Việt đều nằm trong BMP nên logic gõ
  không bị ảnh hưởng, nhưng phần chuyển đổi UTF-8 ↔ wide và chỗ giao tiếp với `NSString`
  (UTF-16) phải kiểm tra lại. Có thể cần sửa nhẹ core cho portable.
- **Icon trên thanh menu khó đổi động** theo trạng thái Việt/Anh trong IMK. Giai đoạn đầu
  chỉ hiện trạng thái bằng dấu tick trong menu; icon động tính sau nếu cần.
- **Marked text trong một số app** (Chrome omnibox gợi ý tự hoàn thành, Terminal, app Java,
  game) có thể hành xử lạ. Xử lý theo D1: gặp đâu ghi nhận đó.
- **Phím tắt ⌃⇧ chỉ hoạt động khi đang focus ô nhập liệu** (IMK chỉ nhận phím khi có
  text client). Bên ngoài ô nhập thì không có tác dụng, nhưng cũng không cần.
- **Phát triển IMK hơi bất tiện**: sau mỗi lần build phải kill process bộ gõ để macOS nạp
  bản mới; đôi khi phải log out/in khi lần đầu đăng ký.

## Đã chốt thêm

- **Cấu trúc repo:** monorepo, phần Mac là thư mục `mac/` trong repo `goxvi`, ngang hàng
  `tsf/`, link thẳng target `goxvi-core`. Không dùng submodule.
- **Phím tắt mặc định:** ⌃⇧ (theo D6).
- **macOS tối thiểu:** 13 Ventura.
