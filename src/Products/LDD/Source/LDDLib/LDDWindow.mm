// LEGO Digital Designer — restored source
// LDDWindow.mm — Mac part of LEGO::LDDWindow (TU 155, Mac 0x21fa50-0x2200ef). Windows:
// LDDWindow.cpp. On the Linux-hosted Mac branch (no Objective-C) no LDDView is added.
#if !defined(_WIN32)
#include "LDDWindow.h"

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#import "LDDView.h"
#endif

namespace LEGO {

// Mac 0x21fa50: the LDDView gets the frame of the GL view (m_view, the LEGOOpenGLView).
bool LDDWindow::CreateContext()
{
    if (!OpenGLWindow::CreateContext())
        return false;
#if defined(__OBJC__)
    NSView* glView = (NSView*)m_view;
    LDDView* view = [[LDDView alloc] initWithFrame:glView ? [glView frame] : NSZeroRect];
    if (!view)
        return false;
    [glView addSubview:view];
    [view release];
#endif
    return true;
}

} // namespace LEGO
#endif // !_WIN32
