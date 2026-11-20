// LEGO Digital Designer — restored source
// TU 126 "NSApplication+LEGO.mm" (file name inferred) — Mac 0x1dc2c0-0x1dc3df, Mac build only:
// category LEGOApplicationCategory on NSApplication; MainMenu.nib's About / Preferences items
// forward to (and are enabled by) the Application singleton. Static initialiser 0x1dc3a0
// (<iostream> only). Pure Objective-C: nothing to compile on the Linux-hosted Mac branch.
#if !defined(_WIN32) && defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#include "Application.h"

@interface NSApplication (LEGOApplicationCategory)
- (void)showLEGOAboutBox:(id)sender;
- (void)showLEGOPreferences:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem*)item;
@end

@implementation NSApplication (LEGOApplicationCategory)
// Mac 0x1dc2c0
- (void)showLEGOAboutBox:(id)sender
{
    LEGO::Application::GetInstance()->ShowAboutBox();
}

// Mac 0x1dc2e0
- (void)showLEGOPreferences:(id)sender
{
    LEGO::Application::GetInstance()->ShowPreferences();
}

// Mac 0x1dc300
- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    if ([item action] == @selector(showLEGOAboutBox:))
        return LEGO::Application::GetInstance()->CanShowAboutBox();
    if ([item action] == @selector(showLEGOPreferences:))
        return LEGO::Application::GetInstance()->CanShowPreferences();
    if ([item action] == @selector(terminate:))
        return LEGO::Application::GetInstance()->IsIdle();
    return YES;
}
@end

#endif
