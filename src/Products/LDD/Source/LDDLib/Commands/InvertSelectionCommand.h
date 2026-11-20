// LEGO Digital Designer — restored source
// LEGO::InvertSelectionCommand — the parts of the scene that are not selected become the
//   selection ("invertSelection" in LDD.commands.editMode, created by EditMode::Initialize,
//   TU 085). Always enabled.
//
// TU 403 "InvertSelectionCommand.cpp": Mac 0x38bad0-0x38c07b, Win 0x66dab1-0x66dc2a. The TU also
// emits the BasicArray<CommandObserver*> base-destructor helper Mac 0x38bf20 (Notifier.h);
// Mac 0x38c000 = <iostream> + a header static (not code).
// RTTI: InvertSelectionCommand : Command. Mac vtable 0x7561cc {D1 0x38bd70, D0 0x38bdd0,
//   OnFinalRelease (NotifierT's), Do 0x38bc00, SetEnable, GetEnable, UpdateEnable 0x38bd50};
//   Win 0x9d8410 {Do 0x66dab1, …, UpdateEnable 0x630465}, Object-in 0x9d83fc {sdd 0x66dbb4}.
// Layout (Mac): Command +0x00..+0x28, scene control +0x28 (referenced through its
//   RenderVisualDebugger base, as BrickConnectSceneManager does), Object +0x2c (sizeof 0x34).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BrickConnectSceneControl;

class InvertSelectionCommand : public Command {
public:
    // Disabled, references `sceneControl` (must not be null), then UpdateEnable. Mac 0x38bad0,
    // Win 0x66db30
    explicit InvertSelectionCommand(BrickConnectSceneControl* sceneControl);
    virtual ~InvertSelectionCommand();                                      // Mac 0x38bd70/0x38bdd0, Win sdd 0x66dc2a

    // SetSelection(every part of the scene without the selected ones). The parts leave the
    // scene's list by swap-with-last, so the new selection's order is the original's.
    virtual void Do();                                                      // 3/0 Mac 0x38bc00, Win 0x66dab1
    // Enabled.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x38bd50, Win 0x630465

private:
    BrickConnectSceneControl* m_sceneControl;   // Mac +0x28 (referenced)
};

} // namespace LEGO
