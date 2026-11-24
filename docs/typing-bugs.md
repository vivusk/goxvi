# Goxvi — Sổ theo dõi bug khi gõ

Mỗi bug lúc gõ (sai chữ, mất chữ, lặp chữ, dấu sai chỗ, phím bị nuốt...) ghi 1 mục ở đây, kể
cả bug chưa sửa. Mục đích: lần sau gặp triệu chứng giống thì tra lại nguyên nhân + cách sửa cũ,
và biết app nào đã kiểm tay.

## Cách ghi

- ID tăng dần `TB-NNN`, không dùng lại. Thêm 1 dòng vào bảng + 1 mục chi tiết bên dưới.
- Trạng thái: `Mở` (chưa sửa) · `Đã sửa — chờ kiểm tay` · `Đã sửa` (đã gõ thử thật trong app) ·
  `Hạn chế đã biết` (chấp nhận, không sửa).
- Nền tảng: `mac` / `win` / `core` (engine, ảnh hưởng cả hai).
- Không dán nội dung người dùng gõ thật vào log/bug ngoài ví dụ tối thiểu để tái hiện.

Mẫu mục chi tiết:

```markdown
### TB-NNN — <tóm tắt ngắn>
- **Nền tảng / app:** mac · Chrome (thanh địa chỉ)
- **Trạng thái:** Mở | Đã sửa — chờ kiểm tay | Đã sửa | Hạn chế đã biết
- **Tái hiện:** các bước gõ → kết quả sai; kết quả mong muốn
- **Nguyên nhân:** vì sao (xác nhận bằng đo, hay chỉ suy luận?)
- **Cách sửa:** file/hàm đổi gì; test thêm
- **Commit:** <hash> (<ngày>)
- **Ghi chú:** rủi ro, trường hợp biên, app cần kiểm thêm
```

## Bảng tổng hợp

| ID | Nền tảng | App | Triệu chứng | Trạng thái | Commit |
|---|---|---|---|---|---|
| TB-001 | core | mọi app | "mèo" ra "meò" khi bật kiểu bỏ dấu mới | Đã sửa | fd4bace |
| TB-002 | mac | Telegram, Claude (VSCode) | Enter ngay sau từ phải bấm 2 lần | Đã sửa | (đợt 2026-11-15) |
| TB-003 | mac | app chèn trực tiếp | ⌫ giữa từ lúc được lúc không | Đã sửa | (đợt 2026-11-15) |
| TB-004 | mac | Chrome (thanh địa chỉ) | Có gợi ý bôi đen, gõ `dd` ra "dd" thay vì "đ" | Đã sửa — chờ kiểm tay | 853cfce |
| TB-005 | mac | Numbers (ô bảng) | Gõ trên ô đang chọn mất chữ đầu ("t" gạch chân, gõ "r" chỉ còn "r") | Đã sửa — chờ kiểm tay | 853cfce |
| TB-006 | mac | mọi app | Gõ tắt / tự sửa ngay trước Enter có thể cần 2 lần Enter | Hạn chế đã biết | — |

## Chi tiết

### TB-001 — Kiểu bỏ dấu mới dời dấu sai với eo/ai/ao...
- **Nền tảng / app:** core · mọi app
- **Trạng thái:** Đã sửa
- **Tái hiện:** bật kiểu bỏ dấu mới, gõ `meof` → "meò"; mong muốn "mèo".
- **Nguyên nhân:** kiểu mới dời dấu sang nguyên âm thứ 2 cho mọi cặp 2 nguyên âm không có phụ âm cuối.
- **Cách sửa:** chỉ dời dấu với oa/oe/uy; eo/ai/ao/au/ay/ia/iu/oi/ui/ua giữ dấu ở nguyên âm đầu ở cả hai kiểu.
- **Commit:** fd4bace (2026-11-15)

### TB-002 — Enter sau từ phải bấm 2 lần (marked text)
- **Nền tảng / app:** mac · Telegram native (`ru.keepcoder.Telegram`), Claude extension trong VSCode
- **Trạng thái:** Đã sửa (đo tay: gửi bằng 1 Enter)
- **Tái hiện:** gõ 1 từ rồi Enter → lần Enter đầu chỉ chốt marked text, phải Enter lần 2 mới gửi.
- **Nguyên nhân:** app dùng Enter đầu để kết thúc marked text.
- **Cách sửa:** chế độ Direct — chữ chèn thẳng qua `insertText:replacementRange:`, không marked
  text; Enter/dấu cách sau từ thường đi thẳng tới app (`DirectInsertTracker`, PDR D2, K11).
  Fallback marked: iTerm2, Terminal, ô không đọc được `selectedRange` (`insert-mode-policy.cpp`).
