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
; Register both TIPs. {sys}=System32 (64-bit regsvr32), {syswow64}=32-bit regsvr32.
Filename: "{sys}\regsvr32.exe"; Parameters: "/s ""{app}\goxvi-tsf.dll"""; StatusMsg: "Đang đăng ký bộ gõ (x64)..."; Flags: runhidden waituntilterminated
Filename: "{syswow64}\regsvr32.exe"; Parameters: "/s ""{app}\x86\goxvi-tsf.dll"""; StatusMsg: "Đang đăng ký bộ gõ (x86)..."; Flags: runhidden waituntilterminated
; Checkbox trang cuối (tick sẵn): đặt Goxvi làm bàn phím tiếng Việt (VIE) NGAY cho
; user vừa cài -> khỏi phải mò Win+Space. runasoriginaluser: PHẢI chạy trong ngữ
; cảnh user (không phải context admin của installer) thì ActivateProfile mới tác
; động đúng phiên đang đăng nhập. Chạy sau khi regsvr32 đã đăng ký DLL ở trên.
Filename: "{app}\Settings\Goxvi.exe"; Parameters: "--activate-tip"; Description: "Đặt Goxvi làm bàn phím tiếng Việt (VIE) ngay bây giờ"; Flags: postinstall runasoriginaluser nowait skipifsilent
; Optionally open Settings after install
Filename: "{app}\Settings\Goxvi.exe"; Description: "Mở Goxvi Settings"; Flags: postinstall nowait skipifsilent unchecked

[UninstallRun]
; Unregister before files are removed (reverse order not required).
Filename: "{sys}\regsvr32.exe"; Parameters: "/u /s ""{app}\goxvi-tsf.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregGoxviX64"
Filename: "{syswow64}\regsvr32.exe"; Parameters: "/u /s ""{app}\x86\goxvi-tsf.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregGoxviX86"

[Messages]
; Trang cuối wizard. Nhấn mạnh: gõ tiếng Việt qua bộ gõ tích hợp Windows (Win+Space),
; KHÔNG cần chạy app Goxvi; app chỉ để cấu hình. App self-contained nên KHÔNG cần .NET.
FinishedLabel=Cài đặt xong! Goxvi đã tích hợp sẵn vào Windows 11: nhấn Win+Space rồi chọn "Goxvi Telex" là gõ được tiếng Việt ngay, KHÔNG cần chạy ứng dụng Goxvi.%n%nChỉ khi cần đổi kiểu gõ hoặc cấu hình thì mới mở ứng dụng Goxvi (đã kèm sẵn mọi thứ, không phải cài thêm gì).%n%n(Nếu chưa thấy trong Win+Space: Settings > Time & language > Language & region > Add a keyboard.)

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

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    WriteInitialConfig;
end;
