# Goxvi — Development Roadmap

Cập nhật: 2026-07-09

## Đã xong (plan `plans/260708-2047-goxvi-telex-ime-implementation/`)

| Phase | Trạng thái |
|---|---|
| 1. Scaffolding & build system | ✅ 2026-07-08 |
| 2. Telex engine core & unit tests | ✅ 2026-07-08 (91 tests hiện tại) |
| 3. TSF TIP skeleton & registration | ✅ 2026-07-08 — registered, gõ thực tế OK |
| 4. Engine integration & hidden composition | ✅ 2026-07-08 |
| 5. Config JSON + live reload | ✅ 2026-07-08 |
| 6. Settings app | ✅ 2026-07-08 — app chạy, ACL granted; viết lại C++/Win32 native 2026-07-12 (bỏ .NET, link goxvi-core) |

## Refinements sau khi dùng thực tế (2026-07-09)

- ✅ Simple Telex (bỏ standalone w→ư)
- ✅ Esc = hủy gõ về phím thô (kiểu UniKey)
- ✅ Foreign backspace tự hồi tiếng Việt (nhuwg+BS → như)
- ✅ Nav-key defer+reinject thống nhất mọi app (fix PageUp/SSH race trong terminal)

## Đang mở / kế tiếp (ưu tiên gợi ý)

1. **Verification còn thiếu** — test matrix 8 app tick từng ô (Notepad ✅, Terminal/SSH ✅,
   còn Word, Excel, Chrome, Edge, VSCode, UWP search); live reload trong UWP (H3);
   password field không biến đổi.
2. **Per-app disable list** — `disabledApps` trong config + gate theo tên process + UI
   settings (đã thiết kế miệng với user, chưa làm — user bảo khoan).
3. x86 build nếu gặp app 32-bit thật (unresolved từ plan).
4. Hotkey toggle Việt/Anh riêng (ngoài Win+Space) — YAGNI đến khi cần.
5. Ship: installer MSI + code signing — ngoài scope hiện tại.
6. Tương lai xa: kiểu gõ VNI, bảng gõ tắt (nền settings UI sẵn).

## Nợ kỹ thuật / known limitations

- Layout không phải QWERTY (AZERTY...) chưa hỗ trợ (hệ quả bỏ ToUnicodeEx — M4).
- "express" không rơi Foreign (cấu trúc parse hợp lệ) — plan chỉ yêu cầu `windows`;
  muốn giống UniKey English-detection cần heuristic thêm.
- Literal backspace: raw tracking best-effort (Esc sau khi vừa BS trong Literal có thể
  lệch nhẹ) — edge case chấp nhận.
- Secure desktop (UAC/logon) không gõ được tiếng Việt — chủ đích (L5).
