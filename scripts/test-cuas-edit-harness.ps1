
# Harness: mo phong o rename Explorer bang WinForms TextBox (EDIT control / CUAS).
# SendInput chi ban phim khi form THUC SU foreground (tranh go nham cua so khac).
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
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

  // Ep foreground: attach vao thread dang giu foreground roi SetForegroundWindow.
  public static bool ForceForeground(IntPtr hWnd) {
    IntPtr fg = GetForegroundWindow();
    if (fg == hWnd) return true;
    uint fgThread = GetWindowThreadProcessId(fg, IntPtr.Zero);
    uint myThread = GetCurrentThreadId();
    AttachThreadInput(myThread, fgThread, true);
    bool ok = SetForegroundWindow(hWnd);
    AttachThreadInput(myThread, fgThread, false);
    return ok && GetForegroundWindow() == hWnd;
  }

  public static bool Tap(IntPtr hWndExpected, byte vk) {
    // Gianh lai foreground neu mat (user click cho khac giua chung).
    if (GetForegroundWindow() != hWndExpected && !ForceForeground(hWndExpected)) {
      return false;  // khong ban bua vao cua so khac
    }
    keybd_event(vk, 0, 0, UIntPtr.Zero);
    keybd_event(vk, 0, 2 /*KEYEVENTF_KEYUP*/, UIntPtr.Zero);
    return true;
  }
}
'@

$log = New-Object System.Collections.ArrayList
$form = New-Object System.Windows.Forms.Form
$form.Text = 'goxvi harness - dung go phim'
$form.TopMost = $true
$form.Width = 360; $form.Height = 120
$form.StartPosition = 'CenterScreen'
$tb = New-Object System.Windows.Forms.TextBox
$tb.Width = 320; $tb.Left = 10; $tb.Top = 20
$form.Controls.Add($tb)

$script:stage = 0
$timer = New-Object System.Windows.Forms.Timer
$timer.Interval = 350
$timer.Add_Tick({
  $script:stage++
  switch ($script:stage) {
    1 {
      $ok = [KeySender]::ForceForeground($form.Handle)
      [void]$log.Add("foreground: $ok")
      $tb.Text = 'test'; $tb.Focus() | Out-Null; $tb.SelectAll()
    }
    2 { [void]$log.Add("sent-a: $([KeySender]::Tap($form.Handle, 0x41))") }
    3 { [void]$log.Add("after-a : text='$($tb.Text)' sel=$($tb.SelectionStart)+$($tb.SelectionLength)") }
    4 { [void]$log.Add("sent-s: $([KeySender]::Tap($form.Handle, 0x53))") }
    5 { [void]$log.Add("after-s : text='$($tb.Text)'") }
    6 { [void]$log.Add("sent-sp: $([KeySender]::Tap($form.Handle, 0x20))") }
    7 { [void]$log.Add("final   : text='$($tb.Text)' sel=$($tb.SelectionStart)+$($tb.SelectionLength)"); $timer.Stop(); $form.Close() }
  }
})
$form.Add_Shown({ $form.Activate(); $tb.Focus() | Out-Null; $timer.Start() })
[System.Windows.Forms.Application]::Run($form)
$log -join "`r`n"
