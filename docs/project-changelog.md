# Goxvi — Changelog

## 2026-07-18 (đợt 5 — 0.0.8)

### Fixed (Telex/VNI: không gõ được "thuê"/"thuế", ra "thuee")
- **Triệu chứng:** gõ Telex `thuee` ra `thuee` (không thành `thuê`), `thuees`
  không ra `thuế`. VNI `thue6` cũng dính.
- **Root cause:** parser âm tiết (`vietnamese-syllable-parser.cpp`) có sẵn các
  dạng nguyên âm trung gian thô (`ie`→iê, `uo`→uô/ươ, `uu`→ưu...) nhưng THIẾU
  `ue`. Nên `thue` bị coi là ngoại lai (Foreign) trước khi `e` thứ 2 kịp double
  thành `ê` → chỉ nối thêm chữ.
- **Fix:** thêm `ue` vào `kMaxNuclei` (1 dòng). Fix ở tầng parser dùng chung nên
  cả Telex (`thuee`) lẫn VNI (`thue6`) đều hết lỗi (thuê, huê, khuê, xuê...).
- Test thêm: `thuee→thuê`, `thuees→thuế`, `parseOf("thue")`. 178/178 pass.
- Bump VERSION 0.0.8.

## 2026-07-14 (đợt 4 — 0.0.7)

### Fixed (Edge/Chrome address bar: Enter/End mất inline suggestion)
- **Triệu chứng:** gõ "hồ sơ và" thấy suggestion "hồ sơ và định gia..." nhưng
  Enter/End lúc ăn lúc không (thường mất, chỉ còn phần đã gõ). Built-in Telex
  của Windows dính y hệt — user kiểm chứng.
- **Root cause:** omnibox Chromium reset default match (inline suggestion) khi
  một composition IME kết thúc; mọi biến thể commit từ TIP (in-band không nuốt
  phím, tự accept tail qua SetSelection) chỉ đổi xác suất thắng race giữa
  commit ↔ phím ↔ autocomplete re-query, không deterministic được.
- **Fix — direct mode scoped cho URL/search bar** (`direct-text-injector.*`,
  hồi sinh từ 13abb0c): đầu từ probe InputScope (IS_URL/IS_SEARCH) + host là
  browser → từ đó gõ KHÔNG composition, mọi biến đổi dấu = backspace +
  KEYEVENTF_UNICODE qua SendInput (tag kGoxviInjectedKeyTag). Omnibox thấy gõ
  tay thuần → suggestion/Enter/End/mũi tên chuẩn 100%, hơn built-in Telex.
- **Chống lỗi lặp chữ kiểu UniKey:** khi cần xoá-gõ-lại mà omnibox đang bôi
  đen tail suggestion, backspace đầu sẽ nuốt nhầm tail → chèn 1 VK_DELETE dọn
  tail trước (gate qua `selectionIsNonEmpty` — sync read session; no-op khi
  caret là insertion point).
- `isPasswordContext` → `readFieldScopes` (1 edit session đọc password + url
  scope, cache chung verdict 100ms Test→KeyDown); click khi đang direct word →
  `engine_.reset()` trong `commitOnPointerDown`.
- **Giới hạn chấp nhận:** gõ tắt không expand trong URL bar (expansion bơm
  phím sẽ rơi SAU terminator không bị nuốt); ký tự nháy nhẹ khi bỏ dấu
  (bản chất backspace+retype). Firefox: có trong browser list nhưng urlbar
  set IS_URL hay không chưa kiểm chứng.
- Bump VERSION 0.0.7.

## 2026-07-14 (đợt 3 — 0.0.6)

### Fixed (update tại chỗ lỗi: không thay được goxvi-tsf.dll đang dùng)
- **Root cause:** DLL đã đăng ký được TSF nạp IN-PROCESS vào mọi app đang gõ
  (explorer, browser, Zalo...) nên Inno KHÔNG ghi đè được khi update → lỗi
  "cannot replace file in use" / buộc reboot.
