# Goxvi for macOS — Quyết định thiết kế

Trạng thái: **Đã triển khai giai đoạn đầu** (bản cá nhân, ký ad-hoc) — `Goxvi.app` gõ Telex/VNI,
phím tắt ⌃⇧, menu input, Settings AppKit. Kiểm thử tương thích app còn chờ kiểm tay.
Ngày: 2026-11-15 (duyệt + triển khai); 2026-11-15 D2 đổi sang chèn chữ trực tiếp (xem D2).

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
| D1 | **Chỉ dùng InputMethodKit (IMK)**, không hook. Gặp app lỗi thì ghi nhận rồi mới tính cách xử lý (đổi cách chèn chữ cho app đó, hoặc hook). Đã áp dụng hướng "đổi cách chèn chữ" (D2, 2026-11-15) — vẫn không event tap / Accessibility | API chính thức, không cần quyền Accessibility, process riêng nên bộ gõ crash không kéo app chết theo |
| D2 | **Mặc định chèn chữ trực tiếp vào văn bản qua `insertText:replacementRange:`** (composition ẩn, không gạch chân — giống bản Win/UniKey): mỗi phím chỉ thay phần đuôi khác của từ đang gõ. **Marked text (gạch chân) chỉ còn là fallback**: app trong danh sách ngoại lệ (hard-code ở `mac/src/insert-mode-policy.cpp`, xem "Tồn đọng tương thích") hoặc ô nhập không đọc được `selectedRange`. Thay D2 cũ ("marked text có gạch chân như bộ gõ Apple") ngày 2026-11-15 | Marked text làm Enter ngay sau từ phải bấm 2 lần ở Telegram native và Claude extension (VSCode): app nhận Enter khi còn trạng thái IME nên bỏ qua lệnh gửi. Spike 2026-11-15 (đo tay + log số liệu): chèn trực tiếp → gửi bằng 1 Enter ở cả hai app, chữ đúng ở Telegram, VSCode, Chrome; iTerm2 bỏ qua `replacementRange` → giữ marked |
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
- **Chèn trực tiếp phụ thuộc app tôn trọng `replacementRange` và báo đúng `selectedRange`**.
  App bỏ qua `replacementRange` (như iTerm2) sẽ ra chữ thừa/sai (vd. `vieê…`). Bộ gõ đọc
  `selectedRange` trước mỗi phím; con trỏ lệch khỏi cuối từ (click, app xoá ô chat) → bỏ từ đang
  gõ, chữ đã chèn giữ nguyên. Không dựa vào đọc lại chữ vừa chèn (Chromium/VSCode trả chữ cũ).
- **Undo (⌘Z) theo từng lần chèn**, không theo cả từ như composition — tuỳ app gộp hay không.
- **App chưa đo vẫn chạy chèn trực tiếp mặc định**. Bề mặt đã đo: Telegram native, VSCode (ô chat
  Claude extension), Chrome (đạt); iTerm2 (không đạt → marked). Chưa đo: Safari, TextEdit, Notes,
  Messages, Claude.app, WebStorm, Terminal.app (chờ kiểm tay). Gặp app ra chữ thừa: báo bundle ID →
  thêm vào danh sách ngoại lệ ở `mac/src/insert-mode-policy.cpp` (+ test), build và cài lại. Không
  có công tắc lúc chạy, không cấu hình qua `config.json`/Settings.
- **Marked text (fallback) trong một số app** (Terminal, app Java, game) có thể hành xử lạ. Xử lý
  theo D1: gặp đâu ghi nhận đó.
- **Phím tắt ⌃⇧ chỉ hoạt động khi đang focus ô nhập liệu** (IMK chỉ nhận phím khi có
  text client). Bên ngoài ô nhập thì không có tác dụng, nhưng cũng không cần.
- **Phát triển IMK hơi bất tiện**: sau mỗi lần build phải kill process bộ gõ để macOS nạp
  bản mới; đôi khi phải log out/in khi lần đầu đăng ký.

## Đã chốt thêm

- **Cấu trúc repo:** monorepo, phần Mac là thư mục `mac/` trong repo `goxvi`, ngang hàng
  `tsf/`, link thẳng target `goxvi-core`. Không dùng submodule.
