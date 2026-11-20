// LEGO Digital Designer — restored source
// LEGO::BGModeCommand — switches to the automatic building-guide mode: a ModeNavigationCommand
//   that is also disabled while the scene is empty (it observes the scene control).
//
// TU 375 "BGModeCommand.cpp": Mac 0x362ed0-0x36363b, Win 0x631b41-0x631c19 (+ ctor 0x631c19).
// The TU also emits Observer<BrickConnectSceneControl, …> construction-vtable thunks. Mac
// 0x3635c0 = <iostream> + a header static (not code).
// RTTI: vmi {ModeNavigationCommand +0, BrickConnectSceneControlObserver (Mac +0x30)}, virtual
//   base Object. Mac vtable 0x75004c (12 entries: ModeNavigationCommand's 10 with UpdateEnable
//   6 = 0x362fa0, then the Itanium primary entries of the observer overriders OnEvent18
//   0x362ff0, OnEvent19 0x363020); observer-in 0x7500b0 {18: 0x363000, 19: 0x363030}.
//   Win: UpdateEnable 0x631b41, observer entries 0x631b78.
// Layout (Mac): ModeNavigationCommand +0x00..+0x30, observer +0x30 (subject +0x34), scene
//   control +0x38 (not referenced), Object +0x3c (operator new 0x44). Win: observer +0x38,
//   scene control +0x44.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "ModeNavigationCommand.h"
#include "BGAutomaticMode.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class BGModeCommand : public ModeNavigationCommand, private BrickConnectSceneControlObserver {
public:
    // Mac 0x362ed0, Win 0x631c19
    BGModeCommand(BGAutomaticMode* mode, BrickConnectSceneControl* sceneControl);
    virtual ~BGModeCommand();                                               // Mac 0x363050/0x363220, Win sdd 0x631bfa

    // ModeNavigationCommand's, then: while that enables, enabled only for a non-empty scene.
    // NOTE: original quirk: when the mode is current (disabled by ModeNavigationCommand) the
    // command is enabled again (both builds: SetEnable(!GetEnable() || !scene->IsEmpty())).
    virtual void UpdateEnable();                                            // 6/3 Mac 0x362fa0, Win 0x631b41

protected:
    // ---- BrickConnectSceneControlObserver: both re-evaluate the enable state ----
    virtual void OnEvent18();                                               // 10 Mac 0x362ff0 (thunk 0x363000), Win 0x631b78
    virtual void OnEvent19();                                               // 11 Mac 0x363020 (thunk 0x363030), Win 0x631b78
    // Observer<BrickConnectSceneControl> part, then ModeNavigationCommand's. Mac 0x363430
    virtual void OnFinalRelease();

    using ModeNavigationCommand::OnAttached;

    BrickConnectSceneControl* m_sceneControl;   // Mac +0x38, Win +0x44
};

} // namespace LEGO