- **`PrepareToInstall` + `FreeLockedDll` (installer/goxvi.iss):** chạy TRƯỚC `[Files]`,
  đổi tên `goxvi-tsf.dll` (x64 + x86) đang bị khoá → `.old` (rồi `.oldN` nếu `.old`
  còn khoá). Windows khoá ghi/xoá DLL đang map nhưng KHÔNG khoá rename → giải phóng
  tên file cho bản mới, KHÔNG cần reboot. Cùng cơ chế mẹo rebuild-khi-bị-lock ở CLAUDE.md.
- **`[UninstallDelete]`** dọn `goxvi-tsf.dll.old*` (x64 + x86) khi gỡ cài.
- **FinishedLabel** thêm nhắc: sau khi CẬP NHẬT, đóng/mở lại app đang gõ (hoặc log
  off/on) để nạp bản mới (app đang chạy vẫn giữ bản `.old` tới khi restart).
- Bump VERSION 0.0.6.

## 2026-07-14 (đợt 2 — 0.0.5)

### Fixed (field report 0.0.4: Goxvi vào list nhưng KHÔNG thành default)
- **Root cause (kiểm chứng thực nghiệm trên máy dev):** `SetDefaultLayoutOrTip` trả OK
  nhưng KHÔNG ghi `InputMethodOverride` (HKCU `Control Panel\International\User Profile`)
  — mà dropdown "Override for default input method" đọc đúng value đó. Thí nghiệm: set
  override = ENG → chạy helper 0.0.4 (exit 0) → override vẫn ENG.
- **Module mới `settings/src/default-input-override.{h,cpp}`:** `ensure...` (skip nếu đã
  đúng → set qua cmdlet chính chủ `Set-WinDefaultInputMethodOverride`, KHÔNG tự ghi
  registry → verify lại read-only), `clear...IfGoxvi` (uninstall: xoá override ma nếu
  đang trỏ Goxvi — ngoại lệ có chủ đích, xoá value trực tiếp vì cmdlet không có clear).
- **Module mới `settings/src/run-hidden-powershell.{h,cpp}`:** runner PS 5.1 ẩn cửa sổ
  dùng chung (fallback `Set-WinUserLanguageList` + override cmdlet), timeout không
  terminate, log exit code.
- **Exit code helper siết lại:** 0 = bền trong language list **VÀ** override đã là Goxvi
  (trước chỉ cần list). `--deactivate-tip` dọn override trước khi gỡ khỏi list. MsgBox
  cảnh báo installer thêm hướng dẫn đặt Override thủ công.
- Bump VERSION 0.0.5.

## 2026-07-14

### Fixed (field report 0.0.3: máy cài xong không có Goxvi trong override-dropdown / language list)
- **Installer chạy activate VÔ ĐIỀU KIỆN + có kiểm tra:** bỏ checkbox postinstall
  `--activate-tip` (bỏ tick / cài silent là mất bước, `nowait` nuốt exit code — fail im
  lặng). Toàn bộ regsvr32 x64 → x86 → `Goxvi.exe --activate-tip` chuyển vào
  `[Code]/RegisterAndActivateTip` tại `ssPostInstall`: tuần tự, `ewWaitUntilTerminated`,
  đọc exit code từng bước; bước hỏng → `Log` + MsgBox hướng dẫn Add a keyboard thủ công
  (`WarnManualStep`, ẩn khi silent). `ExecAsOriginalUser` giữ ngữ cảnh user đăng nhập.
  `FinishedLabel` cập nhật (Goxvi đã là mặc định, Ctrl+Shift chuyển ENG↔VIE).
- **Helper `--activate-tip` tự VERIFY + fallback:** không tin BOOL của
  `InstallLayoutOrTip` nữa — xác minh tip string xuất hiện dưới
  `HKCU\Control Panel\International\User Profile\<tag>` (đúng nguồn Settings đọc, quét
  mọi tag để không phụ thuộc `vi`/`vi-VN`, read-only). Thiếu → fallback
  `Set-WinUserLanguageList` (PowerShell chính chủ, tự thêm ngôn ngữ Việt nếu máy chưa
  có, timeout 60 s không terminate) → verify lại. Exit 0 CHỈ khi đã bền trong language
  list; `SetDefaultLayoutOrTip` + `ActivateProfile` thành best-effort có log.
