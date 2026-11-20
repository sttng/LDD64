// LEGO Digital Designer — restored source
// LEGO::GroupToBrickGroupCommand — makes a new top-level group of the selection ("groupBricks"
//   in LDD.commands.editMode, created by EditMode::Initialize, TU 085); enabled while the
//   selection is not empty.
//
// TU 238 "GroupToBrickGroupCommand.cpp": Mac 0x26f520-0x26fbcb, Win 0x684b07-0x684c40.
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28)}, virtual base
//   Object. Mac vtable 0x73889c {D1 0x26f730, D0 0x26f7a0, OnFinalRelease 0x26f830, Do 0x26f640,
//   SetEnable, GetEnable, UpdateEnable 0x26f690, OnSceneControlAttached 0x26f8c0, OnEvent14
//   0x26f6d0}; observer-in 0x7388cc {14: thunk 0x26f700}. Win 0x9dc208 {Do 0x684b07, …,
//   UpdateEnable 0x66e2c6, 0x66e988}, observer-in 0x9dc1b4 {11: 0x684af0}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), Object +0x30.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class GroupToBrickGroupCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; observes `sceneControl`, then UpdateEnable. Mac 0x26f520, Win 0x684b33
    explicit GroupToBrickGroupCommand(BrickConnectSceneControl* sceneControl);
    virtual ~GroupToBrickGroupCommand();                                    // Mac 0x26f730/0x26f7a0, Win sdd 0x684c40

    // CreateGroup(no parent, selection) (with undo). Nothing without a selection.
    virtual void Do();                                                      // 3/0 Mac 0x26f640, Win 0x684b07
    // Enabled while there is a scene control with a non-empty selection.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x26f690, Win 0x66e2c6
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x26f8c0, Win 0x66e988

protected:
    // The selection became empty (true) / not empty: enabled = !empty.
    virtual void OnEvent14(bool empty);                                     // 8/– Mac 0x26f6d0 (thunk 0x26f700), Win 0x684af0
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x26f830
    virtual void OnFinalRelease();
};

} // namespace LEGO
