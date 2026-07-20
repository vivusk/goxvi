# Goxvi — System Architecture

Cập nhật: 2026-07-09

## Tổng quan

```
┌──────────────────────────────── app process (Notepad, Terminal, ...) ─┐
│  TSF (msctf) ── ITfKeyEventSink ──► goxvi-tsf.dll (TIP, COM, /MT)     │
│                                        │ thin shim                    │
│                                        ▼                              │
│                                  goxvi-core.lib (TelexEngine)         │
│                                        │ display text                 │
│                                        ▼                              │
│      hidden composition (ITfComposition, no display attribute)        │
└────────────────────────────────────────────────────────────────────────┘
          ▲ config.json (đọc, lazy mtime 2s)
          │
%APPDATA%\Goxvi\config.json ◄── atomic write ── Goxvi.exe (Win32 native, link goxvi-core)
                                                (grant ACL S-1-15-2-1 cho UWP)
```

Không IPC, không thread phụ trong DLL. Settings app và TIP chỉ "nói chuyện" qua file.

## Keystroke flow

1. Phím → `OnTestKeyDown`/`OnKeyDown` (eaten quyết định giống hệt nhau ở cả hai).
2. `classifyKey` (key-event-handler): Letter / Backspace / CancelToRaw (Esc) /
   CommitAndPass (terminator: space, Enter, dấu câu, nav key...) /
   CommitAndPassChord (Ctrl-Alt-Win) / Ignore (Shift, CapsLock...).
   Map VK A–Z thủ công + Shift/CapsLock — không dùng ToUnicodeEx (M4, phá dead-key).
   VK 0–9 → Letter chỉ khi `inputMethod=Vni` (tone/dấu bằng số); Telex thì số vẫn là
   terminator như cũ. Config được poll trước `classifyKey` (đầu mỗi từ) để quyết định
   đúng ngay từ phím số đầu tiên.
3. Letter → `TelexEngine::processKey` → edit session `TF_ES_SYNC|TF_ES_READWRITE` →
   `CompositionManager` SetText + display attribute rỗng (không gạch chân) + caret cuối
   từ. Composition bắt đầu tại selection hiện tại (GetSelection; fallback
   InsertTextAtSelection QUERYONLY khi fail).
   **Gõ đè selection** (Explorer F2 rename: tên cũ đang bôi đen): phím chữ đầu từ
   (chỉ trong OnKeyDown — OnTestKeyDown phải side-effect free) →
   `clearFocusedEditControlSelection()`: hỏi thẳng focused window (class chứa "edit")
   bằng `EM_GETSEL`, selection không rỗng → `WM_CLEAR` **đồng bộ** (SendMessage,
   TIP chạy in-process) rồi mới compose. Phải đi đường Win32 trực tiếp vì CUAS edit
   control (1) giấu selection thật khỏi GetSelection (trả collapsed dù control đang
   bôi đen — đo bằng harness), và (2) giam MỌI document edit của TIP (kể cả scratch
   composition start→empty→end) tới khi composition cuối kết thúc → xóa kiểu TSF thấy
   text cũ "ma" cạnh chữ mới tới tận commit. Đã thử inject VK_DELETE + bắn lại phím
   chữ qua SendInput: FAIL — rename edit của Explorer terminate composition non trẻ
   khi nhận text-change notification, từ bị tách ("ar" → "a"+"r" thay vì "ả", còn dính
   thêm gõ tắt của từ sau). Safety net trong `ensureStarted` vẫn xóa selection
   TSF-visible trước khi compose (app aware). E2E test:
   `scripts/test-explorer-rename-e2e.ps1` (rename thật, kỳ vọng `ả.txt`),
   `scripts/test-cuas-edit-harness.ps1` (WinForms TextBox, kiểm tra ghost).
4. Terminator khi đang compose → **eat → commit → reinject** phím qua SendInput (tag
   `kGoxviInjectedKeyTag`, sink pass phím tự bắn qua `GetMessageExtraInfo`; eat trong cả
   Test, commit+reinject chỉ trong KeyDown). Lý do: text commit đi lane async chậm hơn
   phím thô ở terminal host (bug "pm2 listcd" khi PageUp trên SSH) và CUAS edit control
   (Explorer rename: Enter chốt tên CŨ, "-" rơi trước chữ → "-abc"). Cơ chế giống MS
   Telex built-in, áp dụng mọi app. Không compose → pass through như thường.