- **Nhật ký chẩn đoán mới `%APPDATA%\Goxvi\activate-tip.log`** (module
  `settings/src/activate-tip-log.cpp`, tạo mới mỗi lần chạy, mở/đóng theo dòng
  `_SH_DENYNO`): ghi kết quả + `GetLastError`/HRESULT của từng bước
  (InstallLayoutOrTip, verify, fallback, SetDefault, ActivateProfile) — máy lạ trục
  trặc chỉ cần xin file này.
- Bump VERSION 0.0.4.

## 2026-07-13

### Added
- **Tính năng "Khôi phục phím khi gõ sai" (mặc định Bật) — Telex & VNI:** config
  `restoreOnInvalid` (`EngineConfig`, single-source ở `core/include/goxvi/engine-types.h`;
  serialize/parse ở `config-json-serializer.cpp`).
  - **Bật** (mặc định): giữ nguyên spell-check tiếng Việt — phím làm âm tiết sai chính tả thì
    cả từ khôi phục về chuỗi phím thô (bỏ dấu), kiểu UniKey. Không đổi hành vi cũ.
  - **Tắt ("gõ dấu tự do"):** dấu/mũ áp cho MỌI âm tiết, không kiểm tra tiếng Việt. Vd
    `was`→`wá`, `zaajy`→`zậy` (Telex); `wa1`→`wá`, `za6y5`→`zậy` (VNI).
  - Cơ chế: thêm cờ `strict` (= `restoreOnInvalid`) xuyên `parseSyllable` (bỏ các kiểm tra
    membership onset/nhân/coda khi relaxed, chỉ tách cấu trúc) → `applyKeyToWord[Vni]` →
    engine (`applyKey`, `renderWord`, `rebuildWordFromRaw`). Relaxed không bao giờ sinh
    `KeyOutcome::Invalid` nên không rơi về Foreign.
- **UI:** group "Gõ tắt" (tab Tổng quan) đổi tên "Tính năng khác", thêm checkbox
  "Khôi phục phím khi gõ sai" (`overview-tab.cpp`); nới `kClientH` 284→312 cho checkbox mới.
- Test mới: 5 serializer + 5 engine (Telex+VNI free-typing). 176/176 pass x64+x86.

### Fixed (đợt điều tra gõ trong game LoL)
- **Đăng ký thêm category `TIPCAP_SECUREMODE` + `TIPCAP_COMLESS` (L5 revised):** host
  kích hoạt TSF secure-mode (`ActivateEx(TF_TMAE_SECUREMODE)` — game/anticheat) CHỈ nạp
  TIP có SECUREMODE (probe kiểm chứng: 0x2 không nạp TIP thiếu category, sau fix nạp đủ
  mọi flag); COMLESS cho thread chưa `CoInitialize`. Kèm: `ActivateEx` bỏ cài mouse
  click-commit hook khi flags có `TF_TMAE_SECUREMODE` (giảm footprint trong game/UAC).
  Ghi chú: chat LoL vẫn cần toggle kiểu gõ 1 lần/lần mở game — phần chặn còn lại nằm
  trong game process (Vanguard early-block hoặc pipeline GIP nội bộ), ngoài tầm TIP.

### Added (installer tự cấu hình trọn bộ cho user)
- **`Goxvi.exe --activate-tip` nâng cấp trọn bộ 3 bước:** `InstallLayoutOrTip` (ghi BỀN
  vào language list, lên đầu danh sách vi — trước đây chỉ `ActivateProfile` phiên nên
  Settings/override-dropdown không thấy Goxvi) + `SetDefaultLayoutOrTip` (kiểu gõ mặc
  định cho process mới) + `ActivateProfile FORSESSION` (hiệu lực ngay). input.dll load
  động, chuỗi `042A:{CLSID}{Profile}` dựng runtime từ `goxvi-guids.cpp`.
- **`--deactivate-tip` mới:** gỡ Goxvi khỏi language list (`ILOT_UNINSTALL`) — uninstaller
  gọi trong `[UninstallRun]` (best-effort khi uninstall bằng admin khác user).
