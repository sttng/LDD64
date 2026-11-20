// LEGO Digital Designer — restored source
// LEGO::LDDWindow — LDD's main window: an OpenGLWindow whose Mac GL view gets an LDDView
//   (LDDView.mm: right-click forwarding and LXF / LXFML file drops) on top.
//
// TU 155 (TU-name guess "LDDWindow.cpp"): Mac 0x21fa50-0x2200ef (LDDWindow.mm,
// Objective-C), Win 0x5c7f30-0x5c80b4 + 0x5fbcaa (LDDWindow.cpp). The LDDLib/ directory is a guess.
// Vtables: Mac 0x72cdec (= OpenGLWindow's 61 slots, CreateContext 0x21fa50; D1 0x21fb30, D0
// 0x21fb90 + thunks 0x21fc00-0x220030); Win 0x9c94ac (54 slots, CreateContext 0x5fbcaa), sdd
// 0x5c8099 (thunks 0x5c7f28, 0x5c7f9a). No members of its own (Mac Object at +0xfc like
// OpenGLWindow). The constructor is inlined at its caller (Win body 0x5c7f30 sets the vtables).
#pragma once
#include "LEGO/Types.h"
#include "OpenGLWindow.h"

namespace LEGO {

class LDDWindow : public OpenGLWindow {
public:
    LDDWindow(bool flag95, UInt32 level) : OpenGLWindow(flag95, level) {}  // (inlined)
    virtual ~LDDWindow() {}                       // Mac 0x21fb30/0x21fb90, Win 0x5c7f30 / sdd 0x5c8099

    // OpenGLWindow::CreateContext; on the Mac adds an LDDView covering the GL view.
    virtual bool CreateContext();                 // Mac 60 0x21fa50, Win 53 0x5fbcaa
};

} // namespace LEGO
