// LEGO Digital Designer — restored source
// LEGO::NewCommand — a new model ("new" in LDD.commands.editMode, created by EditMode::Initialize,
//   TU 085): Application::CloseDocument, then the ShowStartHelpCommand it was given (the start
//   help screen with the LDD / Mindstorms / LDD Extended tabs). It follows the scene manager's
//   scene control and observes that control's brick palette: the command is disabled until a
//   palette is attached (every palette notification re-enables it).
//
// TU 243 "NewCommand.cpp": Mac 0x271a30-0x2724ab, Win 0x676764-0x6768f5. The TU also emits the
// BasicArray<CommandObserver*> base-destructor helper Mac 0x272310 (Notifier.h); Mac 0x272430 =
// <iostream> + a header static (not code).
// RTTI: vmi {Command +0, BrickPaletteObserver (Mac +0x28, Win +0x2c), BrickConnectSceneManager-
//   Observer (Mac +0x30)}, virtual base Object. Mac vtable 0x73917c {D1 0x271dd0, D0 0x271e30,
//   OnFinalRelease 0x271ea0, Do 0x271b90, SetEnable, GetEnable, UpdateEnable 0x271bc0, then the
//   primary entries of the observer overriders 0x271f70 / 0x271f80 / 0x271f90 / 0x271fa0 (palette
//   OnAttached, OnDetached, OnBagAdded, OnPaletteChanged: all UpdateEnable), OnAttached(scene
//   manager) 0x271be0, OnDetached 0x271c60, OnSceneControlChanged 0x271d00}; palette-in 0x7391c0
//   {3: 0x2720a0, 4: 0x2720c0, 5: 0x2720e0, 9: 0x272100}; scene-manager-in 0x7391f4 {3: 0x271c20,
//   4: 0x271cb0, 5: 0x271d60}. Win 0x9d9794 {Do 0x676764, …, UpdateEnable 0x630465}; palette-in
//   0x9d9774 {0-2: 0x631d53, 6: 0x631956}; scene-manager-in 0x9d9760 {0x676911, 0x66e61e, 0x66e804}.
// Layout (Mac): Command +0x00..+0x28, palette observer +0x28 (subject +0x2c), scene-manager
//   observer +0x30 (subject +0x34), start-help command +0x38 (referenced), Object +0x3c.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BrickPalette.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class BrickConnectSceneControl;

class NewCommand : public Command,
                   private BrickPaletteObserver,
                   private BrickConnectSceneManagerObserver {
public:
    // Disabled; references `showStartHelp`, observes `sceneManager` (attaching follows its scene
    // control's palette). Mac 0x271a30, Win 0x676781
    NewCommand(BrickConnectSceneManager* sceneManager, Command* showStartHelp);
    virtual ~NewCommand();                                                  // Mac 0x271dd0/0x271e30, Win sdd 0x6768d6

    // When Application::CloseDocument agrees (slot 17): the start-help command's Do.
    virtual void Do();                                                      // 3/0 Mac 0x271b90, Win 0x676764
    // Enabled.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x271bc0, Win 0x630465

protected:
    // ---- BrickPaletteObserver: each one UpdateEnable ----
    virtual void OnAttached(BrickPalette* palette);                         // 7  Mac 0x271f70 (thunk 0x2720a0), Win 0x631d53
    virtual void OnDetached(BrickPalette* palette);                         // 8  Mac 0x271f80 (thunk 0x2720c0), Win 0x631d53
    virtual void OnBagAdded(const BrickPalette::Bag* bag);                  // 9  Mac 0x271f90 (thunk 0x2720e0), Win 0x631d53
    virtual void OnPaletteChanged();                                        // 10 Mac 0x271fa0 (thunk 0x272100), Win 0x631956
    // ---- BrickConnectSceneManagerObserver: follow the scene control's palette ----
    // Observes the palette of the manager's scene control (when it has one).
    virtual void OnAttached(BrickConnectSceneManager* sceneManager);        // 11 Mac 0x271be0 (thunk 0x271c20), Win 0x676911
    // Leaves the palette.
    virtual void OnDetached(BrickConnectSceneManager* sceneManager);        // 12 Mac 0x271c60 (thunk 0x271cb0), Win 0x66e61e
    // Leaves the palette, then observes the new control's one.
    virtual void OnSceneControlChanged(BrickConnectSceneControl* previous,
                                       BrickConnectSceneControl* current);  // 13 Mac 0x271d00 (thunk 0x271d60), Win 0x66e804
    // Both observer parts, then NotifierT<Command>'s. Mac 0x271ea0, Win 0x6768f5
    virtual void OnFinalRelease();

    RefPointer<Command> m_showStartHelp;   // Mac +0x38
};

} // namespace LEGO
