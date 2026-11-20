// LEGO Digital Designer — restored source
// LEGO::ExportCommand — exports the model (LDDApplication::Export: "EXPORT" save panel, LDraw /
//   LXFML; "export" in LDD.commands.editMode, created by EditMode::Initialize, TU 085); enabled
//   while the scene is not empty.
//
// TU 237 "ExportCommand.cpp": Mac 0x26eed0-0x26f51b, Win 0x676617-0x676730. Mac 0x26f4a0 =
// <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28)}, virtual base
//   Object. Mac vtable 0x73870c {D1 0x26f050, D0 0x26f0c0, OnFinalRelease 0x26f150, Do 0x26eff0,
//   SetEnable, GetEnable, UpdateEnable 0x26f010, OnSceneControlAttached 0x26f1e0, OnEvent19
//   0x26f1f0}; observer-in 0x73873c {19: thunk 0x26f330}. Win 0x9d9718 {Do 0x676617, …,
//   UpdateEnable 0x63120b (ICF ScreenShotCommand), 0x66e988}, observer-in 0x9d96c4 {16: 0x631956}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), Object +0x30.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class ExportCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; observes `sceneControl`, then UpdateEnable. Mac 0x26eed0, Win 0x676623
    explicit ExportCommand(BrickConnectSceneControl* sceneControl);
    virtual ~ExportCommand();                                               // Mac 0x26f050/0x26f0c0, Win sdd 0x676730

    // LDDApplication::Export.
    virtual void Do();                                                      // 3/0 Mac 0x26eff0, Win 0x676617
    // Enabled while there is a scene control whose scene is not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x26f010, Win 0x63120b
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x26f1e0, Win 0x66e988

protected:
    // The scene's structure changed: UpdateEnable.
    virtual void OnEvent19();                                               // 8/– Mac 0x26f1f0 (thunk 0x26f330), Win 0x631956
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x26f150
    virtual void OnFinalRelease();
};

} // namespace LEGO
