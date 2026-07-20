# Goxvi — Codebase Summary

Cập nhật: 2026-07-09. Trạng thái: 6 phases xong + VNI + gõ tắt (shortcut expansion);
161/161 unit tests; TIP registered (Debug), user gõ hàng ngày.

## Targets

| Target | Loại | Nội dung |
|---|---|---|
| `goxvi-core` | static lib C++20 | Telex engine thuần: state machine (Empty/Composing/Foreign/Literal), syllable parser, tone placement, config serializer. Không Windows API trong logic. |
| `goxvi-tsf` | COM DLL | TSF TIP: key sink, hidden composition (display attribute trống), nav-key defer, config watcher. CRT tĩnh. |
| `goxvi-core-tests` | exe | GoogleTest v1.14.0 (FetchContent) — 197 tests (+1 DISABLED blocklist generator). |
| `goxvi-settings` | Win32 exe (`Goxvi.exe`) | Native ~0.4 MB (/MT tĩnh, KHÔNG .NET), link `goxvi-core` dùng lại serializer; chỉnh `%APPDATA%\Goxvi\config.json`. |

## Hành vi gõ (đã chốt với user)

- **Simple Telex** (default): `aa ee oo dd aw ow uw uow` + dấu `s f r x j`, `z` xóa dấu.
  KHÔNG có `w` đứng một mình → ư (gõ tiếng Anh không biến dạng).
- **VNI** (chọn trong settings): `a6/e6/o6` (â/ê/ô), `o7/u7` (+`uo7`→ươ), `a8` (ă), `d9` (đ),
  số `1-5` = dấu thanh. Chữ cái luôn gõ thẳng, không có transform theo chữ như Telex.
- Đặt dấu kiểu cũ (`hòa`) default, kiểu mới qua config.
- Gõ lặp modifier = undo + literal (`aaa`→aa, `cass`→cas) → state Literal (đóng băng display).
- Từ không thể là tiếng Việt → Foreign: composition hoàn nguyên raw, compose tiếp raw.
- **Backspace trong Foreign tự hồi tiếng Việt**: pop 1 raw key rồi replay phần còn lại,
  toàn bộ hợp lệ → về Composing (`nhuwg`+BS → `như`). Literal vẫn pop 1:1, không hồi transform (M3).
- **Esc = hủy gõ**: từ đang compose hoàn nguyên đúng phím thô (`rawTypedKeys()`), Esc bị nuốt;
  idle thì Esc pass qua app.
- **Nav keys khi đang compose** (PageUp/Down, mũi tên, Home/End, Ins/Del): eat → commit →
  reinject qua SendInput — chữ luôn tới app trước phím (fix race terminal/SSH; giống MS Telex).
- **Gõ tắt** (kiểu UniKey): gõ trigger toàn chữ thường a-z (vd `trc`), khi hết từ (mọi đường
  commit) tự thay bằng expansion (vd `trước`, chuỗi tự do có dấu). Chỉ khớp toàn từ, tuyệt đối
  case (`TRC`/`Trc` không khớp). Bật/tắt riêng (`shortcutsEnabled`), phụ thuộc IME đang bật; Esc
  bỏ qua gõ tắt; Literal-backspace không expand (cờ `rawKeysExact`).

## core/ (engine)