5. Ctrl/Alt/Win chord → commit + pass (không eat, app phải thấy chord thật với modifier
   state sống). Chạy idempotent cả trong OnTestKeyDown (một số app chỉ gọi Test).
6. Commit paths khác: mất focus (ThreadMgr OnSetFocus + sink OnSetFocus), app tự
   terminate composition (OnCompositionTerminated — click/caret move), overflow 32 keys,
   Deactivate. Các đường ngoài key event KHÔNG được cấp sync edit session
   (`TF_E_SYNCHRONOUS`) → `runEditSession` fallback `TF_ES_ASYNCDONTCARE`, mọi lambda
   commit capture by value (chạy trễ an toàn).
6b. **Click chuột khi đang compose** (fix 260711 — mất chữ ở IMM/CUAS host: Telegram/Qt,
   Sublime, WinForms TextBox; các host này coi composition string là tạm và CANCEL khi
   click, mọi rewrite TSF-side bị vứt — đo bằng harness):
   - `mouse-click-commit-hook.*`: WH_MOUSE **thread hook** (TIP in-process) bắt
     button-down TRƯỚC window proc → `commitOnPointerDown()`:
     `ImmNotifyIME(NI_COMPOSITIONSTR, CPS_COMPLETE)` trên hIMC của focused window
     (IMM host commit ngay) + `commitCurrentWord()` fallback (TSF-native host).
   - Result string đi đường POSTED message, còn click đã bị lấy khỏi queue sẽ tới
     window proc trước → hook **eat click + reinject qua SendInput** (tag
     `kGoxviInjectedKeyTag`, posted > input trong priority queue) — chữ vào trước,
     click xử lý sau. Cùng cơ chế reinject như terminator (mục 4).
   - `ITfTextEditSink::OnEndEdit` (advise theo focused context, re-advise khi đổi
     focus): selection đổi + caret thoát khỏi vùng composition (check
     `selectionCoveredByComposition`, end-inclusive vì edit của mình luôn đặt caret
     cuối từ) → commit không expand — lưới an toàn cho caret move không do chuột.
   - `OnCompositionTerminated(ecWrite)`: đọc lại text của range, nếu host đã xóa →
     SetText lại display cuối (ecWrite là write cookie TSF cấp riêng cho việc này).
   - WH_MOUSE chỉ thấy input THẬT của thread đó (PostMessage giả không qua hook —
     harness phải click bằng SendInput + DPI-aware). Click sang app khác không qua
     hook → đường mất focus (6) xử lý.
   - Harness: `scratchpad test-click-during-composition.ps1` (gõ banj → click chỗ
     khác → kỳ vọng 'bạn' còn nguyên, caret theo click).
7. Password field (input scope IS_PASSWORD, check bằng read-only edit session lúc đầu từ)
   + compartment keyboard-disabled → pass-through toàn bộ.
8. **Gõ tắt** cắm vào đúng 1 hàm `commitCurrentWord(applyShortcut=true)` — điểm hội tụ
   mọi đường commit (7 call site). Khi commit VÀ enable VÀ raw exact VÀ `matchShortcut`
   khớp → `commitWithReplacement` (SetText expansion + caret-to-end + EndComposition trong
   1 edit session, phím kết thúc từ rơi SAU expansion). Esc truyền `applyShortcut=false`
   (raw thắng). Literal-backspace → `rawKeysExact()=false` → không expand (raw lệch display).
   Mọi đường click/caret-move (hook chuột, OnEndEdit, OnCompositionTerminated) → KHÔNG
   expand — chữ chốt đúng như hiển thị, click không phải terminator (Decision 8).

## Engine (goxvi-core)

State machine per-từ:

```
Empty ──letter──► Composing ──parse invalid──► Foreign (display = raw, compose tiếp raw)
                     │  ▲                          │ backspace: replay còn lại hợp lệ → Composing
                     │  └──────────────────────────┘
                     └──gõ lặp modifier──► Literal (display buffer đóng băng, pop 1:1)
```

- Composing: raw key buffer là source of truth; sau mỗi key áp `applyKeyToWord`
  (parse → transform → tone); backspace = pop raw đến khi display ngắn đi đúng 1 ký tự.
