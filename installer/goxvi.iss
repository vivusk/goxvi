; Goxvi installer (Inno Setup 6). Targets 64-bit Windows: installs the x64 TIP
; (Notepad, VSCode, Office...) AND the x86 TIP (32-bit apps like Zalo/Electron).
; Build with scripts\build-installer.ps1 (produces dist\goxvi-setup-<ver>.exe).
;
; NOTE: registration is machine-wide via regsvr32 -> DllRegisterServer, which
; writes HKLM\SOFTWARE\Classes\CLSID (x86 auto-redirects to Wow6432Node) and the
; TSF profile/categories through msctf. Requires admin.

#define AppName "Goxvi"
; Single source of truth: repo-root VERSION file (also usable by CMake/csproj).
#define VerFile FileOpen(AddBackslash(SourcePath) + "..\VERSION")
#define AppVersion Trim(FileRead(VerFile))
#expr FileClose(VerFile)
#if AppVersion == ""
  #error VERSION file is empty or missing at repo root
#endif
#define AppPublisher "Goxvi"
; Native Settings app (Goxvi.exe) - a single ~0.5 MB static exe (was a ~160 MB
; self-contained WinUI 3 folder). Built by the x64 CMake preset.
#define SettingsExe "..\build\x64\settings\Release\Goxvi.exe"

