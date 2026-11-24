# Goxvi — Development Roadmap

Cập nhật: 2026-11-15 (thêm macOS; chèn chữ trực tiếp)

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

## macOS — giai đoạn đầu (plan `plans/261115-1631-goxvi-macos-imk/`, 2026-11-15)

| Phase | Trạng thái |
|---|---|
| 1. Build đa nền tảng core + tests | ✅ |
| 2. Khung `Goxvi.app` IMK + cài đặt/đăng ký | ✅ code — chờ kiểm tay |
| 3. Composition session + marked text | ✅ code — chờ kiểm tay |
| 4. Phím tắt ⌃⇧ + menu input + `toggleHotkey` | ✅ code — chờ kiểm tay |
| 5. Settings AppKit | ✅ code — chờ kiểm tay |
| 6. Kiểm tương thích app + docs | 🔄 docs xong — bảng 10 app × 9 kịch bản chờ kiểm tay |

## macOS — chèn chữ trực tiếp (plan `plans/261115-1819-macos-direct-insert-text/`, 2026-11-15)

| Phase | Trạng thái |
|---|---|
| 1. Spike đo `insertText:replacementRange:` | ✅ — Telegram / Claude extension gửi bằng 1 Enter; iTerm2 → marked |
| 2. Logic chèn trực tiếp C++ thuần + test | ✅ |
| 3. Shim Direct + fallback marked, gỡ spike | ✅ code — chờ kiểm tay (S1–S8 mọi app, Terminal.app, S4) |
| 4. PDR D2 + docs | 🔄 docs xong — chờ QA + người duyệt bảng tồn đọng |

Kế tiếp (macOS):
1. Kiểm tay bản chèn trực tiếp (`Goxvi.app` hiện tại) ở app chưa đo; app ra chữ thừa → thêm
   bundle ID vào `mac/src/insert-mode-policy.cpp`; ghi số lần Enter S4 (gõ tắt + Enter) vào PDR.
2. Điền bảng tương thích; lỗi chung của shim sửa + test, lỗi riêng app ghi vào
   [mac/docs/project-overview-pdr.md](../mac/docs/project-overview-pdr.md) "Tồn đọng tương thích".
3. Verify thay đổi root CMake + core trên Windows (`.\scripts\build.ps1 -Config Debug -Test`) trước release chung.
4. Chốt cách đồng bộ `dev` giữa máy Win và máy Mac trước khi snapshot sang `main`.
5. Sau: `.dmg` + Developer ID + notarize, cài `/Library/Input Methods/` (D10 — plan riêng).

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
- L5 revised: SECUREMODE đã đăng ký (bắt buộc để game/anticheat như LoL nạp TIP —
  host secure-mode chỉ nạp TIP có category này). Secure desktop (UAC/logon) nay CÓ THỂ
  nạp Goxvi; hygiene: không log Release, không UI, skip mouse hook ở secure mode.
