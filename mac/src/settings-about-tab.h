#pragma once

#import <Cocoa/Cocoa.h>

// "Giới thiệu" tab: name, version (CFBundleShortVersionString, which
// Info.plist takes from VERSION) and the goxvi.com link.
@interface GoxviAboutTab : NSObject
@property(nonatomic, readonly) NSView* view;
@end
