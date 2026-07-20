# Goxvi — Vietnamese IME for Windows 11 (Telex + VNI)

**Website: [goxvi.com](https://goxvi.com/)**

Bộ gõ tiếng Việt cho Windows 11, xây trên Text Services Framework (TSF). Hỗ trợ hai
kiểu gõ, chọn trong settings app (default: Telex):
- **Simple Telex**: `aa ee oo dd aw ow uw uow` + dấu `s f r x j`, `z` xóa dấu — KHÔNG có
  `w` đứng một mình thành ư (gõ tiếng Anh không bị biến dạng).
- **VNI**: `a6/e6/o6` (â/ê/ô), `o7/u7` (+`uo7`→ươ), `a8` (ă), `d9` (đ), số `1-5` = dấu thanh.

`Esc` hủy gõ tiếng Việt
của từ đang gõ (khôi phục đúng phím thô, kiểu UniKey). Gõ sai → xóa lùi tự bật lại
tiếng Việt khi phần còn lại hợp lệ (`nhuwg`+⌫ → `như`). Phím điều hướng khi đang gõ
dở được chốt chữ trước rồi mới chuyển cho app (PageUp search history trên SSH đúng
prefix). **Gõ tắt** (macro kiểu UniKey): gõ trigger toàn chữ thường (vd `trc`), hết từ
tự thay bằng chuỗi mở rộng có dấu (vd `trước`); quản lý danh sách trong settings app,
bật/tắt riêng. Chi tiết kiến trúc: [docs/system-architecture.md](docs/system-architecture.md).
Composition ẩn (không gạch chân, UX như UniKey), engine thuần tách riêng để unit-test
không cần TSF, cấu hình qua JSON + settings app Win32 native (link `goxvi-core`).

## Cài đặt (người dùng)

Tải `goxvi-setup-<version>.exe` ở trang **Releases** và chạy (cần quyền admin — installer
đăng ký bộ gõ machine-wide, cả x64 lẫn x86 cho app 32-bit như Zalo). Cài xong chuyển bàn
phím sang **Goxvi** bằng `Win + Space`. Gỡ qua Settings → Apps như app thường.

Tự build installer từ source: `.\scripts\build-installer.ps1` (cần Inno Setup 6) →
`dist\goxvi-setup-<version>.exe`.

## Cấu trúc

| Thư mục | Nội dung |
|---|---|
| `core/` | `goxvi-core` — Telex state machine thuần C++20, không Windows API trong logic |
| `tsf/` | `goxvi-tsf.dll` — TSF text input processor (COM), thin shim over core |
| `tests/` | `goxvi-core-tests` — GoogleTest (FetchContent, pinned v1.14.0) |
| `settings/` | `goxvi-settings` → `Goxvi.exe` — Win32 native (~0.4 MB, /MT tĩnh, KHÔNG .NET), link `goxvi-core`, chỉnh `%APPDATA%\Goxvi\config.json` |
| `installer/` | `goxvi.iss` (Inno Setup 6) → `dist/goxvi-setup-<ver>.exe` |
| `docs/` | Kiến trúc, chuẩn code, changelog |
| `third_party/` | Vendored headers (nlohmann/json) |
| `scripts/` | build / register-tip / build-installer (PowerShell) |

## Build (C++)

Yêu cầu: VS 2022 (MSVC v143), CMake ≥ 3.26, Windows 11 SDK.

```powershell
.\scripts\build.ps1 -Config Debug -Test          # mặc định -Arch All: build CẢ x64 + x86
.\scripts\build.ps1 -Config Release -Arch x64    # chỉ 1 arch
# settings app (Goxvi.exe) build tự động cùng build.ps1 — không còn switch riêng
# hoặc thủ công (1 arch):
cmake --preset x64
cmake --build --preset x64-debug
ctest --preset x64-debug
```

`-Arch`: `x64` | `x86` | `All` (mặc định **All** — luôn build kèm bản 32-bit cho Win 10 x86).
Settings app (`Goxvi.exe`) build tự động theo từng arch cùng TIP. Test x86 chạy qua WoW64.

CRT link tĩnh (`/MT`) toàn cục — DLL load vào mọi process nên không phụ thuộc VCRuntime.

## Đăng ký TIP (1 lần, cần admin)

```powershell
.\scripts\register-tip.ps1 -Config Debug   # regsvr32 goxvi-tsf.dll
.\scripts\unregister-tip.ps1 -Config Debug
```

Lưu ý: registry giữ **path tuyệt đối của 1 config** — đổi Debug↔Release phải chạy lại
script register với `-Config` tương ứng. Rebuild cùng config không cần đăng ký lại,
chỉ cần đóng app đang giữ DLL (Notepad...).

## Config

`%APPDATA%\Goxvi\config.json` (schema v1):

```json
{
  "version": 1,
  "toneStyle": "old",
  "inputMethod": "telex",
  "shortcutsEnabled": true,
  "autoCorrectEnabled": true,
  "shortcuts": [ { "trigger": "trc", "expansion": "trước" } ]
}
```

`autoCorrectEnabled` (mặc định bật): tự sửa 2 lỗi đảo phím khi gõ nhanh — dấu móc
`w` (ơ/ư/ă) ra trước nguyên âm (`twosi`/`twsoi` → `tới`), và coda cuối `ng`/`nh`/`ch`
gõ ngược thành `gn`/`hn`/`hc` (`cuxgn` → `cũng`, `nhahn` → `nhanh`, `sahcs` → `sách`). Chỉ sửa lúc chốt
từ và chỉ với từ không hợp lệ, không đụng gõ tay/tiếng Anh (có blocklist tiếng Anh).

`shortcuts`: mảng `{trigger, expansion}` (trigger toàn chữ thường a-z, expansion tự do
có dấu). `shortcutsEnabled` bật/tắt gõ tắt độc lập. Bật/tắt cả bộ gõ là việc của
Windows — chuyển bàn phím bằng `Win + Space`.

DLL tự reload lazy theo mtime (throttle 2s, check lúc bắt đầu từ mới). File hỏng/thiếu →
dùng defaults. Settings app ghi atomic (temp + rename) và grant ACL đọc cho
`ALL APPLICATION PACKAGES` để TIP trong app UWP đọc được config.

## Settings app (phase 6)

App thuần Win32 (`goxvi-settings` → `Goxvi.exe`), link `goxvi-core` để dùng lại
`parseConfigJson`/`serializeConfigJson` (schema JSON single-source, không khai lại DTO).
Build tự động cùng `.\scripts\build.ps1`; build lẻ:

```powershell
cmake --build --preset x64-release --target goxvi-settings
# ra: build\x64\settings\Release\Goxvi.exe
```

UI: form dọc/hẹp, tab control comctl32 (SysTabControl32) 3 thẻ Tổng quan / Bảng gõ tắt / Giới thiệu;
cửa sổ cố định (không maximize), single-instance, DPI PerMonitorV2. Không có .NET/WinUI/WASDK,
không cần resources.pri.

### Hỗ trợ Windows 10 32-bit (OS x86)

Windows 11 chỉ có bản 64-bit; hỗ trợ 32-bit **chỉ dành cho Windows 10 edition x86 thật**
(≥ 1809). Trên OS đó binary x86 chạy **native** (không WoW64), nên cần build cả TIP lẫn
Settings ra x86 — Win32 tĩnh `/MT` nên máy đích không cần cài .NET/runtime.

```powershell
# TIP + Settings x86 (Goxvi.exe build kèm theo preset)
cmake --preset x86; cmake --build --preset x86-release
# ra: build\x86\tsf\Release\goxvi-tsf.dll + build\x86\settings\Release\Goxvi.exe
```

`register-tip.ps1`/`unregister-tip.ps1` tự chọn arch theo bitness của OS chạy script
(`[Environment]::Is64BitOperatingSystem`): OS 32-bit → đăng ký DLL x86 (CLSID vào thẳng
`HKLM\...\CLSID`, không có Wow6432Node). Chú ý: **chưa test trên OS 32-bit thật** — mới xác
minh binary x86 chạy qua WoW64 trên máy 64-bit (cùng tập lệnh nên tương đương native x86).

## Docs

Kiến trúc + các quyết định thiết kế: [docs/system-architecture.md](docs/system-architecture.md).
Toàn bộ tài liệu trong [docs/](docs/).

## Giấy phép

[MIT](LICENSE).
