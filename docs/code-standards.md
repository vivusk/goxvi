# Goxvi — Code Standards

Cập nhật: 2026-11-15 (thêm Objective-C++ / macOS)

## Chung

- File name kebab-case, tự mô tả (`vietnamese-syllable-parser.cpp`, `config-store.cpp`).
- File < ~200 dòng, tách module theo concern (parser / transform / placement / engine...).
- YAGNI–KISS–DRY; comment giải thích "vì sao" + reference finding của plan (H2, M3, R2-M1...).
- Commit: conventional commits (`feat(core): ...`, `fix(scripts): ...`), không AI reference.

## C++ (core + tsf)

- C++20, `/W4 /utf-8 /EHsc`, `UNICODE`; CRT tĩnh toàn cục (`CMAKE_MSVC_RUNTIME_LIBRARY`,
  H2 — không được đổi per-target).
- `core/` KHÔNG include Windows.h trong logic; API public duy nhất ở `core/include/goxvi/`.
- Source văn bản tiếng Việt dùng literal UTF-8 trực tiếp (`L"việt"`) — an toàn nhờ `/utf-8`.
- COM: mọi method noexcept-safe (try/catch tại boundary, không leak exception);
  refcount qua Interlocked; smart pointer `Microsoft::WRL::ComPtr`; mọi thao tác text
  qua edit session `TF_ES_SYNC | TF_ES_READWRITE`.
- GUID định nghĩa 1 lần trong TU initguid duy nhất (`goxvi-guids.cpp`).
- KHÔNG log nội dung gõ ở Release (`GOXVI_LOG` compile-out — security bắt buộc).
- Test: GoogleTest table-driven; case gõ thực tế mới → thêm vào bảng, không sửa kiến trúc.

## PowerShell (scripts/)

- **Pure ASCII bắt buộc** — PS 5.1 đọc .ps1 BOM-less UTF-8 như ANSI, ký tự đa byte
  gây ParserError (đã dính 2 lần với em dash).
- GUI exe (regsvr32...) phải `Start-Process -Wait -PassThru` lấy ExitCode — `&` không đợi,
  `$LASTEXITCODE` stale.
- Script cần admin: tự elevate qua `Start-Process -Verb RunAs` (UAC prompt rõ ràng).

## C++ Win32 (settings/)

- App thuần Win32 (`goxvi-settings` → `Goxvi.exe`), KHÔNG .NET/WinUI/WASDK.
- Link `goxvi-core` để DÙNG LẠI `parseConfigJson`/`serializeConfigJson` — schema JSON
  single-source ở core, KHÔNG khai lại DTO. Ghi bằng nlohmann → UTF-8 thuần (không escape `\u`).
- Ghi file: LUÔN temp-then-rename (atomic, `MoveFileEx REPLACE_EXISTING`) — DLL đọc song song;
  stamp ACL `S-1-15-2-1` (ALL APPLICATION PACKAGES read) lên `%APPDATA%\Goxvi` cho TIP trong
  AppContainer (H3).
- Chuỗi tiếng Việt để nguyên trong wide-literal nhờ `/utf-8` toàn cục (không cần escape).
- Manifest (comctl6 + PerMonitorV2) nhúng qua `.rc`, link `/MANIFEST:NO` (không sinh manifest kép).
- Không đụng core/tsf logic; chỉ chung schema qua core lib.

## Objective-C++ (mac/)

- `.mm` biên dịch với `-fobjc-arc` (ARC) — không retain/release tay; object phải sống suốt
  `[NSApp run]` (vd `IMKServer`) giữ bằng strong ref toàn cục. CF object (`TIS*`) vẫn `CFRelease` tay.
- Logic phím viết C++ thuần trong `goxvi-mac-logic` (không include Cocoa) để unit-test (K2);
  `.mm` chỉ dịch NSEvent ↔ `CompositionSession` ↔ IMK client. Thêm hành vi gõ → sửa/thêm test
  ở `mac/tests/` trước.
- Thao tác IMK client (`selectedRange`, `insertText:replacementRange:`, `setMarkedText`, bundle ID)
  chỉ qua `mac/src/client-text-editor.h|.mm`; controller không gọi thẳng client. Hành vi chèn trực
  tiếp mới (phím, ⌫, gõ tắt...) → thêm test `DirectInsertTracker` (`mac/tests/direct-insert-tracker*-tests.cpp`)
  trước. App mới cần marked text → thêm bundle ID vào `mac/src/insert-mode-policy.cpp` + test policy.
- K1: `std::wstring` của core là UTF-16 code unit; đổi `NSString` ↔ wide chỉ qua
  `mac/src/wide-nsstring.h` (từng phần tử ↔ `unichar`), không `NSUTF32StringEncoding`; không
  viết ký tự ngoài BMP trong `L"..."` ở `mac/` và `tests/` (dùng `utf8ToWide` hoặc surrogate escape).
- Callback IMK (`goxvi-input-controller.mm`): bắt exception tại boundary (`try/catch(...)` → `abandon()` + trả `NO`),
  không để exception thoát ra Objective-C runtime. Chỉ chạy trên main thread.
- **KHÔNG log nội dung gõ** (`NSLog` chỉ cho lỗi hệ thống, không kèm chữ/phím) — cùng lý do bên Win.
- Config ghi atomic (`NSDataWritingAtomic`), schema qua `parseConfigJson`/`serializeConfigJson`
  của core — không khai DTO lại. Chuỗi tiếng Việt để nguyên trong `@"..."`/`L"..."` (UTF-8 source).
- Cờ clang chung `-Wall -Wextra` (root CMake, khối không-MSVC); không thêm cờ MSVC vào đường Mac.

## Shell (mac/scripts/)

- `#!/bin/bash` + `set -euo pipefail`; đường dẫn tính từ `$(dirname "$0")`, có dấu cách
  (`Input Methods`) → luôn quote.
- Mã thoát ≠ 0 chỉ khi build / copy / `codesign` / `TISRegisterInputSource` lỗi; `--status` chỉ in
  thông tin (bật source có thể cần System Settings hoặc log out/in — không lấy làm mã thoát).
- Chỉ đụng `~/Library/Input Methods/Goxvi.app`; `killall Goxvi || true` trước khi thay bundle.

## Build môi trường (máy dev hiện tại)

- CMake/ctest: dùng bundle VS 2022 (không có trong PATH).
- SDK 26100 không còn `msctf.lib` — không link, GUID qua initguid TU.
- Settings app build cùng CMake (`goxvi-settings`), không còn bước dotnet/makepri/resources.pri.
- DLL bị app lock khi rebuild: rename `goxvi-tsf.dll` → `.oldN.dll` rồi build lại.
- macOS: Xcode clang + CMake ≥ 3.26 (Unix Makefiles, không cần ninja); preset `mac-debug` / `mac-release`.