- Recompute pipeline: `vietnamese-syllable-parser` (onset/nucleus/coda, prefix-validity,
  gi/qu đặc biệt) → `telex-transform-rules` (aa/ee/oo/dd/w, tone s f r x j, z, undo) hoặc
  `vni-transform-rules` (a6/e6/o6, o7/u7, a8, d9, tone 1-5, undo) tùy `EngineConfig.inputMethod`
  → `tone-placement-rules` (marked vowel > coda > kiểu cũ/mới) → render UTF-16.
  Hai bộ rule dùng chung `WordState`/`KeyOutcome` (định nghĩa trong telex-transform-rules.h);
  VNI không có transform theo chữ cái — chữ luôn append thẳng, chỉ số 1-9 kích hoạt biến đổi.
- `rawTypedKeys()` phục vụ Esc hủy gõ (mọi state đều track raw); `rawKeysExact()` gate gõ tắt
  (false sau Literal-backspace vì raw có thể lệch display).
- Cap 32 keys → `KeyResult.overflow` → shim commit + reset + re-feed (R2-M1).
- Gõ tắt tách hẳn engine: `shortcut-expander` (`matchShortcut`) thuần, không biết Telex/VNI;
  `utf8-utf16-conversion` thuần (không Windows API/codecvt) mang expansion có dấu giữa JSON
  UTF-8 và `ShortcutEntry` wstring (surrogate pair cho non-BMP, invalid → U+FFFD).

## Config

- Schema v1: `{"version":1,"enabled":bool,"toneStyle":"old"|"new","inputMethod":"telex"|"vni",
  "shortcutsEnabled":bool,"restoreOnInvalid":bool,"autoCorrectEnabled":bool,
  "shortcuts":[{"trigger","expansion"}]}`; parse tolerant (hỏng/thiếu/lạ
  → default per-field; entry gõ tắt thiếu field/rỗng → bỏ qua entry đó, không fail cả mảng),
  serializer thuần trong core (unit-test được).
- DLL chỉ ĐỌC; reload lazy: mỗi lần bắt đầu từ mới, nếu >2s từ check trước thì stat
  mtime; đổi → reload → `setConfig`. `enabled=false` → pass-through từ gate đầu từ.
- Settings app ghi temp-then-rename (atomic); tạo `%APPDATA%\Goxvi` + grant read ACL
  `ALL APPLICATION PACKAGES` (S-1-15-2-1) để TIP trong process AppContainer đọc được (H3).

## Quyết định kiến trúc quan trọng

