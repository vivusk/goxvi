# CLAUDE.md

Guidance for Claude Code when working in this repo. Read `./README.md` and `docs/`
before planning. Global workflow rules in `%USERPROFILE%/.claude/CLAUDE.md` still apply.

## Dự án

Goxvi — bộ gõ tiếng Việt (Telex + VNI) cho Windows 11, dựng trên Text Services
Framework (TSF). Engine thuần C++20 tách khỏi lớp COM/TSF để unit-test không cần Windows.
Composition ẩn (không gạch chân, UX kiểu UniKey). Cấu hình qua `%APPDATA%\Goxvi\config.json`
+ app cấu hình thuần C++/Win32 (`Goxvi.exe`, ~0.4 MB, KHÔNG .NET).

## Layout

| Thư mục | Nội dung |
|---|---|
| `core/` | `goxvi-core` — Telex/VNI state machine thuần C++20, KHÔNG Windows API trong logic. API public ở `core/include/goxvi/`. |
| `tsf/` | `goxvi-tsf.dll` — TSF TIP (COM), thin shim over core. |
| `tests/` | `goxvi-core-tests` — GoogleTest (FetchContent, pinned v1.14.0). |
| `settings/` | `goxvi-settings` — app cấu hình thuần Win32 (comctl32 tab UI), link `goxvi-core` để tái dùng serializer JSON, build ra `Goxvi.exe`. KHÔNG .NET/WinUI. |
| `installer/` | `goxvi.iss` (Inno Setup 6) → `dist/goxvi-setup-<ver>.exe`. |
| `scripts/` | build / register-tip / build-installer (PowerShell, pure ASCII). |
| `third_party/` | Vendored headers (nlohmann/json). |

`VERSION` (repo root) = single source of truth cho phiên bản (settings/CMakeLists.txt sinh
`settings-version.h` + installer .iss đọc chung).

## Build

```powershell
# C++ (mặc định -Arch All: build CẢ x64 + x86 — bắt buộc giữ đồng bộ, xem dưới)
.\scripts\build.ps1 -Config Release            # TIP + Goxvi.exe, x64 + x86
.\scripts\build.ps1 -Config Debug -Test        # + chạy ctest

# Settings app (goxvi-settings) là target CMake -> build TỰ ĐỘNG cùng build.ps1
# (không còn bước dotnet riêng). Build lẻ:
#   cmake --build --preset x64-release --target goxvi-settings

# Installer end-to-end (Release binaries + Inno compile)
.\scripts\build-installer.ps1
```

Yêu cầu: VS 2022 (MSVC v143, cmake/ctest lấy từ bundle VS — KHÔNG có trong PATH), Windows 11 SDK.
CRT link tĩnh `/MT` toàn cục (DLL load vào mọi process). Xem [README.md](README.md) cho chi tiết x86/Win10.

**QUAN TRỌNG — sửa code TSF phải build CẢ x64 lẫn x86**: app 64-bit load DLL x64, app
32-bit (Zalo/Electron WoW64...) load DLL x86 — thiếu 1 bản là 2 loại app lệch hành vi.
`-Arch All` (mặc định) lo việc này.

## Rebuild DLL khi bị app lock (BẮT BUỘC NHỚ)

`goxvi-tsf.dll` đã đăng ký được TIP nạp **in-process** vào mọi app đang gõ (explorer,
browser, VSCode, Telegram...), nên linker không ghi đè được → `LNK1104 cannot open file`.
Windows cho **đổi tên** DLL đang bị load (khoá ghi/xoá nhưng không khoá rename), giải phóng
tên file cho bản mới:

```powershell
# đổi tên bản đang bị khoá → .old, rồi build lại (linker ghi DLL mới vào path cũ)
foreach ($t in @(
  "build\x64\tsf\Release\goxvi-tsf.dll",
  "build\x86\tsf\Release\goxvi-tsf.dll")) {
  if (Test-Path $t) { Move-Item $t "$t.old" -Force }
}
.\scripts\build.ps1 -Config Release
```

Sau build: **đóng & mở lại** app cần test (hoặc log off/on) để nạp DLL mới — KHÔNG cần
đăng ký lại (path CLSID không đổi). File `.old` xoá được sau khi mọi process nhả (log off).
Tìm process đang giữ: `Get-Process | ? { $_.Modules.FileName -contains (Resolve-Path <dll>) }`.

## Đăng ký TIP (1 lần, admin)

```powershell
.\scripts\register-tip.ps1 -Config Release     # regsvr32; tự elevate UAC
.\scripts\unregister-tip.ps1 -Config Release
```

