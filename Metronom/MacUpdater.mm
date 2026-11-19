#import <AppKit/AppKit.h>
#import <Sparkle/Sparkle.h>

#include "MacUpdater.h"

#include <QtGlobal>

namespace
{
bool updatesConfigured()
{
    NSDictionary *const info{[[NSBundle mainBundle] infoDictionary]};
    const id feed{info[@"SUFeedURL"]};
    const id key{info[@"SUPublicEDKey"]};
    return [feed isKindOfClass:[NSString class]] && [feed length] > 0 && [key isKindOfClass:[NSString class]] &&
           [key length] > 0;
}

// Qt only moves the items it recognizes (About, Settings, Quit) into the app menu; any other item of the QML
// app-named menu would show up in a second menu, so this one is added natively after the About item.
void addCheckForUpdatesItem(SPUStandardUpdaterController *controller)
{
    NSMenu *const appMenu{[[[NSApp mainMenu] itemAtIndex:0] submenu]};
    if (appMenu == nil) {
        return;
    }
    const NSUInteger lastAbout{[appMenu.itemArray
        indexOfObjectWithOptions:NSEnumerationReverse
                     passingTest:^BOOL(NSMenuItem *candidate, NSUInteger, BOOL *) {
                       return [candidate.title hasPrefix:@"About"];
                     }]};
    const NSInteger index{lastAbout == NSNotFound ? 0 : static_cast<NSInteger>(lastAbout) + 1};
    NSMenuItem *const item
    {
        [[NSMenuItem alloc] initWithTitle:@"Check for Updates…" action:@selector(checkForUpdates:) keyEquivalent:@""]
    };
    item.target = controller;
    [appMenu insertItem:item atIndex:index];
}
} // namespace

void MacUpdater::start()
{
    if (!updatesConfigured()) {
        return;
    }
    // Created once and kept for the process lifetime: AppKit may still call it while the app quits.
    static SPUStandardUpdaterController *const controller{[] {
        SPUStandardUpdaterController *const created{[[SPUStandardUpdaterController alloc] initWithStartingUpdater:YES
                                                                                                  updaterDelegate:nil
                                                                                               userDriverDelegate:nil]};
        addCheckForUpdatesItem(created);
        return created;
    }()};
    Q_UNUSED(controller);
}