- `include/goxvi/engine-types.h` — `EngineConfig{toneStyle,inputMethod,enabled,shortcutsEnabled,shortcuts}`, `ShortcutEntry{trigger,expansion}` (wstring), `InputMethod{Telex,Vni}`, `KeyResult{consumed,overflow,display}`, `kMaxRawKeys=32`.
- `include/goxvi/telex-engine.h` — API shim thấy: `processKey`, `processBackspace`, `currentDisplay`, `rawTypedKeys`, `rawKeysExact` (gõ tắt gate — false sau Literal-backspace), `reset`, `setConfig` (pimpl). Lớp `TelexEngine` chạy cả Telex lẫn VNI, chọn bằng `EngineConfig.inputMethod`.
- `include/goxvi/config-json-serializer.h` — parse/serialize schema v1, tolerant.
- `include/goxvi/shortcut-expander.h` — `matchShortcut(rawWord, shortcuts)` thuần: trả expansion khi rawWord toàn a-z thường + khớp 1 trigger (reverse scan, entry sau thắng); nullopt nếu có ký tự hoa/không khớp.
- `src/shortcut-expander.cpp` — impl `matchShortcut` + `isAllLowerAscii`, không phụ thuộc Telex/VNI.
- `src/utf8-utf16-conversion.*` — `utf8ToWide`/`wideToUtf8` thuần (không Windows API, không codecvt); serializer dùng để mang expansion có dấu giữa JSON UTF-8 và `ShortcutEntry` wstring; invalid → U+FFFD, surrogate pair cho non-BMP.
- `src/vietnamese-syllable-parser.*` — onset/nucleus/coda + prefix validity (gi/qu đặc biệt M2; bảng nuclei gồm cả raw intermediates ie/uo/ưo/uu/uou/uye...).
- `src/telex-transform-rules.*` — `applyKeyToWord` trả Applied/UndoToLiteral/Invalid; định nghĩa chung `WordState`/`KeyOutcome`/`Letter`/`Tone` cho cả Telex và VNI.
- `src/vni-transform-rules.*` — `applyKeyToWordVni`, cùng contract; digit-only transforms (1-9), chữ cái luôn plain.
- `src/tone-placement-rules.*` — marked vowel > coda > old/new style; bảng 12 nguyên âm × 6 thanh × 2 case.
- `include/goxvi/typo-autocorrect.h` + `src/typo-autocorrect.cpp` — `autoCorrectTypo(rawKeys, cfg)`: entry point + helper dùng chung (`toLowerAscii`/`isMisplaceableModifier`/`composeStrict`) + blocklist; 2 rule ở file riêng (`typo-autocorrect-misplaced-modifier.cpp`, `typo-autocorrect-transposed-coda.cpp`), thử lần lượt, match đầu thắng. Rule 1 (phím bổ trợ trước nguyên âm): lúc chốt từ, sửa lỗi phím bổ trợ gõ trước nguyên âm (`twosi`/`twsoi`→`tới`). Chỉ từ Foreign; bóc phím bổ trợ sát trước nguyên âm, chèn lại sau, kiểm hợp lệ Việt. Telex bắt buộc run bóc chứa `w` (né cụm phụ âm Anh br/cr/pr...); VNI peel số 1-8. Rule 2 (coda 2 ký tự cuối `ng`/`nh`/`ch` gõ đảo thành `gn`/`hn`/`hc`): quét từ phải, cặp cuối cùng (không được ở đầu từ, phía sau chỉ được là phím bổ trợ) → hoán vị 2 ký tự → compose lại (`cuxgn`→`cũng`, `nhahn`→`nhanh`, `sahcs`→`sách`, `cugnx`→`cũng`). Chốt chặn `english-blocklist.generated.inc` (27 từ, sinh offline qua gtest DISABLED). Dùng chung `detail::renderWord` với engine.
- `src/telex-engine.cpp` — raw buffer source of truth; dispatch Telex/VNI theo config; Composing recompute; Foreign replay-on-backspace; Literal buffer riêng (vẫn track raw cho Esc); cờ `rawExact` (false khi Literal-backspace, true khi clear); overflow cap 32 (R2-M1).

## tsf/ (TIP shim)

