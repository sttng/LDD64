// LEGO Digital Designer — restored source
// LEGO::CutCommand — cut (true) / copy (false) of the selection into the edit mode's clip board
//   ("cut" / "copy" in LDD.commands.editMode, created by EditMode::Initialize, TU 085); enabled
//   while the selection is not empty (it observes the scene control).
//
// TU 233 "CutCommand.cpp": Mac 0x26c150-0x26cb5b, Win 0x66e889-0x66ea13. The TU also emits the
// clip board notification BricksClipBoard::NotifyChanged (Mac 0x26c360, BricksClipBoard.h) and the
// BasicArray<CommandObserver*> base-destructor helper Mac 0x26c9e0 (Notifier.h); Mac 0x26cae0 =
// <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28)}, virtual base
//   Object. Mac vtable 0x737cac {D1 0x26c680, D0 0x26c6e0, OnFinalRelease 0x26c750, Do 0x26c2b0,
//   SetEnable, GetEnable, UpdateEnable 0x26c5e0, OnSceneControlAttached 0x26c7e0, OnEvent14
//   0x26c620}; observer-in 0x737cdc {14: thunk 0x26c650}. Win 0x9d8790 {Do 0x66e889, SetEnable,
//   GetEnable, UpdateEnable 0x66e2c6, 0x66e988}, observer-in 0x9d873c {11: 0x684af0}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), cut flag +0x30, clip board
//   +0x34 (referenced), Object +0x38 (sizeof 0x40). Win: Object +0x44.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"
#include "BricksClipBoard.h"

namespace LEGO {

class CutCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; references `clipBoard`, observes `sceneControl`, then UpdateEnable.
    // Mac 0x26c150, Win 0x66e8d8
    CutCommand(BrickConnectSceneControl* sceneControl, BricksClipBoard* clipBoard, bool cut);
    virtual ~CutCommand();                                                  // Mac 0x26c680/0x26c6e0, Win sdd 0x66ea13

    // The clip board gets the element-brick snapshots of the selection (cut: the selection is
    // removed with undo), then its observers hear about it. Nothing without a selection.
    virtual void Do();                                                      // 3/0 Mac 0x26c2b0, Win 0x66e889
    // Enabled while there is a scene control with a non-empty selection.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x26c5e0, Win 0x66e2c6
    // UpdateEnable. NOTE: a NEW virtual of the class (Mac primary slot 7, Win slot 4 with one
    // argument, body shared by every command of this kind: Win 0x66e988), not an override of
    // the observer's OnAttached (the observer-in table keeps the empty base entry) — evidently
    // meant as OnAttached; nothing calls it. (name inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x26c7e0, Win 0x66e988

protected:
    // ---- BrickConnectSceneControlObserver ----
    // The selection became empty (true) / not empty: enabled = !empty.
    virtual void OnEvent14(bool empty);                                     // 8/– Mac 0x26c620 (thunk 0x26c650), Win 0x684af0
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x26c750
    // (thunks 0x26c8c0, 0x26c9c0)
    virtual void OnFinalRelease();

    bool                        m_cut;        // Mac +0x30
    RefPointer<BricksClipBoard> m_clipBoard;  // Mac +0x34
};

} // namespace LEGO
