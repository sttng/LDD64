// LEGO Digital Designer — restored source
// LEGO::GroupToLibraryCommand — stores the selected bricks as a model in the palette's library
//   ("groupToLibrary" in LDD.commands.editMode, created by EditMode::Initialize, TU 085): the
//   bricks are copied into a private scene, saved as "<user data>/LibraryGroups/<brand>/<uuid>
//   .lxfml" (LXFML 5 with a <Brand name=…> meta) and handed to BrickPalette::AddModel. Enabled
//   while the selection is not empty.
//
// TU 239 "GroupToLibraryCommand.cpp": Mac 0x26fbd0-0x270beb, Win 0x6846af-0x684aef. The TU also
// emits the BasicArray<CommandObserver*> base-destructor helper Mac 0x270a70 (Notifier.h);
// Mac 0x270b70 = <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneControlObserver (private; Mac +0x28)}, virtual base
//   Object. Mac vtable 0x738a2c {D1 0x270710, D0 0x270770, OnFinalRelease 0x2707e0, Do 0x26fd70,
//   SetEnable, GetEnable, UpdateEnable 0x270670, OnSceneControlAttached 0x270870, OnEvent14
//   0x2706b0}; observer-in 0x738a5c {14: thunk 0x2706e0}. Win 0x9dc158 {Do 0x68481a, …,
//   UpdateEnable 0x66e2c6, 0x66e988}, observer-in 0x9dc104 {11: 0x684af0}.
// Layout (Mac): Command +0x00..+0x28, observer +0x28 (subject +0x2c), scene +0x30 (referenced),
//   Object +0x34 (sizeof 0x3c). Win: Object +0x40.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneControlObserver.h"

namespace LEGO {

class Scene;

class GroupToLibraryCommand : public Command, private BrickConnectSceneControlObserver {
public:
    // Disabled; a new (empty) scene of the scene manager's BrickConnect (NewScene(0)); observes
    // `sceneControl`, then UpdateEnable. Mac 0x26fbd0, Win 0x6846af
    explicit GroupToLibraryCommand(BrickConnectSceneControl* sceneControl);
    virtual ~GroupToLibraryCommand();                                       // Mac 0x270710/0x270770, Win sdd 0x6847fb

    // See the class comment. Nothing without a selection.
    virtual void Do();                                                      // 3/0 Mac 0x26fd70, Win 0x68481a
    // Enabled while there is a scene control with a non-empty selection.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x270670, Win 0x66e2c6
    // UpdateEnable; a new virtual, not the observer's OnAttached (see CutCommand.h). (name inferred)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control); // 7/4 Mac 0x270870, Win 0x66e988

protected:
    // The selection became empty (true) / not empty: enabled = !empty.
    virtual void OnEvent14(bool empty);                                     // 8/– Mac 0x2706b0 (thunk 0x2706e0), Win 0x684af0
    // Observer<BrickConnectSceneControl> part, then NotifierT<Command>'s. Mac 0x2707e0
    virtual void OnFinalRelease();

    RefPointer<Scene> m_scene;   // Mac +0x30 the copy target (emptied after every save)
};

} // namespace LEGO
