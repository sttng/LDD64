// LEGO Digital Designer — restored source
// LEGO::SaveCommand — saves the model ("save" in LDD.commands.editMode): enabled while the
//   scene is modified and not empty (it observes the scene control).
//
// TU 255 "SaveCommand.cpp": Mac 0x275570-0x275c0b, Win 0x6764a8-0x6765f8. Mac 0x275b90 =
// <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (Mac +0x28, Win +0x2c)}, virtual base
//   Object. Mac vtable 0x73a49c {D1 0x275740, D0 0x2757b0, OnFinalRelease 0x275840, Do 0x275690,
//   SetEnable, GetEnable, UpdateEnable 0x2756b0, OnSceneControlAttached 0x2758d0 (a new
//   virtual, Win slot 4 0x66e988; see CutCommand.h), then the Itanium primary entries of
//   the observer overriders OnEvent19 0x2758e0 and OnEvent13 0x275700}; observer-in
//   0x73a4d0 {OnEvent13 thunk 0x275710, OnEvent19 thunk 0x275a20}. Win 0x9d9678 {Do 0x6764a8, …,
//   UpdateEnable 0x6764b4}, observer-in 0x9d9624 {10: 0x6764e3, 16: 0x631956 (ICF with
//   ScreenShotCommand::OnEvent19)}.
// Layout (Mac / Win): Command +0x00..+0x28, observer +0x28 / +0x2c (subject +0x2c / +0x34),
//   Object +0x30 / +0x38 (Mac sizeof 0x38).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class SaveCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; observes `sceneControl`, then UpdateEnable. Mac 0x275570, Win 0x6764eb
    explicit SaveCommand(BrickConnectSceneControl* sceneControl);
    virtual ~SaveCommand();                                                 // Mac 0x275740/0x2757b0, Win sdd 0x6765f8

    // Application::SaveDocument.
    virtual void Do();                                                      // 3/0 Mac 0x275690, Win 0x6764a8
    // Enabled while there is a scene control whose scene is modified and not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x2756b0, Win 0x6764b4
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name
    // inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x2758d0, Win 0x66e988

protected:
    // ---- BrickConnectSceneControlObserver ----
    // UpdateEnable. (declared first: Mac primary entry 8, OnEvent13 9)
    virtual void OnEvent19();                                               // 19/16 Mac 0x2758e0 (thunk 0x275a20), Win 0x631956
    // The modified state changed: enabled = `modified`.
    virtual void OnEvent13(bool modified);                                  // 13/10 Mac 0x275700 (thunk 0x275710), Win 0x6764e3
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x275840
    virtual void OnFinalRelease();
};

} // namespace LEGO