- **Hotkey Ctrl+Shift chuyển ENG ↔ VIE (thói quen UniKey):** module mới
  `settings/src/language-switch-hotkey.cpp` ghi `HKCU\Keyboard Layout\Toggle`
  (Language Hotkey=2, Hotkey=2, Layout Hotkey=3) + `SPI_SETLANGTOGGLE` hiệu lực ngay;
  gọi kèm trong `--activate-tip` (best-effort). Checkbox installer đổi mô tả tương ứng.

## 2026-07-12

### Fixed (đợt rà soát perf/security)
- **Đồng bộ cap đọc config TIP ↔ settings (`goxvi::kMaxConfigFileBytes` = 1 MB, single-source
  ở `core/include/goxvi/engine-types.h`):** trước đây TIP cap 64 KB còn settings cap 1 MB —
  config vượt 64 KB (gõ tắt dài) làm TIP âm thầm không bao giờ reload config nữa. Kèm:
  - Ô "Thay bằng" giới hạn `kMaxShortcutExpansionChars` (1000 ký tự) qua `EM_SETLIMITTEXT`;
    ô "Viết tắt" dùng `kMaxRawKeys` thay số 32 hardcode.
  - `parseConfigJson` skip entry quá cỡ (trigger > `kMaxRawKeys` không bao giờ match được;
    expansion > cap) — config sửa tay không thổi phồng text commit. Test mới
    `ShortcutsSkipOversizedEntries`.
- **Kiểm tra kết quả `SendInput` trước khi nuốt input gốc:** `reinjectKey` trả bool; nếu
  inject fail (BlockInput/desktop switch) thì key sink un-eat (`*eaten = FALSE`) để host tự
  đưa phím terminator, mouse hook cho click gốc đi qua thay vì `return 1` — không còn khả
  năng mất Enter/space/click của user (từ đã commit xong, chỉ degrade thứ tự).
- **`isPasswordContext` fail-safe + hết dangling capture:** `runEditSession` thêm tham số
  `allowAsyncFallback` (mặc định true); check password truyền `false` — lambda capture
  by-reference không còn nguy cơ chạy async trễ trên stack đã chết, và khi sync session bị
  từ chối thì coi như password (chữ đi thẳng, không compose) thay vì mặc định "không phải".
- **Cache verdict blocked-context (disabled/password) từ `OnTestKeyDown` sang `OnKeyDown`**
  cùng phím/context (cửa sổ 100 ms): check password tốn 1 sync edit session, trước chạy 2
  lần mỗi đầu từ, giờ 1.
- **Single-instance tìm cửa sổ theo window class (`kMainWindowClass` share qua
  `settings-window.h`)** thay vì `FindWindowW` theo title "Goxvi" (có thể match nhầm cửa sổ
  bất kỳ trùng tên).
- Verify: build x64+x86 Release sạch (0 warning), 166/166 test pass cả 2 arch.
- Ghi chú rà soát: ACE `S-1-15-2-1` được Windows map generic→specific khi ghi
  (`icacls` xác nhận `RX`), nên check `alreadyGranted` hoạt động đúng — không sửa.

### Added
- **Checkbox trang cuối installer "Đặt Goxvi làm bàn phím tiếng Việt (VIE) ngay bây giờ"
  (tick sẵn)** — user VN chưa quen Win+Space nên cài xong bật bộ gõ luôn:
  - `Goxvi.exe --activate-tip`: chế độ helper không cửa sổ, gọi TSF
    `ITfInputProcessorProfileMgr::ActivateProfile(TF_PROFILETYPE_INPUTPROCESSOR,
    0x042A, CLSID_GoxviTextService, GUID_GoxviProfile, TF_IPPMF_FORSESSION |
    TF_IPPMF_ENABLEPROFILE)` → đặt Goxvi làm bộ gõ VIE đang dùng cho phiên user.
    COM-only (CoCreateInstance + GUID initguid), KHÔNG cần `msctf.lib`, KHÔNG chọc
    `HKCU\...\User Profile` (fragile). Windows tự nhớ lựa chọn gần nhất per-user.
  - `settings/`: thêm `activate-tip.cpp/.h`; `main.cpp` bắt cờ `--activate-tip` chạy
    helper rồi thoát (trước mutex/cửa sổ). CMake DÙNG LẠI `tsf/src/goxvi-guids.cpp`
    (initguid TU) + include `tsf/src` → DRY GUID, không khai lại.
  - `installer/goxvi.iss`: `[Run]` entry `--activate-tip` với flag
    **`runasoriginaluser`** (BẮT BUỘC — phải chạy ngữ cảnh user, không phải context
    admin của installer, thì ActivateProfile mới tác động đúng phiên đăng nhập),
    `postinstall nowait skipifsilent`; chạy sau khi regsvr32 đã đăng ký DLL. "Mở Goxvi
    Settings" chuyển thành `unchecked`. Verify: build x64+x86 sạch; chạy helper exit 0,
    input đổi sang Goxvi.
  - Giới hạn: hiệu quả tức thì cho phiên; KHÔNG phải override-default bền vững tuyệt đối
    (cố ý tránh registry hack). Cài im lặng bỏ qua (không UI checkbox).

