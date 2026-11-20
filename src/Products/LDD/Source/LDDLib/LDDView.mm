// LEGO Digital Designer — restored source
// LDDView.mm — objc class LDDView (TU 156, Mac 0x2200f0-0x2205ff). See LDDView.h. Mac only:
// nothing is compiled on the Linux-hosted Mac branch (no Objective-C) or on Windows.
#if !defined(_WIN32) && defined(__OBJC__)
#import "LDDView.h"
#import "MacWindow.h"
#include "Window.h"
#include "Application.h"
#include "Path.h"
#include "Platform.h"
#include "StringConversion.h"

@implementation LDDView

// Mac 0x2200f0: accepts dragged file names.
- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
    }
    return self;
}

// Mac 0x220190
- (BOOL)acceptsFirstResponder
{
    return YES;
}

// Mac 0x2201a0
- (void)rightMouseDown:(NSEvent*)event
{
    [[self nextResponder] rightMouseDown:event];
}

// Mac 0x2201e0: a copy of file names, unless a modal window / modal state is up.
// NOTE: the draggingSourceOperationMask is asked for and ignored.
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender
{
    LEGO::Window* window = [(LEGOMacWindow*)[self window] GetLEGOWindow];
    if (!window)
        return NSDragOperationNone;
    if ([NSApp modalWindow] || window->IsModal())
        return NSDragOperationNone;
    [sender draggingSourceOperationMask];
    if ([[[sender draggingPasteboard] types] containsObject:NSFilenamesPboardType])
        return NSDragOperationCopy;
    return NSDragOperationNone;
}

// Mac 0x2202b0: only the first file counts; it must end in .lxfml or .lxf.
- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard* pasteboard = [sender draggingPasteboard];
    if (![[pasteboard types] containsObject:NSFilenamesPboardType])
        return NO;
    NSArray* files = [pasteboard propertyListForType:NSFilenamesPboardType];
    if ([files count] == 0)
        return NO;
    NSString* extension = [[files objectAtIndex:0] pathExtension];
    if ([extension isEqualToString:@"lxfml"])
        return YES;
    return [extension isEqualToString:@"lxf"];
}

// Mac 0x2203b0: opens the first file (VFS path of the native path).
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard* pasteboard = [sender draggingPasteboard];
    if (![[pasteboard types] containsObject:NSFilenamesPboardType])
        return NO;
    NSArray* files = [pasteboard propertyListForType:NSFilenamesPboardType];
    if ([files count] == 0)
        return NO;
    LEGO::Path path(LEGO::String(), 0);
    LEGO::Platform* platform = LEGO::Platform::GetInstance();
    path.Append(platform->NativeToVFSPath(LEGO::MakeString((CFStringRef)[files objectAtIndex:0])));
    return LEGO::Application::GetInstance()->OpenDocumentFile(path);
}
@end
#endif