- **Commit:** đợt 2026-11-15 (xem `project-changelog.md`)

### TB-003 — ⌫ giữa từ lúc được lúc không
- **Nền tảng / app:** mac · các app dùng chế độ Direct
- **Trạng thái:** Đã sửa
- **Nguyên nhân:** app bỏ qua lệnh chỉ xoá (`insertText:@""` có `replacementRange`).
- **Cách sửa:** ⌫ chỉ xoá ký tự cuối → để app tự xoá (trả `NO`); ⌫ làm dời dấu → lệnh thay
  (xoá > 0, chèn > 0). Lệnh không bao giờ chỉ xoá (`direct-insert-tracker.cpp`).
- **Commit:** đợt 2026-11-15

### TB-004 — Chrome: gợi ý autocomplete làm mất biến đổi chữ
- **Nền tảng / app:** mac · Chrome, thanh địa chỉ (có thể cả ô tìm kiếm có gợi ý inline khác)
- **Trạng thái:** Đã sửa — chờ kiểm tay
- **Tái hiện:** gõ `d` → Chrome gợi ý "duy-pham.com", phần "uy-pham.com" bôi đen; gõ `d` → ra
  "dd", mất bôi đen. Mong muốn "đ". Gõ tiếp khi không có bôi đen thì bình thường.
- **Nguyên nhân:** `DirectInsertTracker::isStale` coi mọi vùng chọn (`selectedRange.length > 0`)
  là "người dùng đã click/chọn" → bỏ từ, chữ `d` sau bắt đầu từ mới.
- **Cách sửa:**
  - `isStale(caret)` chỉ so vị trí đầu vùng chọn với cuối từ; vùng chọn bắt đầu đúng cuối từ = gợi ý inline, giữ từ.
  - `applyTextEdit` thay `{caret - N, N + độ dài vùng chọn}` → nuốt luôn gợi ý như gõ thường.
  - ⌫ khi đang có gợi ý: không đưa vào engine, trả `NO` (app chỉ xoá gợi ý, từ giữ nguyên).
  - Test: `DirectInsertTrackerTest.InlineSuggestionAfterCaretKeepsWord`.
- **Commit:** 853cfce (2026-11-24)
- **Ghi chú:** tự kéo chọn đoạn bắt đầu đúng cuối từ rồi gõ tiếp → đoạn đó bị thay (giống gõ thường).

### TB-005 — Numbers: mất chữ đầu khi gõ trên ô đang chọn
- **Nền tảng / app:** mac · Numbers (dự đoán cả Keynote)
- **Trạng thái:** Đã sửa — chờ kiểm tay
- **Tái hiện:** chọn 1 ô (chưa vào chế độ sửa), gõ `t` → "t" gạch chân; gõ `r` → chỉ còn "r",
  không gạch chân; gõ tiếp bình thường. Mong muốn "tr".
- **Nguyên nhân (suy luận, chưa đo):** đầu từ không đọc được caret → chế độ Marked (gạch chân).
  Numbers mở ô sửa mới và làm rơi marked text; `dropStaleComposition` thấy hết `markedRange`
  tưởng app đã nhận chữ → `abandon`, chữ đầu mất.
- **Cách sửa:** `recoverDroppedWord:` (`goxvi-input-controller.mm`) — đầu từ không đọc được caret,
  giờ đọc được và hết marked text → `reinsertWord` chèn lại từ (bỏ qua nếu ngay trước caret đã có,
  tránh lặp chữ), chuyển sang Direct giữa từ (`DirectInsertTracker::adopt`).
  Test: `DirectInsertTrackerTest.AdoptedWordContinuesInDirectMode`.
- **Commit:** 853cfce (2026-11-24)
- **Ghi chú:** nếu kiểm tay vẫn lỗi → giả thuyết sai; cần log Debug `selectedRange`/`markedRange`
  (không log nội dung gõ) để xem Numbers thực sự báo gì. Cần kiểm: `tre`+`s` → "tré", `dd` → "đ".

### TB-006 — Gõ tắt / tự sửa ngay trước Enter
- **Nền tảng / app:** mac · chưa đo theo app
- **Trạng thái:** Hạn chế đã biết
- **Nguyên nhân:** chữ thay thế được chèn trong lần nhấn Enter, một số app có thể nuốt Enter đó.