Registry giữ **path tuyệt đối của 1 config** → đổi Debug↔Release phải register lại đúng
`-Config`. Rebuild cùng config KHÔNG cần re-register.

## Branching & Release (BẮT BUỘC theo)

Làm việc hằng ngày trên **`dev`** (commit chi tiết thoải mái). **`main` = mỗi đợt release
CHỈ 1 commit** — KHÔNG lôi mấy chục commit chi tiết của dev sang. Khi dev xong 1 đợt,
"gộp" trạng thái dev thành đúng 1 commit trên main bằng **snapshot** (không cần chung gốc,
không bao giờ conflict — hợp repo 1 người, local, chưa remote):

```bash
# Bump VERSION (repo root) TRƯỚC — csproj + goxvi.iss + tag đọc chung 1 nguồn.
# --- LẦN ĐẦU (tạo main) ---
git switch --orphan main
git read-tree -u --reset dev        # main := đúng y cây dev (kể cả file dev đã xoá)
git commit -m "release vX.Y.Z"      # main đúng 1 commit
git tag vX.Y.Z
git switch dev
# --- CÁC LẦN SAU ---
git switch main
git read-tree -u --reset dev        # cập nhật main = dev mới nhất
git commit -m "release vX.Y.Z"      # main +1 commit
git tag vX.Y.Z
git switch dev
```

- Đừng dùng `git merge dev` / merge thường vào main (kéo hết commit dev sang → mất mục đích).
- `git merge --squash dev` cũng cho 1 commit NHƯNG cần main & dev **chung gốc**; snapshot ở
  trên không cần, ưu tiên dùng.
- Nếu sau này lên GitHub + muốn PR "Squash and merge": lúc đó chuyển sang mô hình chung-gốc
  (merge-squash), vì snapshot tạo lịch sử rời → PR báo "unrelated histories".

## Conventions (chi tiết ở docs/code-standards.md)

- File name kebab-case, tự mô tả; file < ~200 dòng, tách theo concern.
- Comment giải thích "vì sao" + tham chiếu finding của plan (H2, M3, R2-M1, Decision N...).
- Commit conventional (`feat(core):`, `fix(tsf):`), KHÔNG reference AI.
- **C++**: C++20 `/W4 /utf-8 /EHsc UNICODE`; CRT tĩnh (không đổi per-target); `core/` không
  include Windows.h trong logic; COM method noexcept-safe, `ComPtr`, text qua edit session
  `TF_ES_SYNC|TF_ES_READWRITE`; GUID initguid TU duy nhất (`goxvi-guids.cpp`);
  **KHÔNG log nội dung gõ ở Release** (`GOXVI_LOG` compile-out — security).
- **PowerShell**: PURE ASCII bắt buộc (PS 5.1 đọc BOM-less UTF-8 như ANSI → ParserError với
  ký tự đa byte). GUI exe phải `Start-Process -Wait -PassThru` lấy ExitCode.
- **Settings (Win32)**: app cấu hình thuần Win32 trong `settings/`, link `goxvi-core` để
  DÙNG LẠI `parseConfigJson`/`serializeConfigJson` (schema JSON single-source ở core, KHÔNG
  khai lại DTO). Ghi config LUÔN temp-then-rename (atomic, DLL đọc song song). Phải stamp ACL
  `S-1-15-2-1` (ALL APPLICATION PACKAGES, read) lên `%APPDATA%\Goxvi` để TIP trong AppContainer
  (UWP/Search) đọc được config (H3). Chuỗi tiếng Việt để nguyên trong wide-literal — `/utf-8`
  toàn cục lo phần mã hoá (khác PowerShell, .cpp KHÔNG cần ASCII).

## SDK gotchas

- SDK 26100 không còn `msctf.lib` → không link, GUID qua initguid TU.
- Settings native: manifest nhúng qua `.rc` (comctl6 visual styles + PerMonitorV2 DPI); link
  `/MANIFEST:NO` để không đụng manifest mặc định của linker. `GetWindowText` KHÔNG đọc được
  edit-control cross-process (chỉ caption) — test tự động phải kiểm qua `config.json`, không
  đọc text control từ tiến trình khác.

## Docs

`docs/`: `system-architecture.md` (kiến trúc + các Decision), `codebase-summary.md`,
`code-standards.md`, `project-changelog.md`, `development-roadmap.md`.
Plans: `plans/` (LOCAL-ONLY — đã gitignore, không publish; clone từ GitHub sẽ không có
thư mục này).
