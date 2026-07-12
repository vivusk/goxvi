
# E2E test: rename that tren Explorer. Tao file "test.txt" trong thu muc tam,
# mo Explorer, chon file, F2, go a-r, Enter -> ky vong ten "ả.txt" (a hoi).
param([int]$KeyGapMs = 150)

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class KeySender {
  [DllImport("user32.dll")] static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
  [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
  [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hWnd);
  [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd, IntPtr pid);
  [DllImport("kernel32.dll")] static extern uint GetCurrentThreadId();
  [DllImport("user32.dll")] static extern bool AttachThreadInput(uint idAttach, uint idAttachTo, bool fAttach);
  public static bool ForceForeground(IntPtr hWnd) {
    for (int i = 0; i < 6; i++) {
      IntPtr fg = GetForegroundWindow();
      if (fg == hWnd) return true;
      // Trick mo khoa foreground: go phim Alt "ao" de thread nay duoc tinh
      // la co input gan nhat, roi attach vao thread foreground hien tai.
      keybd_event(0x12, 0, 0, UIntPtr.Zero);
      keybd_event(0x12, 0, 2, UIntPtr.Zero);
      uint fgThread = GetWindowThreadProcessId(fg, IntPtr.Zero);
      uint myThread = GetCurrentThreadId();
      AttachThreadInput(myThread, fgThread, true);
      SetForegroundWindow(hWnd);
      AttachThreadInput(myThread, fgThread, false);
      System.Threading.Thread.Sleep(300);
      if (GetForegroundWindow() == hWnd) return true;
    }
    return GetForegroundWindow() == hWnd;
  }
  public static bool Tap(IntPtr hWndExpected, byte vk) {
    if (GetForegroundWindow() != hWndExpected && !ForceForeground(hWndExpected)) return false;
    keybd_event(vk, 0, 0, UIntPtr.Zero);
    keybd_event(vk, 0, 2, UIntPtr.Zero);
    return true;
  }

  [StructLayout(LayoutKind.Sequential)]
  struct LASTINPUTINFO { public uint cbSize; public uint dwTime; }
  [DllImport("user32.dll")] static extern bool GetLastInputInfo(ref LASTINPUTINFO plii);
  [DllImport("kernel32.dll")] static extern uint GetTickCount();
  public static uint IdleMs() {
    LASTINPUTINFO lii = new LASTINPUTINFO();
    lii.cbSize = (uint)Marshal.SizeOf(typeof(LASTINPUTINFO));
    GetLastInputInfo(ref lii);
    return GetTickCount() - lii.dwTime;
  }
}
'@

# Cho den khi user ngung go phim/chuot >= 4s (toi da doi 90s) de khong nhieu test.
$waited = 0
while ([KeySender]::IdleMs() -lt 4000 -and $waited -lt 90000) {
  Start-Sleep -Milliseconds 500; $waited += 500
}
if ([KeySender]::IdleMs() -lt 4000) { Write-Output 'FAIL: user van dang go, bo qua'; exit 1 }

$dir = Join-Path $env:TEMP 'goxvi-rename-test'
if (Test-Path $dir) { Remove-Item $dir -Recurse -Force -Confirm:$false }
New-Item -ItemType Directory $dir | Out-Null
New-Item -ItemType File (Join-Path $dir 'test.txt') | Out-Null

$shell = New-Object -ComObject Shell.Application
$shell.Open($dir)
Start-Sleep -Seconds 2

$win = $null
foreach ($w in $shell.Windows()) {
  if ($w.LocationURL -like '*goxvi-rename-test*') { $win = $w; break }
}
if (-not $win) { Write-Output 'FAIL: khong tim thay cua so Explorer'; exit 1 }

$item = $win.Document.Folder.ParseName('test.txt')
$win.Document.SelectItem($item, 0x1D)  # select + deselect-others + ensure-visible + focus
Start-Sleep -Milliseconds 500

$hwnd = [IntPtr]$win.HWND
if (-not [KeySender]::ForceForeground($hwnd)) { Write-Output 'FAIL: khong gianh duoc foreground'; $win.Quit(); exit 1 }
Start-Sleep -Milliseconds 300

$ok = $true
$ok = $ok -and [KeySender]::Tap($hwnd, 0x71)  # F2
Start-Sleep -Milliseconds 600
$ok = $ok -and [KeySender]::Tap($hwnd, 0x41)  # a
Start-Sleep -Milliseconds $KeyGapMs
$ok = $ok -and [KeySender]::Tap($hwnd, 0x52)  # r
Start-Sleep -Milliseconds $KeyGapMs
$ok = $ok -and [KeySender]::Tap($hwnd, 0x0D)  # Enter
Start-Sleep -Milliseconds 900

$names = (Get-ChildItem $dir | Select-Object -ExpandProperty Name) -join ', '
Write-Output "sent-all: $ok"
Write-Output "result  : $names"
$win.Quit()
