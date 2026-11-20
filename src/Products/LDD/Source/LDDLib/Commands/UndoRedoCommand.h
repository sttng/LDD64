// LEGO Digital Designer — restored source
// LEGO::UndoRedoCommand — undo (true) or redo (false) on the undo manager of the scene manager's
//   current scene control ("undo" / "redo" in LDD.commands.editMode); enabled while the undo
//   (redo) stack is not empty.
//
// TU 260 "UndoRedoCommand.cpp": Mac 0x277d50-0x27876b, Win 0x66ea4e-0x66ecd0. The TU also emits
// the BasicArray<CommandObserver*> helper Mac 0x2785d0 (Notifier.h). Mac 0x2786f0 = <iostream> +
// a header static (not code).
// RTTI: vmi {Command +0, BrickConnectSceneManagerObserver (Mac +0x28, Win +0x2c),
//   UndoManagerObserver (Mac +0x30, Win +0x38)}, virtual base Object.
//   Mac vtable 0x73adcc {D1 0x2780c0, D0 0x278120, OnFinalRelease 0x278190, Do 0x277ee0,
//   SetEnable, GetEnable, UpdateEnable 0x277f10, then the Itanium primary entries of the
//   observer overriders: OnSceneControlChanged 0x277f80, OnUndoAvailable 0x278250,
//   OnRedoAvailable 0x278270, OnAttached(UndoManager) 0x278040}; scene-manager observer-in
//   0x73ae04 {5: 0x277fe0}; undo observer-in 0x73ae2c {3: 0x278080, 5: 0x278470, 6: 0x2784a0}.
//   Win 0x9d8800 {Do 0x66ea4e, …, UpdateEnable 0x66ea94}; 0x9d87ec {2: 0x66ecad}; 0x9d87d8
//   {0: 0x66ea68, 2: 0x66ebbb, 3: 0x66ebcc}.
// Layout (Mac): Command +0x00..+0x28, scene-manager observer +0x28 (subject +0x2c), undo
//   observer +0x30 (subject +0x34), scene manager +0x38, undo flag +0x3c, Object +0x40.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "UndoManager.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class BrickConnectSceneControl;

class UndoRedoCommand : public Command,
                        private BrickConnectSceneManagerObserver,
                        private UndoManagerObserver {
public:
    // Disabled; observes `sceneManager` and the undo manager of its scene control (when it has
    // one; attaching sets the enable state). Mac 0x277d50, Win 0x66eade
    UndoRedoCommand(BrickConnectSceneManager* sceneManager, bool undo);
    virtual ~UndoRedoCommand();                                             // Mac 0x2780c0/0x278120, Win sdd 0x66ec72

    // UndoManager::Undo / Redo (nothing without an undo manager).
    virtual void Do();                                                      // 3/0 Mac 0x277ee0, Win 0x66ea4e
    // Enabled while the scene manager has a scene control whose undo (redo) stack is not empty.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x277f10, Win 0x66ea94

protected:
    // ---- BrickConnectSceneManagerObserver: follow the new control's undo manager ----
    virtual void OnSceneControlChanged(BrickConnectSceneControl* previous,
                                       BrickConnectSceneControl* current); // 7 Mac 0x277f80 (thunk 0x277fe0), Win 0x66ecad
    // ---- UndoManagerObserver ----
    virtual void OnAttached(UndoManager* manager);                          // 10 Mac 0x278040 (thunk 0x278080), Win 0x66ea68
    virtual void OnUndoAvailable(bool available);                           // 8 Mac 0x278250 (thunk 0x278470), Win 0x66ebbb
    virtual void OnRedoAvailable(bool available);                           // 9 Mac 0x278270 (thunk 0x2784a0), Win 0x66ebcc
    // Both observer parts, then NotifierT<Command>'s. Mac 0x278190, Win 0x66ec91
    virtual void OnFinalRelease();

    using BrickConnectSceneManagerObserver::OnAttached;

    RefPointer<BrickConnectSceneManager> m_sceneManager;   // Mac +0x38
    bool                                 m_undo;           // Mac +0x3c
};

} // namespace LEGO