### Changed
- **Thu chiều rộng Settings 290 → 228 (logic) cho tỉ lệ dáng đứng ~0.8 (W:H) thuận mắt
  (trước gần vuông); rút gọn checkbox "Bật gõ tắt và tự động thay thế" → "Bật gõ tắt".**
  Bề rộng control 3 tab co ĐỘNG theo page width (`GetClientRect`→logical): overview group
  box/radio fill ngang; shortcuts nút Thêm neo phải + ô "Thay bằng" giãn giữa + ListView +
  cột "Thay bằng" fill; about tiêu đề/phiên bản/ô nội dung fill. Không hardcode bề rộng nên
  không vỡ khi form hẹp. Verify: build x64+x86 sạch; screenshot 3 tab cân đối, không clip.
- **Bỏ nút "Đóng" + status line ở Settings; thu chiều cao 460 → 284 (logic):** đóng cửa sổ
  bằng nút X caption (mặc định Win); tab lấp gần hết cửa sổ (chỉ chừa lề 8px). Cảnh báo lúc
  nạp config (parse lỗi — hiếm) chuyển từ status line sang **MessageBox** để không nuốt lỗi.
  `PageContext.status = nullptr` (notifySaved/commit đã guard null). Bỏ WM_CTLCOLORBTN/STATIC
  ở cửa sổ chính (không còn child static/button). 2 tab Bảng gõ tắt / Giới thiệu vẫn cao động
  (fill đáy page area). Verify: build x64+x86 sạch; screenshot 3 tab gọn, không trống/không clip.
