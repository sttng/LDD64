// LEGO Digital Designer — restored source
// LEGO::ImportCommand — imports a model into the click-stick tool ("import" in
//   LDD.commands.editMode): the tool becomes the active tool and receives the model
//   (LDDApplication::ImportWithPanel with the tool as ModelLoaderHandler); cancelled: no tool.
//
// TU 241 "ImportCommand.cpp": Mac 0x271640-0x2719eb, Win 0x66a245 / 0x62c175. The constructor is
// inline (Mac: inlined in EditMode::Initialize 0x18829b; Win copy 0x62a851). Mac 0x271970 =
// <iostream> + a header static (not code).
// RTTI: ImportCommand : Command. Mac layout: Command, scene manager +0x28, click-stick tool +0x2c
//   (neither referenced), Object +0x30; vtable 0x738fec {D1 0x271690, D0 0x271700, OnFinalRelease
//   0x271780, Do 0x271640, SetEnable, GetEnable, UpdateEnable 0x2717d0}. Win 0x9d1540 {Do
//   0x66a245, …, UpdateEnable 0x630465 (ICF: SetEnable(true))}, Object-in 0x9d152c {sdd 0x62c175}.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"

namespace LEGO {

class BrickConnectSceneManager;
class ClickStickTool;

class ImportCommand : public Command {
public:
    ImportCommand(BrickConnectSceneManager* sceneManager, ClickStickTool* clickStickTool)
        : Command(true), m_sceneManager(sceneManager), m_clickStickTool(clickStickTool) {} // Win 0x62a851
    virtual ~ImportCommand();                                               // Mac 0x271690/0x271700, Win sdd 0x62c175

    // The click-stick tool becomes the active tool and imports through the "IMPORT" open panel;
    // when that fails or is cancelled the scene manager gets no tool.
    virtual void Do();                                                      // 3/0 Mac 0x271640, Win 0x66a245
    // Always enabled. (own copy of Command's)
    virtual void UpdateEnable();                                            // 6/3 Mac 0x2717d0, Win 0x630465

protected:
    // NotifierT<Command>'s (own copy of the inherited body). Mac 0x271780
    virtual void OnFinalRelease();

private:
    BrickConnectSceneManager* m_sceneManager;     // Mac +0x28
    ClickStickTool*           m_clickStickTool;   // Mac +0x2c
};

} // namespace LEGO
