// LEGO Digital Designer — restored source
// LEGO::ScreenShotCommand — saves a PNG of the scene (save panel "SCREENSHOTTITLE" in the
//   document's directory, else Platform directory 5; "SCREENSHOTERROR" alert on failure);
//   enabled while the scene is not empty (it observes the scene control). Base of
//   BGScreenShotCommand (TU 407, which passes its own render queue).
//
// TU 256 "ScreenShotCommand.cpp" (Mac 0x275c10-0x276b6b; Win 0x63120b-0x6316bc).
// Mac layout (constructor 0x275d50): Command +0x00..+0x28, BrickConnectSceneControlObserver
// (private base) +0x28 (subject +0x2c), Object +0x30 (sizeof 0x38). Win (ctor 0x631628):
// observer +0x2c (subject +0x34), Object +0x3c.
// Vtables: Mac 0x73a62c {D1 0x2766e0, D0 0x276860, OnFinalRelease 0x276a20, Do 0x2765f0,
// SetEnable, GetEnable, UpdateEnable 0x275e70, Mac-only primary entries of the observer
// overriders OnAttached 0x276610, OnEvent19 0x276640}; Win 0x9d2dc8 {Do 0x6315ae, …,
// UpdateEnable 0x63120b}, observer-in 0x9d2d74 {OnAttached 0x631d53, …, OnEvent19 0x631956}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class RenderQueue;

class ScreenShotCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled until attached (OnAttached runs UpdateEnable). Mac 0x275d50, Win 0x631628
    explicit ScreenShotCommand(BrickConnectSceneControl* control);
    virtual ~ScreenShotCommand();                                           // Mac 0x2766e0/0x276860, Win 0x6315b6 (sdd 0x6316bc)

    virtual void Do();                                                      // 3/0 Mac 0x2765f0, Win 0x6315ae (TakeScreenShot(0))
    // Enabled while there is a scene control and its scene is not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x275e70, Win 0x63120b

protected:
    // Save panel, then SaveScreenshot(control, path, (0, 0), no target, no camera fit, `queue`,
    // with post-processing); an alert when that fails. Mac 0x275eb0, Win 0x63122e (name inferred)
    void TakeScreenShot(RenderQueue* queue);

    // ---- BrickConnectSceneControlObserver: both re-evaluate the enable state ----
    virtual void OnAttached(BrickConnectSceneControl* control);             // 7/– Mac 0x276610 (thunk 0x276620), Win 0x631d53
    virtual void OnEvent19();                                               // 8/– Mac 0x276640 (thunk 0x276650), Win 0x631956
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x276a20
    virtual void OnFinalRelease();
};

} // namespace LEGO
