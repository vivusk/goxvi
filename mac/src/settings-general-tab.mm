#import "settings-general-tab.h"

#include <string>
#include <utility>

#include "config-store.h"
#import "settings-ui-helpers.h"

namespace {

// Popup order == K4 values; an unknown stored string shows as ctrl+shift,
// which is also how the detector interprets it (parseToggleHotkey).
NSArray<NSString*>* hotkeyValues() {
  return @[ @"ctrl+shift", @"option+shift", @"ctrl+option", @"none" ];
}

}  // namespace

@implementation GoxviGeneralTab {
  NSPopUpButton* _inputMethod;
  NSPopUpButton* _toneStyle;
  NSPopUpButton* _hotkey;
  NSButton* _restoreOnInvalid;
  NSButton* _autoCorrect;
}

- (instancetype)init {
  if ((self = [super init])) {
    NSView* view = [[GoxviFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 500, 320)];
    SEL changed = @selector(changed:);
    const CGFloat labelX = 20, labelW = 220, fieldX = 245, fieldW = 240;
    [view addSubview:goxviLabel(@"Kiểu gõ:", NSMakeRect(labelX, 24, labelW, 20))];
    _inputMethod = goxviPopup(@[ @"Telex", @"VNI" ], NSMakeRect(fieldX, 20, fieldW, 26),
                              self, changed);
    [view addSubview:goxviLabel(@"Kiểu đặt dấu:", NSMakeRect(labelX, 60, labelW, 20))];
    _toneStyle = goxviPopup(@[ @"Kiểu cũ  (hòa, thúy)", @"Kiểu mới  (hoà, thuý)" ],
                            NSMakeRect(fieldX, 56, fieldW, 26), self, changed);
    [view addSubview:goxviLabel(@"Phím tắt bật/tắt tiếng Việt:",
                                NSMakeRect(labelX, 96, labelW, 20))];
    _hotkey = goxviPopup(@[ @"⌃⇧  Control + Shift", @"⌥⇧  Option + Shift",
                            @"⌃⌥  Control + Option", @"Không dùng" ],
                         NSMakeRect(fieldX, 92, fieldW, 26), self, changed);
    _restoreOnInvalid = goxviCheckbox(@"Khôi phục phím khi gõ sai chính tả",
                                      NSMakeRect(labelX, 140, 460, 20), self, changed);
    _autoCorrect = goxviCheckbox(@"Tự sửa lỗi gõ đảo phím (vd. twosi → tới)",
                                 NSMakeRect(labelX, 170, 460, 20), self, changed);
    for (NSView* control in @[ _inputMethod, _toneStyle, _hotkey, _restoreOnInvalid, _autoCorrect ]) {
      [view addSubview:control];
    }
    _view = view;
    [self reload];
  }
  return self;
}

- (void)reload {
  const goxvi::EngineConfig& config = ConfigStore::instance().current();
  [_inputMethod selectItemAtIndex:config.inputMethod == goxvi::InputMethod::Vni ? 1 : 0];
  [_toneStyle selectItemAtIndex:config.toneStyle == goxvi::ToneStyle::New ? 1 : 0];
  const NSUInteger hotkey = [hotkeyValues() indexOfObject:@(config.toggleHotkey.c_str())];
  [_hotkey selectItemAtIndex:hotkey == NSNotFound ? 0 : (NSInteger)hotkey];
  _restoreOnInvalid.state = config.restoreOnInvalid ? NSControlStateValueOn : NSControlStateValueOff;
  _autoCorrect.state = config.autoCorrectEnabled ? NSControlStateValueOn : NSControlStateValueOff;
}

// Only the field of the sender changes, so other fields (incl. an unknown
// hand-edited toggleHotkey) are kept as stored.
- (void)changed:(id)sender {
  goxvi::EngineConfig config = ConfigStore::instance().current();
  if (sender == _inputMethod) {
    config.inputMethod = _inputMethod.indexOfSelectedItem == 1 ? goxvi::InputMethod::Vni
                                                               : goxvi::InputMethod::Telex;
  } else if (sender == _toneStyle) {
    config.toneStyle = _toneStyle.indexOfSelectedItem == 1 ? goxvi::ToneStyle::New
                                                           : goxvi::ToneStyle::Old;
  } else if (sender == _hotkey) {
    config.toggleHotkey = hotkeyValues()[(NSUInteger)_hotkey.indexOfSelectedItem].UTF8String;
  } else if (sender == _restoreOnInvalid) {
    config.restoreOnInvalid = _restoreOnInvalid.state == NSControlStateValueOn;
  } else if (sender == _autoCorrect) {
    config.autoCorrectEnabled = _autoCorrect.state == NSControlStateValueOn;
  } else {
    return;
  }
  if (!goxviSaveConfig(std::move(config), _view.window)) [self reload];
}

@end
