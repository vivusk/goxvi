#pragma once

#import "goxvi-input-controller.h"

// Methods shared between goxvi-input-controller.mm and its menu category
// (goxvi-input-controller-menu.mm), split only to keep files < 200 lines.
@interface GoxviInputController ()
// Ends the current word without rewrites (K8); Direct mode never touches the client.
- (void)finishComposition:(id)client;
// Adopts a new ConfigStore revision, only while idle (K5).
- (void)syncConfigIfIdle;
// Drops the word (session + Direct tracker) without touching the client.
- (void)forgetWord;
- (void)toggleVietnamese:(id)client;
@end
