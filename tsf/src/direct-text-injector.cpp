#include "direct-text-injector.h"

#include <vector>

#include "key-event-handler.h"  // kGoxviInjectedKeyTag

namespace goxvi_direct {
namespace {

void pushKeyDownUp(std::vector<INPUT>& inputs, WORD vk, DWORD extraFlags) {
  INPUT input = {};
  input.type = INPUT_KEYBOARD;
  input.ki.wVk = vk;
  input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC));
  input.ki.dwFlags = extraFlags;
  input.ki.dwExtraInfo = goxvi_keys::kGoxviInjectedKeyTag;
  inputs.push_back(input);
  input.ki.dwFlags = extraFlags | KEYEVENTF_KEYUP;
  inputs.push_back(input);
}

}  // namespace

void sendTextReplacement(int eraseCount, const std::wstring& insertText,
                         bool leadingForwardDelete) {
  std::vector<INPUT> inputs;
  inputs.reserve(2 + static_cast<size_t>(eraseCount) * 2 +
                 insertText.size() * 2);

  if (leadingForwardDelete) {
    pushKeyDownUp(inputs, VK_DELETE, KEYEVENTF_EXTENDEDKEY);  // E0-extended
  }
  for (int i = 0; i < eraseCount; ++i) {
    pushKeyDownUp(inputs, VK_BACK, 0);
  }
  for (const wchar_t ch : insertText) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = ch;
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    input.ki.dwExtraInfo = goxvi_keys::kGoxviInjectedKeyTag;
    inputs.push_back(input);
    input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    inputs.push_back(input);
  }

  if (!inputs.empty()) {
    SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
  }
}

}  // namespace goxvi_direct
