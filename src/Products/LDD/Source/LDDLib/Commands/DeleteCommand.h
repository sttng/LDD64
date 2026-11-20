// LEGO Digital Designer — restored source
// LEGO::DeleteCommand — deletes the selection ("delete" in LDD.commands.editMode, created by
//   EditMode::Initialize, TU 085); enabled while the selection is not empty.
//
// TU 235 "DeleteCommand.cpp": Mac 0x26e310-0x26e9ab, Win 0x66e299-0x66e3fa. The TU also emits
// NotifierT<Command, CommandObserver> destructor helpers (Notifier.h); Mac 0x26e930 = <iostream>.
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28, Win +0x2c)},
//   virtual base Object. Mac vtable 0x7384bc {D1 0x26e510, D0 0x26e580, OnFinalRelease 0x26e610,
//   Do 0x26e430, SetEnable, GetEnable, UpdateEnable 0x26e470, OnSceneControlAttached 0x26e6a0,
//   OnEvent14 0x26e4b0}; observer-in 0x7384ec {14: thunk 0x26e4e0}. Win 0x9d8628 {Do 0x66e299,
//   …, UpdateEnable 0x66e2c6, 0x66e988}, observer-in 0x9d85d4 {11: 0x684af0}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), Object +0x30 (sizeof 0x38).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class DeleteCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; observes `sceneControl`, then UpdateEnable. Mac 0x26e310, Win 0x66e2ed
    explicit DeleteCommand(BrickConnectSceneControl* sceneControl);
    virtual ~DeleteCommand();                                               // Mac 0x26e510/0x26e580, Win sdd 0x66e3fa

    // RemoveSelection (with undo); an emptied scene resets the camera.
    virtual void Do();                                                      // 3/0 Mac 0x26e430, Win 0x66e299
    // Enabled while there is a scene control with a non-empty selection.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x26e470, Win 0x66e2c6
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x26e6a0, Win 0x66e988

protected:
    // The selection became empty (true) / not empty: enabled = !empty.
    virtual void OnEvent14(bool empty);                                     // 8/– Mac 0x26e4b0 (thunk 0x26e4e0), Win 0x684af0
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x26e610
    virtual void OnFinalRelease();
};

} // namespace LEGO
