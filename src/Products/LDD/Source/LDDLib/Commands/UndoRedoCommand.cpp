// LEGO Digital Designer — restored source
// TU 260 "UndoRedoCommand.cpp": Mac 0x277d50-0x27876b, Win 0x66ea4e-0x66ecd0.
// Not code: Mac 0x2786f0 = <iostream> + a header static.
#include "UndoRedoCommand.h"
#include "BrickConnectSceneControl.h"

namespace LEGO {

// Mac 0x277d50, Win 0x66eade
UndoRedoCommand::UndoRedoCommand(BrickConnectSceneManager* sceneManager, bool undo)
    : Command(false, false), m_sceneManager(sceneManager), m_undo(undo)
{
    BrickConnectSceneManagerObserver::Attach(sceneManager);
    if (sceneManager->GetSceneControl())
        UndoManagerObserver::Attach(sceneManager->GetSceneControl()->GetUndoManager());
}

// Mac 0x2780c0 (D1) / 0x278120 (D0), Win sdd 0x66ec72
UndoRedoCommand::~UndoRedoCommand()
{
}

// Mac 0x278190 (thunks 0x278360, 0x278450, 0x2785b0), Win 0x66ec91
void UndoRedoCommand::OnFinalRelease()
{
    UndoManagerObserver::OnFinalRelease();
    BrickConnectSceneManagerObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x277ee0, Win 0x66ea4e
void UndoRedoCommand::Do()
{
    UndoManager* manager = UndoManagerObserver::GetSubject();
    if (!manager)
        return;
    if (m_undo)
        manager->Undo();
    else
        manager->Redo();
}

// Mac 0x277f10, Win 0x66ea94
void UndoRedoCommand::UpdateEnable()
{
    BrickConnectSceneManager* sceneManager = BrickConnectSceneManagerObserver::GetSubject();
    if (!sceneManager || !sceneManager->GetSceneControl()) {
        SetEnable(false);
        return;
    }
    UndoManager* manager = sceneManager->GetSceneControl()->GetUndoManager();
    SetEnable(m_undo ? manager->CanUndo() : manager->CanRedo());
}

// Mac 0x277f80 (thunk 0x277fe0), Win 0x66ecad
void UndoRedoCommand::OnSceneControlChanged(BrickConnectSceneControl* /*previous*/,
                                            BrickConnectSceneControl* current)
{
    UndoManagerObserver::Detach();
    if (current)
        UndoManagerObserver::Attach(current->GetUndoManager());
}

// Mac 0x278040 (thunk 0x278080), Win 0x66ea68
void UndoRedoCommand::OnAttached(UndoManager* manager)
{
    SetEnable(m_undo ? manager->CanUndo() : manager->CanRedo());
}

// Mac 0x278250 (thunk 0x278470), Win 0x66ebbb
void UndoRedoCommand::OnUndoAvailable(bool available)
{
    if (m_undo)
        SetEnable(available);
}

// Mac 0x278270 (thunk 0x2784a0), Win 0x66ebcc
void UndoRedoCommand::OnRedoAvailable(bool available)
{
    if (!m_undo)
        SetEnable(available);
}

} // namespace LEGO