| Quyết định | Lý do |
|---|---|
| Hidden composition, không commit-then-replace | Đường chuẩn mọi app test kỹ; undo theo từ sạch |
| CRT tĩnh toàn cục từ phase 1 (H2) | DLL load vào mọi process, không phụ thuộc VCRuntime; tránh LNK2038 |
| Engine thuần tách khỏi TSF | Unit-test giây, dev-loop không cần đăng ký lại |
| L5 REVISED: đăng ký CẢ SECUREMODE + COMLESS | Game/anticheat host (LoL) kích hoạt TSF bằng `ActivateEx(TF_TMAE_SECUREMODE)` → msctf CHỈ nạp TIP có category SECUREMODE (probe kiểm chứng: flags 0x2 không nạp TIP thiếu nó) — thiếu thì TIP chết tới khi user toggle kiểu gõ in-game; mọi TIP built-in MS đều có. Vệ sinh secure-mode: không log phím Release, không UI, tôn trọng password context, và SKIP mouse click-commit hook khi flags có TF_TMAE_SECUREMODE (giảm footprint trong game/UAC). COMLESS: kích hoạt trên thread chưa CoInitialize, đường code degrade an toàn |
| Nav-key defer + reinject thống nhất mọi app | User yêu cầu 1 cơ chế; fix race terminal; direct mode đã thử rồi bỏ (13abb0c) |
| Direct mode hồi sinh CHỈ cho URL/search bar trình duyệt (0.0.7) | Composition commit reset default match của Chromium omnibox → Enter/End mất inline suggestion; built-in Telex dính y hệt (user kiểm chứng), mọi biến thể commit (in-band, tự accept tail) chỉ đổi xác suất thắng race. Gõ qua key lane (backspace + KEYEVENTF_UNICODE SendInput) là đường duy nhất omnibox coi như gõ tay. Gate kép: exe browser (`hostHandlesClickTermination_`) + InputScope IS_URL/IS_SEARCH probe đầu từ; VK_DELETE dẫn đầu dọn tail suggestion đang bôi đen (probe `selectionIsNonEmpty`) né lỗi lặp chữ kiểu UniKey; gõ tắt không expand ở đây. Mọi field khác giữ composition — decision trên vẫn đúng cho phần còn lại |
| Gõ tắt cắm 1 hàm commitCurrentWord, không đổi state machine | Điểm hội tụ mọi commit; expand = "thay composition trước khi commit"; engine không cần biết gõ tắt tồn tại |
| Tự sửa lỗi đảo phím (`typo-autocorrect`, 0.0.9) CHỈ lúc chốt từ: rule 1 = dấu/thanh trước nguyên âm (neo vào `w`), rule 2 = coda cuối `ng`/`nh`/`ch` gõ đảo `gn`/`hn`/`hc` | Gõ nhanh đảo phím bổ trợ ra trước nguyên âm (`twosi`/`twsoi` cho `tới`). Sửa ở commit (không live) → không phá gõ tay, chỉ đụng từ đang Foreign (từ hợp lệ → nullopt). Bóc phím bổ trợ SÁT trước nguyên âm, phần còn lại phải là onset NON-EMPTY hợp lệ (không đụng phụ âm đầu → `she` giữ nguyên). **Telex bắt buộc run bị bóc chứa `w`**: `s f r x j` trùng phụ âm cụm tiếng Anh (br, cr, pr, thr...) nên bóc `r`/`s` lẻ sẽ phá `bra→bả`, `pray→...` — lớp false-positive vô hạn; `w` không phải phụ âm Việt nên `w` trước nguyên âm gần như luôn là móc đặt sai (tone kèm theo ăn theo). VNI miễn gate (bổ trợ là SỐ, không có trong từ Anh). Chốt chặn cuối = blocklist tiếng Anh hard-code (~20 từ chứa w: two, twang, dwarf, thwart...) sinh offline từ wordlist qua gtest DISABLED; `twos` cố ý loại (user muốn →tớ). Rule 2 (`cuxgn`→`cũng`, `nhahn`→`nhanh`, `sahcs`→`sách`): quét từ phải, cặp `gn`/`hn`/`hc` cuối, KHÔNG ở đầu từ (`gnaw`) và phía sau chỉ được là phím bổ trợ (`cugnx`), hoán vị rồi compose lại — chạy cả Telex lẫn VNI nên blocklist chặn `sign`→`sing`. Blocklist chung 27 từ. Opt-in `autoCorrectEnabled` mặc định BẬT. Gõ tắt ưu tiên trước |
| KHÔNG phục hồi phím với từ TOÀN CHỮ HOA (`shouldRelaxAcronym`, 0.0.9) | Viết tắt toàn hoa có dấu (`ĐAL`, `ĐL`, `PLÔ`) là chủ đích của user → không revert-to-raw như spell-check. Khi `Invalid` (chỉ ở strict), nếu từ toàn hoa + (có chữ Việt có dấu đ ă â ê ô ơ ư HOẶC chưa có nguyên âm) + ≥2 letter → set cờ `acronymRelaxed`, recompose RELAXED (không bao giờ Invalid, dấu áp tiếp: `PLOO→PLÔ` vì `PL` invalid trước khi `oo` tới nên phải relaxed sớm, không đóng băng Literal). Vế "có dấu" giữ `ĐAL/ĐL`; loại acronym Anh chỉ có thanh/không dấu (`URL→Ủ+L`, `USB→Ú+B` có nguyên âm nhưng không dấu → vẫn revert raw vì R/S là phím thanh Telex); ≥2 letter loại foreign opener/đầu câu viết hoa đơn (`Windows`). Từ hoa hợp lệ (`VIỆT`) không đụng. Chốt với user: phục hồi CHỈ khi từ có chữ thường |
| Case rule tuyệt đối (chỉ toàn chữ thường mới khớp) | Gõ hoa giữ nguyên chữ gốc, không normalize — đơn giản, đoán được (chốt với user) |
| Debug log compile-out ở Release | IME log phím = keylogger-shaped code |
| Settings Win32 thuần thay WinUI 3 | Cắt ~160 MB runtime/arch, installer 45→2.4 MB, link goxvi-core dùng lại serializer; đánh đổi UI kém hiện đại hơn |