[Setup]
; AppId is the installer identity (Add/Remove Programs) - distinct from the TIP CLSID.
AppId={{B2E7C1A4-9D3F-4A62-8E10-6C5B7F2A9D48}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
PrivilegesRequired=admin
; 64-bit OS only (installs both x64 + x86 TIP for that OS). For a genuine 32-bit
; Windows 10 build a separate all-x86 installer.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
Compression=lzma2
SolidCompression=yes
OutputDir=..\dist
OutputBaseFilename=goxvi-setup-{#AppVersion}
UninstallDisplayIcon={app}\Settings\Goxvi.exe
; Icon of the setup .exe itself (same icon as the Settings app).
SetupIconFile=..\settings\assets\goxvi.ico
WizardStyle=modern
; Logo Goxvi ở góc phải trên các trang wizard (BMP, nhiều cỡ cho các mức DPI).
WizardSmallImageFile=wizard-small-55.bmp,wizard-small-83.bmp,wizard-small-110.bmp,wizard-small-138.bmp

[Languages]
; Toàn bộ khung wizard (nút Next/Back/Cancel, tiêu đề trang chuẩn...) tiếng Việt.
; Vietnamese.isl là bản dịch "unofficial" của Inno (không ship sẵn) nên vendor kèm repo.
Name: "vi"; MessagesFile: "Vietnamese.isl"

[Files]
; x64 TIP -> {app}\goxvi-tsf.dll
Source: "..\build\x64\tsf\Release\goxvi-tsf.dll"; DestDir: "{app}"; Flags: ignoreversion
; x86 TIP -> {app}\x86\goxvi-tsf.dll (for 32-bit host apps)
Source: "..\build\x86\tsf\Release\goxvi-tsf.dll"; DestDir: "{app}\x86"; Flags: ignoreversion
; Settings app: single native Goxvi.exe (icon + manifest embedded; no runtime).
Source: "{#SettingsExe}"; DestDir: "{app}\Settings"; Flags: ignoreversion

[Icons]
Name: "{group}\Goxvi Settings"; Filename: "{app}\Settings\Goxvi.exe"
Name: "{group}\Uninstall Goxvi"; Filename: "{uninstallexe}"

[Run]
; Đăng ký TIP (regsvr32) + kích hoạt cho user (--activate-tip) chuyển HẾT sang
; [Code]/RegisterAndActivateTip tại ssPostInstall: chạy VÔ ĐIỀU KIỆN (kể cả
; silent, hết kiểu checkbox bỏ tick là mất như <= 0.0.3), TUẦN TỰ (activate chắc
; chắn sau regsvr32) và CÓ CHỜ + kiểm tra exit code từng bước (trước đây nowait
; nên fail im lặng). Ở đây chỉ còn tùy chọn mở Settings sau khi cài.
Filename: "{app}\Settings\Goxvi.exe"; Description: "Mở Goxvi Settings"; Flags: postinstall nowait skipifsilent unchecked

[UninstallRun]
; Gỡ Goxvi khỏi language list của user TRƯỚC khi unregister DLL. Best-effort:
; [UninstallRun] không hỗ trợ runasoriginaluser (Inno chỉ cho ở [Run]); với UAC
; cùng tài khoản thì HKCU vẫn là hive của user đó nên vẫn dọn đúng — chỉ lệch khi
; uninstall bằng tài khoản admin KHÁC (chấp nhận, Windows tự ẩn TIP không còn nữa).
Filename: "{app}\Settings\Goxvi.exe"; Parameters: "--deactivate-tip"; Flags: runhidden waituntilterminated; RunOnceId: "RemoveGoxviInputMethod"
; Unregister before files are removed (reverse order not required).
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\goxvi-tsf.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregGoxviX64"
Filename: "{syswow64}\regsvr32.exe"; Parameters: "/u /s ""{app}\x86\goxvi-tsf.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregGoxviX86"

[UninstallDelete]
; Dọn các bản DLL cũ bị đổi tên khi update tại chỗ (FreeLockedDll). Còn khoá thì
; Inno bỏ qua, Windows tự nhả sau khi log off.
Type: files; Name: "{app}\goxvi-tsf.dll.old*"
Type: files; Name: "{app}\x86\goxvi-tsf.dll.old*"

[Messages]
; Trang cuối wizard. Nhấn mạnh: gõ tiếng Việt qua bộ gõ tích hợp Windows (Win+Space),
; KHÔNG cần chạy app Goxvi; app chỉ để cấu hình. App self-contained nên KHÔNG cần .NET.
FinishedLabel=Cài đặt xong! Goxvi đã được thêm vào Windows và đặt làm kiểu gõ mặc định — gõ tiếng Việt được ngay, KHÔNG cần chạy ứng dụng Goxvi. Nhấn Ctrl+Shift (hoặc Win+Space) để chuyển ENG ↔ VIE.%n%nChỉ khi cần đổi kiểu gõ hoặc cấu hình thì mới mở ứng dụng Goxvi (đã kèm sẵn mọi thứ, không phải cài thêm gì).%n%n(Nếu chưa thấy Goxvi: Settings > Time & language > Language & region > Vietnamese > Language options > Add a keyboard.)%n%n(Nếu vừa CẬP NHẬT bản mới: đóng và mở lại app đang gõ — hoặc đăng xuất/đăng nhập — để nạp phiên bản mới.)

[Code]
{ Two wizard pages to seed %APPDATA%\Goxvi\config.json (input method + tone
  style). Config is only written on a FRESH install - an existing user config is
  left untouched so reinstall/upgrade never resets the user's choices. }
var
  InputMethodPage: TInputOptionWizardPage;
  ToneStylePage: TInputOptionWizardPage;

procedure InitializeWizard;
begin
  InputMethodPage := CreateInputOptionPage(wpSelectDir,
    'Kiểu gõ', 'Chọn cách gõ dấu tiếng Việt',
    'Có thể đổi lại sau trong Goxvi Settings.',
    True, False);
  InputMethodPage.Add('Telex  —  gõ dấu bằng chữ cái (aa → â, as → á)');
  InputMethodPage.Add('VNI  —  gõ dấu bằng phím số (a6 → â, a1 → á)');
  InputMethodPage.SelectedValueIndex := 0; { telex }

  ToneStylePage := CreateInputOptionPage(InputMethodPage.ID,
    'Kiểu bỏ dấu', 'Chọn cách đặt dấu thanh',
    'Có thể đổi lại sau trong Goxvi Settings.',
    True, False);
  ToneStylePage.Add('Kiểu cũ  —  đặt trên nguyên âm đầu:  hòa,  thúy');
  ToneStylePage.Add('Kiểu mới  —  đặt theo âm chính:  hoà,  thuý');
  ToneStylePage.SelectedValueIndex := 0; { old }
end;

function SelectedInputMethod: String;
begin
  if InputMethodPage.SelectedValueIndex = 1 then Result := 'vni' else Result := 'telex';
end;

function SelectedToneStyle: String;
begin
  if ToneStylePage.SelectedValueIndex = 1 then Result := 'new' else Result := 'old';
end;

procedure WriteInitialConfig;
var
  Dir, Path, Json: String;
begin
  Dir := ExpandConstant('{userappdata}\Goxvi');
  Path := Dir + '\config.json';
  if FileExists(Path) then Exit; { keep the user's existing settings }
  if not ForceDirectories(Dir) then Exit;
  Json :=
    '{' + #13#10 +
    '  "version": 1,' + #13#10 +
    '  "toneStyle": "' + SelectedToneStyle + '",' + #13#10 +
    '  "inputMethod": "' + SelectedInputMethod + '",' + #13#10 +
    '  "shortcutsEnabled": true,' + #13#10 +
    '  "shortcuts": []' + #13#10 +
    '}' + #13#10;
  SaveStringToFile(Path, Json, False);
end;

{ Fix field report 0.0.3: máy cài xong vẫn thiếu Goxvi trong language list nên
  không đặt default được. Nguyên nhân: --activate-tip là checkbox postinstall
  (bỏ tick / cài silent là mất) + nowait (exit code không ai đọc -> fail im
  lặng). Fix: chạy VÔ ĐIỀU KIỆN + TUẦN TỰ tại ssPostInstall, mỗi bước chờ +
  kiểm tra exit code, hỏng thì Log + chỉ dẫn thêm tay. ExecAsOriginalUser:
  PHẢI ngữ cảnh user đăng nhập (không phải admin của installer) thì mới ghi
  đúng HKCU của user. Helper còn tự verify registry + fallback
  Set-WinUserLanguageList; nhật ký %APPDATA%\Goxvi\activate-tip.log. }
procedure WarnManualStep(const Detail: String);
begin
  Log('Goxvi setup step failed: ' + Detail);
  if not WizardSilent then
    MsgBox('Chưa tự cài xong Goxvi làm bàn phím mặc định (' + Detail + ').'
      + #13#10#13#10
      + 'Thêm thủ công: Settings > Time & language > Language & region > '
      + 'Vietnamese > Language options > Add a keyboard > Goxvi.' + #13#10
      + 'Đặt mặc định: Settings > Time & language > Typing > Advanced '
      + 'keyboard settings > Override for default input method > Goxvi.'
      + #13#10#13#10
      + 'Chi tiết lỗi: %APPDATA%\Goxvi\activate-tip.log',
      mbInformation, MB_OK);
end;

procedure RegisterAndActivateTip;
var
  ResultCode: Integer;
begin
  { regsvr32 /s: exit 0 = DllRegisterServer OK (ghi HKLM - installer elevated) }
  if (not Exec(ExpandConstant('{sys}\regsvr32.exe'),
      '/s "' + ExpandConstant('{app}\goxvi-tsf.dll') + '"', '',
      SW_HIDE, ewWaitUntilTerminated, ResultCode)) or (ResultCode <> 0) then
  begin
    WarnManualStep('regsvr32 x64, code ' + IntToStr(ResultCode));
    Exit; { chưa đăng ký được TIP thì activate phía sau chắc chắn vô nghĩa }
  end;
  { x86 fail: app 64-bit vẫn gõ được -> cảnh báo nhưng vẫn activate tiếp }
  if (not Exec(ExpandConstant('{syswow64}\regsvr32.exe'),
      '/s "' + ExpandConstant('{app}\x86\goxvi-tsf.dll') + '"', '',
      SW_HIDE, ewWaitUntilTerminated, ResultCode)) or (ResultCode <> 0) then
    WarnManualStep('regsvr32 x86, code ' + IntToStr(ResultCode));
  { Helper exit 0 = đã VERIFY Goxvi nằm bền trong language list của user }
  if (not ExecAsOriginalUser(ExpandConstant('{app}\Settings\Goxvi.exe'),
      '--activate-tip', '', SW_HIDE, ewWaitUntilTerminated, ResultCode))
      or (ResultCode <> 0) then
    WarnManualStep('activate-tip, code ' + IntToStr(ResultCode));
end;

{ Update tại chỗ: goxvi-tsf.dll đã đăng ký được TSF nạp IN-PROCESS vào mọi app
  đang gõ (explorer, browser, Zalo...) nên installer KHÔNG ghi đè được -> lỗi
  "cannot replace file in use" / buộc reboot. Windows cho ĐỔI TÊN DLL đang map
  (khoá ghi/xoá nhưng KHÔNG khoá rename), giải phóng tên file cho bản mới mà
  không cần reboot. Y hệt mẹo rebuild-khi-bị-lock ở CLAUDE.md, áp cho installer.
  App đang chạy giữ bản .old tới khi khởi động lại; .old dọn ở lần update sau
  (DeleteFile im lặng khi còn khoá) + [UninstallDelete]. }
function FreeLockedDll(const Path: String): Boolean;
var
  i: Integer;
  OldName: String;
begin
  Result := True;
  if not FileExists(Path) then Exit; { fresh install: chưa có gì để giải phóng }
  DeleteFile(Path + '.old');         { dọn .old cũ đã nhả khoá (app đã tắt) }
  if RenameFile(Path, Path + '.old') then Exit;
  { .old vẫn bị khoá (app từ update trước chưa tắt) -> thử tên .oldN duy nhất }
  for i := 1 to 999 do
  begin
    OldName := Path + '.old' + IntToStr(i);
    DeleteFile(OldName);
    if RenameFile(Path, OldName) then Exit;
  end;
  Result := False; { hết cách: cùng lắm Inno tự lên lịch thay khi reboot }
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  // Thư mục app đã biết từ bản cài trước. Đổi tên cả x64 lẫn x86 trước [Files].
  if not FreeLockedDll(ExpandConstant('{app}\goxvi-tsf.dll')) then
    Result := 'Không giải phóng được goxvi-tsf.dll (x64) đang dùng.'
  else if not FreeLockedDll(ExpandConstant('{app}\x86\goxvi-tsf.dll')) then
    Result := 'Không giải phóng được goxvi-tsf.dll (x86) đang dùng.';
  if Result <> '' then
    Result := Result + #13#10 + 'Hãy đăng xuất (log off) rồi chạy lại bộ cài.';
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    WriteInitialConfig;
    RegisterAndActivateTip;
  end;
end;
