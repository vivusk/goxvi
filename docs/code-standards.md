# Goxvi — Code Standards

Cập nhật: 2026-07-09

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

## Build môi trường (máy dev hiện tại)

- CMake/ctest: dùng bundle VS 2022 (không có trong PATH).
- SDK 26100 không còn `msctf.lib` — không link, GUID qua initguid TU.
- Settings app build cùng CMake (`goxvi-settings`), không còn bước dotnet/makepri/resources.pri.
- DLL bị app lock khi rebuild: rename `goxvi-tsf.dll` → `.oldN.dll` rồi build lại.