- `dll-main/dll-exports/class-factory/goxvi-tsf.def` — COM plumbing, 4 exports.
- `goxvi-guids.*` — CLSID `{7A3B9F42-...}`, profile, display attribute; TU duy nhất initguid (+inputscope.h).
- `goxvi-text-service.*` — ITfTextInputProcessorEx + KeyEventSink + CompositionSink + DisplayAttributeProvider + ThreadMgrEventSink; wire engine + composition + config.
- `key-event-handler.*` — phân loại VK (map A–Z thủ công, KHÔNG ToUnicodeEx — M4); VK 0-9 → Letter chỉ khi `inputMethod=Vni`; CommitAndPass (terminator) tách riêng CommitAndPassChord (Ctrl/Alt/Win); compartment disabled; `readFieldScopes` (1 edit session đọc input scope → password fail-safe true + urlField IS_URL/IS_SEARCH); `selectionIsNonEmpty` (probe tail bôi đen cho direct mode); `clearFocusedEditControlSelection` (`EM_GETSEL` + `WM_CLEAR` sync lên focused "edit" window — CUAS giấu selection thật khỏi GetSelection; inject VK_DELETE đã thử và fail: Explorer terminate composition non trẻ khi text đổi); `reinjectKey` (SendInput, EXTENDEDKEY chỉ cho nav key, tag `kGoxviInjectedKeyTag`).
- `key-event-sink.cpp` — OnTestKeyDown/OnKeyDown chung handler, eaten nhất quán; đầu từ (`wordStart` = không composition + engine rỗng) probe scope 1 lần (cache 100ms cùng blocked verdict) → set `directWordMode_` = browser + urlField; letter đầu từ (chỉ KeyDown, Test side-effect free) → `clearFocusedEditControlSelection` trước khi compose (fix gõ đè selection ở Explorer F2 rename/CUAS); terminator khi compose → eat → commit → reinject (fix thứ tự phím/chữ ở terminal + CUAS edit), khi direct word → chỉ `engine_.reset()` + pass native (gõ tắt KHÔNG expand ở URL bar — expansion bơm phím sẽ rơi sau terminator); chord commit idempotent chạy cả trong Test, không eat; Esc → `cancelWordToRaw`/`directCancelToRaw`. `commitCurrentWord(applyShortcut=true)` là điểm hội tụ gõ tắt (chỉ Esc/click truyền false).
- `direct-text-injector.*` — direct mode cho URL/search bar trình duyệt (hồi sinh từ 13abb0c, scoped): backspace + KEYEVENTF_UNICODE qua SendInput thay composition; omnibox thấy gõ tay thuần → suggestion/Enter/End chuẩn 100% (composition commit reset default match của omnibox — built-in Telex dính y hệt, không fix được trong mô hình composition). `leadingForwardDelete`: VK_DELETE dọn tail suggestion đang bôi đen trước khi backspace (tránh lỗi lặp chữ kiểu UniKey), no-op khi caret là insertion point; gate qua `selectionIsNonEmpty`.
- `composition-manager.*` / `edit-session-callbacks.*` — mọi thao tác text qua `TF_ES_SYNC|TF_ES_READWRITE`; composition bắt đầu tại selection (GetSelection, safety net xóa selection non-empty trước StartComposition; fallback InsertTextAtSelection QUERYONLY); caret về cuối từ; commit = EndComposition giữ text; `commitWithReplacement` (gõ tắt): SetText expansion + caret-to-end + EndComposition trong 1 edit session (phím kết thúc từ rơi SAU expansion).
- `display-attribute-provider.*` — TF_DISPLAYATTRIBUTE toàn default → không gạch chân.
- `config-file-watcher.*` — reload lazy mtime, throttle 2s, check lúc bắt đầu từ.
- `tip-registration.*` — CLSID (HKCR, Apartment) + profile 0x042A + categories (L5 revised: CÓ SECUREMODE — game/anticheat như LoL activate TSF secure mode, thiếu là TIP không được nạp; CÓ COMLESS — thread không CoInitialize; ActivateEx skip mouse hook khi TF_TMAE_SECUREMODE).

Ghi chú lịch sử: direct input mode ban đầu làm cho terminal rồi bỏ (13abb0c →
c22195e, user muốn 1 cơ chế thống nhất); 0.0.7 hồi sinh CÓ GIỚI HẠN cho URL/search
bar trình duyệt vì composition về nguyên lý không giữ được omnibox suggestion.

## settings/ (Win32 native)

