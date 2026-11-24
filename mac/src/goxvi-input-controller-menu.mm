#import "goxvi-input-controller-private.h"

#include <utility>

#import "input-menu.h"
#import "settings-window-controller.h"

#include "config-store.h"
#include "input-mode-state.h"

// Input menu of GoxviInputController (PDR D7). Every action ends the word
// under the current insert mode first (finishComposition:).
@implementation GoxviInputController (Menu)

- (NSMenu*)menu {
  return buildInputMenu(@selector(menuAction:), InputModeState::instance().vietnameseOn,
                        ConfigStore::instance().current().inputMethod);
}

// Telex/VNI from the menu: apply now + persist to config.json (D7, D8).
- (void)selectInputMethod:(goxvi::InputMethod)method {
  ConfigStore& store = ConfigStore::instance();
  if (store.current().inputMethod == method) return;
  [self finishComposition:self.client];
  goxvi::EngineConfig config = store.current();
  config.inputMethod = method;
  store.update(std::move(config));  // failure already logged; memory still updated
  [self syncConfigIfIdle];
}

- (void)menuAction:(id)sender {
  try {
    switch (inputMenuTagFromSender(sender)) {
      case GoxviMenuTagToggleVietnamese:
        [self toggleVietnamese:self.client];
        break;
      case GoxviMenuTagTelex:
        [self selectInputMethod:goxvi::InputMethod::Telex];
        break;
      case GoxviMenuTagVni:
        [self selectInputMethod:goxvi::InputMethod::Vni];
        break;
      case GoxviMenuTagSettings:
        [self finishComposition:self.client];  // the window takes the focus
        [GoxviSettingsWindowController showSettings];
        break;
      default:
        break;
    }
  } catch (...) {
    [self forgetWord];
  }
}

@end
