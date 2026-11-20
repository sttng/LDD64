// LEGO Digital Designer — restored source
// LEGO::SelectAllCommand — selects every part of the scene ("selectAll" in LDD.commands.editMode,
//   created by EditMode::Initialize, TU 085): the scene manager drops its active tool first.
//   Enabled while the scene is not empty.
//
// TU 257 "SelectAllCommand.cpp": Mac 0x276b70-0x2771fb, Win 0x66e151-0x66e27a. Mac 0x277180 =
// <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28)}, virtual base
//   Object. Mac vtable 0x73a7bc {D1 0x276d40, D0 0x276db0, OnFinalRelease 0x276e40, Do 0x276c90,
//   SetEnable, GetEnable, UpdateEnable 0x276cd0, OnAttached 0x276ed0, OnEvent19 0x276d10};
//   observer-in 0x73a7ec {3: thunk 0x277010, 19: thunk 0x276d20}. Win 0x9d8590 {Do 0x66e151, …,
//   UpdateEnable 0x63120b}, observer-in 0x9d853c {0: 0x631d53, 16: 0x631956}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), Object +0x30.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class SelectAllCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; observes `sceneControl` (OnAttached sets the enable state). Mac 0x276b70,
    // Win 0x66e174
    explicit SelectAllCommand(BrickConnectSceneControl* sceneControl);
    virtual ~SelectAllCommand();                                            // Mac 0x276d40/0x276db0, Win sdd 0x66e27a

    // The scene manager's active tool goes, then BrickConnectSceneControl::SelectAll.
    virtual void Do();                                                      // 3/0 Mac 0x276c90, Win 0x66e151
    // Enabled while there is a scene control whose scene is not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x276cd0, Win 0x63120b

protected:
    // ---- BrickConnectSceneControlObserver: both UpdateEnable ----
    virtual void OnAttached(BrickConnectSceneControl* control);             // 7 Mac 0x276ed0 (thunk 0x277010), Win 0x631d53
    virtual void OnEvent19();                                               // 8 Mac 0x276d10 (thunk 0x276d20), Win 0x631956
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x276e40
    virtual void OnFinalRelease();
};

} // namespace LEGO