- Target CMake `goxvi-settings` → `Goxvi.exe` (~0.4 MB, /MT tĩnh, KHÔNG .NET/WinUI/WASDK); link `goxvi-core` để dùng lại `parseConfigJson`/`serializeConfigJson` (schema single-source, không khai lại DTO). `settings-version.h.in` → CMake sinh `settings-version.h` từ file `VERSION` (repo root).
- `src/config-store.{cpp,h}` — load tolerant qua core serializer; save atomic (temp + `MoveFileEx REPLACE_EXISTING`); ghi UTF-8 thuần (không escape `\u`); stamp ACL đọc `S-1-15-2-1` (H3) idempotent qua `src/app-container-acl.{cpp,h}`.
- `src/main.cpp` — entry point, single-instance (mutex), DPI PerMonitorV2, khởi tạo comctl32; bắt cờ `--activate-tip` / `--deactivate-tip` (chạy helper rồi thoát, trước mutex/cửa sổ).
- `src/language-switch-hotkey.{cpp,h}` — đặt hotkey Ctrl+Shift chuyển ngôn ngữ nhập (thói quen UniKey): ghi `HKCU\Keyboard Layout\Toggle` (Language Hotkey=2, Hotkey=2, Layout Hotkey=3 — tắt layout-hotkey để khỏi tranh Ctrl+Shift) + `SystemParametersInfo(SPI_SETLANGTOGGLE)` hiệu lực ngay không cần re-logon. Gọi kèm trong `--activate-tip` (best-effort).
- `src/activate-tip.{cpp,h}` — `Goxvi.exe --activate-tip`: helper không cửa sổ, trọn bộ 3 bước: `InstallLayoutOrTip` (input.dll, load động — ghi BỀN vào language list, lên ĐẦU danh sách vi; RegisterProfile enabled-by-default KHÔNG tự vào list nên Settings/override-dropdown sẽ không thấy nếu thiếu) + `SetDefaultLayoutOrTip` (kiểu gõ mặc định cho process mới, best-effort) + TSF `ActivateProfile(FORSESSION|ENABLEPROFILE)` (hiệu lực ngay). Kết quả bước 1 được VERIFY qua `HKCU\Control Panel\International\User Profile\<tag>` (đúng nguồn Settings đọc, quét mọi tag, read-only — field report 0.0.4: không tin BOOL trả về); thiếu → fallback `Set-WinUserLanguageList` (PowerShell, tự thêm ngôn ngữ Việt, timeout 60 s) rồi verify lại. Exit 0 CHỈ khi bền trong list. `--deactivate-tip`: `InstallLayoutOrTip(ILOT_UNINSTALL)` gỡ khỏi list (uninstaller). Chuỗi `042A:{CLSID}{Profile}` dựng runtime từ `tsf/src/goxvi-guids.cpp` (một nguồn GUID, initguid TU) — COM-only, không `msctf.lib`.
- `src/activate-tip-log.{cpp,h}` — nhật ký chẩn đoán helper: `%APPDATA%\Goxvi\activate-tip.log` (tạo mới mỗi lần chạy, mở/đóng theo dòng `_SH_DENYNO`, UTF-8): kết quả + `GetLastError`/HRESULT từng bước — helper chạy không cửa sổ từ installer nên đây là bằng chứng duy nhất khi máy lạ trục trặc.
- `src/default-input-override.{cpp,h}` — "Override for default input method" = value `InputMethodOverride` (HKCU `Control Panel\International\User Profile`). KIỂM CHỨNG THỰC NGHIỆM (0.0.4): `SetDefaultLayoutOrTip` trả OK nhưng KHÔNG ghi value này → `ensureDefaultInputMethodOverride` set qua cmdlet `Set-WinDefaultInputMethodOverride` (không tự ghi registry) + verify read-only; `clear...IfGoxvi` xoá override ma khi uninstall (ngoại lệ xoá value trực tiếp, chỉ khi value là Goxvi). Exit 0 của `--activate-tip` = bền trong list **và** override là Goxvi.
- `src/run-hidden-powershell.{cpp,h}` — runner PS 5.1 (System32, full path) ẩn cửa sổ dùng chung cho fallback `Set-WinUserLanguageList` + cmdlet override; chờ có timeout (không terminate khi quá hạn — tránh chém giữa lúc ghi language list), log exit code. psCommand chỉ dùng nháy đơn bên trong.
- `src/settings-window.{cpp,h}` — cửa sổ cố định (không maximize), tab control comctl32 (SysTabControl32) 3 thẻ lấp gần hết cửa sổ (lề 8px); `kClientW=228 × kClientH=284` (dáng đứng ~0.8 W:H), khít nội dung tab Tổng quan. ĐÃ BỎ nút Đóng (đóng bằng X caption) + status line (`PageContext.status=nullptr`); cảnh báo nạp config qua MessageBox. `src/{ui-common,tabs}.h` — helper chung + khai báo tab.
- `src/overview-tab.cpp` — thẻ Tổng quan: radio Telex/VNI + kiểu bỏ dấu (cũ/mới) + checkbox "Bật gõ tắt" (đã BỎ chọn giao diện Sáng/Tối — theo hệ thống). Group box strip theme (`SetWindowTheme "",""`) + page bỏ `WS_CLIPCHILDREN` để nền đồng nhất COLOR_3DFACE. Group box/radio rộng ĐỘNG theo page width; nội dung cố định (bottom ~224) quyết định `kClientH`.
- `src/shortcuts-tab.cpp` — thẻ Bảng gõ tắt: ListView + edit bar 1 hàng (upsert theo trigger, lọc trigger a–z). Bề RỘNG + CAO đều động theo page (`GetClientRect`→logical): nút Thêm neo phải, ô "Thay bằng" + ListView + cột giãn ngang; ListView + nút "Xoá mục đã chọn" fill tới đáy — không hardcode nên co theo `kClientW/kClientH`.
- `src/about-tab.cpp` — thẻ Giới thiệu: tiêu đề/phiên bản/ô nội dung read-only rộng + cao động, fill page area.
- `goxvi-settings.rc` — icon + VERSIONINFO + nhúng `goxvi-settings.manifest` (comctl6 + PerMonitorV2), link `/MANIFEST:NO`. Assets `assets/goxvi.ico` + `goxvi-logo.png`.

