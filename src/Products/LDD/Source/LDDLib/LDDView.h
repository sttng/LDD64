// LEGO Digital Designer — restored source
// LDDView — transparent NSView over LDD's GL view (TU 156, Mac 0x2200f0-0x2205ff, LDDView.mm;
// Mac only, objc class LDDView : NSView, instance size 80, no ivars). Forwards right clicks to
// the next responder and opens a dropped .lxf / .lxfml file through Application::
// OpenDocumentFile. Added by LDDWindow::CreateContext (LDDWindow.mm).
#pragma once
#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>

@interface LDDView : NSView
- (id)initWithFrame:(NSRect)frame;
- (BOOL)acceptsFirstResponder;
- (void)rightMouseDown:(NSEvent*)event;
- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
@end
#endif
