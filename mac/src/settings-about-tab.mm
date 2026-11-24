#import "settings-about-tab.h"

#import "settings-ui-helpers.h"

namespace {

NSString* const kWebsite = @"https://goxvi.com/";

}  // namespace

@implementation GoxviAboutTab

- (instancetype)init {
  if ((self = [super init])) {
    NSView* view = [[GoxviFlippedView alloc] initWithFrame:NSMakeRect(0, 0, 500, 320)];
    NSTextField* name = goxviLabel(@"Goxvi", NSMakeRect(20, 20, 460, 30));
    name.font = [NSFont boldSystemFontOfSize:22];
    NSString* version =
        [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"] ?: @"?";
    NSTextField* versionLabel =
        goxviLabel([NSString stringWithFormat:@"Phiên bản %@", version],
                   NSMakeRect(20, 56, 460, 20));
    NSTextField* summary = [NSTextField wrappingLabelWithString:
        @"Bộ gõ tiếng Việt Telex và VNI cho macOS, dựng trên InputMethodKit. "
        @"Đảo Việt/Anh bằng phím tắt (mặc định ⌃⇧) hoặc menu nhập liệu trên thanh menu."];
    summary.frame = NSMakeRect(20, 90, 460, 60);
    NSButton* link = goxviButton(@"goxvi.com", NSMakeRect(15, 160, 140, 32), self,
                                 @selector(openWebsite:));
    for (NSView* control in @[ name, versionLabel, summary, link ]) [view addSubview:control];
    _view = view;
  }
  return self;
}

- (void)openWebsite:(id)sender {
  [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:kWebsite]];
}

@end