- **Phím tắt mặc định:** ⌃⇧ (theo D6).
- **macOS tối thiểu:** 13 Ventura.

## Tồn đọng tương thích

Theo D1, lỗi riêng từng app được **ghi nhận** trước; hướng đã dùng: đổi cách chèn chữ theo app
(D2). `plans/` là local-only nên kết quả đo chép vào đây. Nguồn: spike đo chèn trực tiếp
2026-11-15 (quan sát người đo + log số liệu). Kiểm tay đầy đủ bản đã sửa (S1–S8 mọi app):
**chưa kiểm tay**.

| App (bundle ID) | Kịch bản | Hiện tượng | Hướng xử lý |
|---|---|---|---|
| Telegram native `ru.keepcoder.Telegram` (bản Swift, không phải Telegram Desktop Qt) | Từ có dấu + Enter ngay | Marked text: phải bấm Enter 2 lần mới gửi | Chèn trực tiếp → gửi bằng 1 Enter (đã đo ở spike) |
| Claude extension (webview VSCode, `com.microsoft.VSCode`) | Từ có dấu + Enter ngay | Như Telegram | Chèn trực tiếp → gửi bằng 1 Enter (đã đo ở spike) |
| Telegram, VSCode, Chrome | ⌫ giữa từ | `insertText:@""` (lệnh chỉ xoá) bị app bỏ qua 100% → ⌫ "lúc được lúc không" | ⌫ chỉ xoá ký tự cuối đi thẳng tới app; ⌫ làm đổi nhiều hơn (dời dấu) → lệnh thay `del>0, ins>0` |
| Telegram | Gửi tin / đổi chat khi đang có từ dở | `selectedRange` nhảy về 0 | Bỏ từ dở (không xoá nhầm) |
| VSCode, Chrome | Đọc lại chữ ngay sau khi chèn | Trả chữ cũ dù màn hình đúng (client bất đồng bộ) | Không dùng đọc lại; chỉ so `selectedRange` trước phím kế |
| Claude extension (quan sát); log mọi app đã đo | Click giữa từ | IMK không gọi `commitComposition:` (0 lần trong log) | Phát hiện qua con trỏ lệch → bỏ từ, chữ cũ giữ nguyên (đã quan sát ở Claude extension) |
| iTerm2 `com.googlecode.iterm2` | S1–S3 | Bỏ qua `replacementRange`, con trỏ không cập nhật → chữ sai hoàn toàn | Fallback marked (hard-code) |
| Terminal.app `com.apple.Terminal` | — | Chưa đo; suy theo iTerm2 | Fallback marked (hard-code), chờ kiểm tay xác nhận |
| Safari, TextEdit, Notes, Messages, Claude.app, WebStorm | S1–S8 | Chưa đo (chạy chèn trực tiếp mặc định) | Chờ kiểm tay; lỗi → thêm bundle vào ngoại lệ |

App chạy marked text (fallback, hard-code ở `mac/src/insert-mode-policy.cpp`, so khớp bundle ID
chính xác, không cấu hình được qua `config.json`/Settings): `com.googlecode.iterm2`,
`com.apple.Terminal`; cộng mọi ô nhập không đọc được `selectedRange`. Chế độ chọn ở đầu mỗi từ.

Hạn chế đã biết (do thiết kế, không phải lỗi app):

- **Gõ tắt / tự sửa ngay trước Enter** có thể vẫn phải bấm Enter 2 lần: chữ thay thế được chèn
  trong chính lần nhấn Enter. Số lần Enter theo app (kịch bản S4): **chưa đo** (chờ kiểm tay).
- Mất focus / đảo Việt-Anh giữa từ giữ đúng chữ đang hiện, **không** áp gõ tắt/tự sửa
  (khác Win — plan K8). Chèn trực tiếp: chữ đã nằm trong văn bản, không cần commit.
- Config chỉ đọc 1 lần khi process bộ gõ khởi động; sửa tay `config.json` phải `killall Goxvi`
  (Settings/menu áp dụng ngay — plan K5).
- Trạng thái Việt/Anh không lưu qua lần khởi động; icon menu bar tĩnh.