- **Settings app viết lại thuần C++/Win32 (bỏ WinUI 3 + .NET) — cắt dung lượng ~95%:**
  - Trước: `Goxvi.exe` là WinUI 3 self-contained ~160 MB/arch (runtime .NET + WASDK
    nhét kèm) → installer nén 45 MB. Sau: exe Win32 tĩnh `/MT` **~0.4 MB** →
    installer nén **2.4 MB**. Không phụ thuộc .NET/WASDK trên máy đích.
  - `settings/` là target CMake `goxvi-settings` (build tự động cùng `build.ps1`, bỏ bước
    dotnet). **Link `goxvi-core`** để DÙNG LẠI `parseConfigJson`/`serializeConfigJson` →
    schema JSON vẫn single-source ở core (không khai lại DTO như bản C# cũ).
  - UI: tab control comctl32 (`SysTabControl32`) 3 thẻ Tổng quan / Danh sách gõ tắt /
    Giới thiệu; radio Telex-VNI + kiểu bỏ dấu + checkbox gõ tắt; ListView + edit bar
    upsert-theo-trigger; lọc trigger a–z; cửa sổ cố định (không maximize); single-instance;
    PerMonitorV2 DPI. **Bỏ chọn Sáng/Tối** (theo hệ thống) cho gọn.
  - `config-store` C++: đọc/ghi atomic (temp + `MoveFileEx`) + stamp ACL `S-1-15-2-1`
    (ALL APPLICATION PACKAGES read) lên `%APPDATA%\Goxvi` cho TIP trong AppContainer (H3) —
    thay phần logic cũ ở C# `GoxviConfigStore`.
  - Xoá `settings/GoxviSettings` (C#/.sln) + `scripts/generate-settings-pri.ps1`
    (không còn WinUI → không cần resources.pri). Verify: bật app, đổi Telex→VNI, kiểu bỏ
    dấu, gõ tắt (add/upsert/lọc) → `config.json` ghi đúng; ACL granted; x64+x86 build sạch.
  - Nền đồng nhất một màu mặc định Windows (`COLOR_3DFACE` #F0F0F0), KHÔNG dùng trắng: page bg
    + `WM_CTLCOLORSTATIC` = 3DFACE; radio/checkbox tự tô 3DFACE nên khớp sẵn. 2 mấu chốt để
    hết hộp lệch quanh radio: (1) `SetWindowTheme(groupbox, "", "")` bỏ theme group box (theme
    group box tô ruột #F9F9F9 lệch); (2) BỎ `WS_CLIPCHILDREN` ở page để page phủ #F0F0F0 xuống
    dưới group box — nếu không, ruột group box (đã trong suốt) lộ tab-body #F9F9F9. Sample pixel:
    radio = ruột group box = page = #F0F0F0. Ô nhập/ListView giữ trắng (mặc định OS cho field).
  - Tab Tổng quan gọn lại: bỏ 3 dòng mô tả ngắn ở mỗi tuỳ chọn (Telex/VNI, kiểu bỏ dấu, gõ tắt);
    thu nhỏ group box khít radio → hết cảnh checkbox "Bật gõ tắt" đè lên dòng hướng dẫn phía dưới.
  - Đổi sang form dọc/hẹp (290×460 logic, trước 690×496 ~2/3 bề rộng): tuỳ chọn Telex/VNI +
    kiểu bỏ dấu là radio xếp DỌC (mỗi lựa chọn 1 dòng) trong group box. Tab Bảng gõ tắt: ô
    Viết tắt + Thay bằng + nút Thêm/Cập nhật nằm CHUNG 1 hàng, bỏ nút Huỷ (Esc vẫn huỷ sửa),
    bỏ dòng hướng dẫn; ListView 2 cột. Đổi tên tab **"Danh sách gõ tắt" → "Bảng gõ tắt"**.
    Tab Giới thiệu: thêm logo (icon app 40px, `STM_SETICON`) + tiêu đề nhỏ lại (15→12pt).
    Nút Đóng canh giữa (trước phải); status dời lên trên nút, canh giữa, message rút gọn.

## 2026-07-11

### Fixed
- **Click khi đang gõ làm MẤT chữ chưa commit (Telegram/Qt, Sublime, WinForms TextBox —
  IMM/CUAS host cancel composition string khi click; Notepad/TSF-native không bị):**
  1. *WH_MOUSE thread hook* (`mouse-click-commit-hook.*`): bắt button-down TRƯỚC window
     proc → `commitOnPointerDown()` = `ImmNotifyIME(CPS_COMPLETE)` (IMM host commit tại
     chỗ) + `commitCurrentWord(applyShortcut=false)` fallback (TSF-native). Result
     string đi đường posted message còn click đã rời queue → hook **eat click +
     reinject qua SendInput** (tag chung `kGoxviInjectedKeyTag`; posted > input) —
     chữ vào buffer trước, click xử lý sau. Đo từng bước bằng DBWIN listener +
     harness click thật (SendInput, DPI-aware; PostMessage giả không qua WH_MOUSE).
  2. *Commit lúc mất focus trước giờ chưa bao giờ chạy*: `RequestEditSession(TF_ES_SYNC)`
     ngoài key event bị `TF_E_SYNCHRONOUS` → `runEditSession` fallback
     `TF_ES_ASYNCDONTCARE`, lambdas capture by value (chạy trễ an toàn).
  3. *`ITfTextEditSink::OnEndEdit`* (advise theo focused context): caret thoát vùng
     composition (selection change không phải của mình) → commit không expand.
  4. *`OnCompositionTerminated`* dùng `ecWrite` (write cookie): host xóa text sau
     terminate → SetText lại display cuối.
  - Ghi chú đo được: TSF-side SetText trong terminate bị CUAS vứt (batch/discard);
    EM_REPLACESEL vào control bị wipe bởi cancel đang chạy dở; WinForms TextBox còn
    RECREATE HWND giữa chừng — mọi phương án "cứu sau" đều thua, phải commit TRƯỚC click.

### Added
- Harness click-khi-compose (scratchpad `test-click-during-composition.ps1`): gõ `banj`
  → click chuột thật chỗ khác → kỳ vọng `bạn` còn nguyên. Kèm kỹ thuật debug TIP Release:
  HKCU CLSID override sang DLL Debug (không cần admin) + DBWIN_BUFFER listener PowerShell.
- **Build x86 cho app 32-bit (Zalo Desktop = Electron WoW64):** preset CMake `x86`
  (build/x86). App 32-bit không load được DLL x64 → hiện VIE nhưng gõ ENG, Win+Space
  không kích hoạt lại được khi app focus. Đăng ký per-user (không cần admin):
  `HKCU\Software\Classes\WOW6432Node\CLSID\{...}\InprocServer32` → x86 DLL; profile +
  category CTF (HKLM) dùng chung 2 bitness, không phải đăng ký lại.

## 2026-07-10

### Fixed
- **Explorer F2 rename (CUAS edit control) — 4 bug một cụm:**
  1. *Gõ đè selection không thay text cũ*: composition giờ bắt đầu tại GetSelection
     (bỏ InsertTextAtSelection QUERYONLY — store CUAS trả range rỗng cạnh selection),
     kèm safety net xóa selection TSF-visible trong `ensureStarted` (app aware).
  2. *Enter chốt tên CŨ, ký tự đặc biệt nhảy lên trước từ (`abc`+`-` → `-abc`)*:
     terminator (space/Enter/dấu câu/số...) khi đang compose → **eat + commit +
     reinject** qua SendInput — mở rộng cơ chế nav-key defer cũ; text commit đi làn
     async chậm hơn phím thô ở CUAS edit nên pass-through là thua race. Ctrl/Alt/Win
     chord tách thành `CommitAndPassChord` (commit, không eat, giữ modifier thật).
  3. *Text cũ "ma" bôi xanh cạnh chữ mới cho tới commit*: phím chữ đầu từ (chỉ
     OnKeyDown) → `clearFocusedEditControlSelection()`: `EM_GETSEL` + `WM_CLEAR`
     đồng bộ thẳng vào focused edit control (TIP in-process). Bắt buộc Win32 vì CUAS
     (1) giấu selection thật khỏi GetSelection, (2) giam mọi document edit của TIP
     tới khi composition kết thúc (đo bằng harness).
  4. *`ar` bị tách thành `a`+`r` (dính gõ tắt thành "arồi")*: bỏ phương án inject
     VK_DELETE + bắn lại phím chữ — Explorer terminate composition non trẻ khi nhận
     text-change notification. WM_CLEAR đồng bộ (mục 3) thay thế, không còn race.
- `reinjectKey` chỉ set KEYEVENTF_EXTENDEDKEY cho nav key (Enter/dấu câu không phải E0).

### Added
- Test tự động không cần gõ tay: `scripts/test-explorer-rename-e2e.ps1` (rename thật
  trên Explorer: F2 → `ar` → Enter, kỳ vọng `ả.txt`; chờ user idle ≥4s, ép foreground
  AttachThreadInput + Alt ảo) và `scripts/test-cuas-edit-harness.ps1` (WinForms TextBox
  = EDIT/CUAS, kiểm tra ghost + commit; keybd_event vì SendKeys .NET bị chặn journal hook).

## 2026-07-09

### Added
- **Gõ tắt (shortcut/macro expansion)** kiểu UniKey: gõ trigger toàn chữ thường a-z
  (vd `trc`), hết từ tự thay bằng expansion tự do có dấu (vd `trước`). Khớp toàn từ trên
  raw keys, case tuyệt đối (`TRC`/`Trc` không khớp). Cắm 1 hàm `commitCurrentWord(applyShortcut)`
  — expand trên MỌI đường commit (space/Enter/nav-key/Alt-Tab/view-change/Deactivate). Esc
  bỏ qua (raw thắng); Literal-backspace không expand (cờ `rawKeysExact`). Bật/tắt riêng
  `shortcutsEnabled`, phụ thuộc IME đang bật.
  - Core: module thuần `shortcut-expander` (`matchShortcut`) + `utf8-utf16-conversion`
    (JSON UTF-8 ↔ `ShortcutEntry` wstring, surrogate cho non-BMP) + schema v1 thêm
    `shortcutsEnabled`/`shortcuts`. +70 unit tests (161/161 tổng).
  - TSF: `CompositionManager::commitWithReplacement` (SetText + caret-to-end +
    EndComposition trong 1 edit session → phím kết thúc từ rơi SAU expansion).
  - Settings: MainWindow giữ toggle "Bật gõ tắt" + nút mở `ShortcutsWindow` (form riêng sửa
    danh sách — thêm/sửa/xoá, trigger auto-lowercase `MaxLength=32`) để main window gọn;
    Reset GIỮ danh sách gõ tắt (Decision 10).

### Changed
- **Simple Telex**: bỏ quy tắc `w` đứng một mình → ư; chỉ còn aw/ow/uw/uow
  (gõ tiếng Anh "web/wow" không biến dạng). (`ab1cf9b`)
- **Esc = hủy gõ tiếng Việt**: từ đang compose hoàn nguyên đúng phím thô rồi commit,
  Esc bị nuốt; idle thì pass qua app. Thay quyết định L1 cũ (Esc=commit). (`ab1cf9b`)
- **Foreign backspace tự hồi tiếng Việt**: pop raw key rồi replay phần còn lại,
  hợp lệ toàn bộ → quay về Composing (`nhuwg`+BS → `như`; `vieejtk`+BS → `việt`).
  Literal vẫn pop 1:1 (M3). (`4a1cfa5`)
- **Nav-key defer thống nhất mọi app**: PageUp/Down, mũi tên, Home/End, Ins/Del khi
  đang compose → eat + commit + reinject (SendInput, tag GetMessageExtraInfo) — fix
  race "cd"+PageUp → "pm2 listcd" trên SSH/terminal, cùng cơ chế MS Telex built-in.
  (`8bab328` conhost-only → `13abb0c` direct mode → chốt `c22195e` uniform, bỏ direct)

### Fixed
- `register-tip.ps1`/`unregister-tip.ps1`: pure ASCII (PS 5.1 đọc BOM-less UTF-8 như
  ANSI → ParserError) (`b8a43d5`); tự elevate qua UAC (`8651a18`); đợi regsvr32 GUI exe
  lấy exit code thật — hết báo fail giả (`6bfebd2`).

## 2026-07-08

### Added
- Phase 1: scaffolding, CMake presets x64, CRT tĩnh toàn cục, gtest FetchContent. (`a27d438`)
- Phase 2: Telex engine core — state machine 4 trạng thái, syllable parser (gi/qu M2),
  tone placement kiểu cũ/mới, undo lặp modifier, buffer cap 32. (`59f2fbd`)
- Phase 3: TSF COM TIP — 4 exports, class factory, key sink pass-through, registration
  CLSID + profile 0x042A + categories (không secure mode L5). (`e4964b4`)
- Phase 4: hidden composition (display attribute rỗng), key classification (không
  ToUnicodeEx M4), commit paths đầy đủ, password/disabled context. (`d7cb4ec`)
- Phase 5: config.json schema v1 + serializer tolerant (core) + file watcher lazy mtime
  2s (tsf) + enabled=false pass-through; vendor nlohmann/json 3.11.3. (`9f7f5ac`)
- Phase 6: GoxviSettings WinUI 3 unpackaged self-contained — atomic save, ACL
  AppContainer S-1-15-2-1 (H3), single instance; build bằng dotnet CLI với
  EnableCoreMrtTooling=false + generate-settings-pri.ps1 (resources.pri bắt buộc). (`17e96a6`)

### Milestones
- TIP đăng ký thành công, user gõ tiếng Việt thực tế trong Notepad/terminal OK.