## Build & scripts

- `cmake --preset x64` → `x64-debug`/`x64-release`; `ctest --preset x64-debug`.
- **QUAN TRỌNG — sửa code TSF phải build Release CẢ HAI bản** `build/x64` **và** `build/x86`
  (`cmake --preset x86` → `x86-release`): app 64-bit load DLL x64, app 32-bit (Zalo
  Desktop = Electron WoW64, và mọi app x86 khác) load DLL x86 — thiếu 1 bản là hai loại
  app lệch hành vi. Đăng ký: x64 = HKLM (regsvr32 elevated), x86 = per-user
  `HKCU\Software\Classes\WOW6432Node\CLSID\{...}\InprocServer32` (không cần admin);
  profile/category CTF trong HKLM dùng chung 2 bitness, không đăng ký lại.
- `scripts/build.ps1 [-Config] [-Test]`; `register-tip.ps1`/`unregister-tip.ps1 [-Config]` (tự elevate UAC; L3: registry giữ path 1 config).
- Settings build tự động cùng `build.ps1`; build lẻ: `cmake --build --preset x64-release --target goxvi-settings`.
- Dev-loop khi DLL bị app lock: rename `goxvi-tsf.dll` → `.oldN.dll` rồi build; app restart mới nhận DLL mới; không cần re-register.

## Test status

- Unit 161/161 (typing table, parser, tone, foreign/backspace/recovery/overflow, serializer, raw-cancel, VNI, gõ tắt match rule, utf8↔utf16 round-trip + non-BMP, rawKeysExact gate).
- Thực tế: Notepad + terminal/SSH user dùng hàng ngày OK.
- Chưa tick: full matrix 8 app, live reload trong UWP (H3), password field, gõ tắt end-to-end
  trong Notepad (cần re-register/restart TIP: Debug DLL đang bị process lock — build Release OK).
