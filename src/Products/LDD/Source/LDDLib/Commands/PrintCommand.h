// LEGO Digital Designer — restored source
// LEGO::PrintCommand — prints a picture of the scene (TU 251 "PrintCommand.cpp", Mac
//   0x274a20-0x2751db); enabled while the scene is not empty. The
//   constructor is inline (its only copy: Mac 0x13f1a0 in TU 060, Win 0x5ca121).
// Mac layout (0x13f1a0): Command +0x00..+0x28, BrickConnectSceneControlObserver +0x28 (subject
//   +0x2c), window +0x30 (referenced), scene control +0x34, Object +0x38 (sizeof 0x40).
// Vtables: Mac 0x739fec {D1 0x274cd0, D0 0x274d30, OnFinalRelease 0x274da0, Do 0x274a20,
//   SetEnable, GetEnable, UpdateEnable 0x274c90, OnSceneControlAttached 0x274e30, OnEvent19
//   0x274e40}, observer-in 0x73a01c {19: thunk 0x274f40 -> UpdateEnable}. Win 0x9ca328 {Do
//   0x609299, …, UpdateEnable 0x63120b, 0x66e988}, observer-in 0x9ca2d4 {16: 0x631956},
//   Object-in 0x9ca2bc {sdd 0x5ca1cf / 0x5ca251}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Command.h"
#include "Window.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class PrintCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Mac 0x13f1a0 (inline, emitted in TU 060), Win 0x5ca121
    PrintCommand(Window* window, BrickConnectSceneControl* sceneControl)
        : Command(true), m_window(window), m_sceneControl(sceneControl)
    {
        BrickConnectSceneControlObserver::Attach(sceneControl);
        UpdateEnable();
    }
    virtual ~PrintCommand();                                                // Mac 0x274cd0/0x274d30, Win sdd 0x5ca251

    // Print dialog (owned by the window); accepted: document "LDD Print Screenshot", one page
    // with a square BGR24 picture of the scene (RenderToBitmap: camera fitted, post-processed; side
    // min(root control width, height, 2048) rounded down to 16) drawn into the top-left square
    // of the page (side min(page width, height)); a failed draw aborts the document.
    virtual void Do();                                                      // 3/0 Mac 0x274a20, Win 0x609299
    // Enabled while there is a scene control whose scene is not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x274c90, Win 0x63120b
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name
    // inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x274e30, Win 0x66e988

protected:
    virtual void OnFinalRelease();                                          // 2 Mac 0x274da0 (thunks 0x274f20, 0x275040)
    // The scene changed: UpdateEnable. (BrickConnectSceneControlObserver slot 19)
    virtual void OnEvent19();                                               // 8/– Mac 0x274e40 (thunk 0x274f40), Win 0x631956

    RefPointer<Window>          m_window;        // Mac +0x30
    BrickConnectSceneControl*   m_sceneControl;  // Mac +0x34
};

} // namespace LEGO
