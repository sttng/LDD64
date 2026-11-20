// LEGO Digital Designer — restored source
// TU 261 "UploadModelCommand.cpp" (Mac 0x278770-0x27997b; Win 0x6316db-0x631ad3):
// UploadModelCommand, and UploadToFactoryCommand's destructor, OnFinalRelease and observer
// forwards (emitted here in the original). Static initialiser Mac 0x279900: <iostream> + a
// header static.
#include "UploadModelCommand.h"

#include "BrickConnectSceneControl.h"
#include "LDDApplication.h"
#include "LDDWebCommunicator.h"
#include "UploadModelDialog.h"

#include <iostream>

namespace LEGO {

// ---- UploadToFactoryCommand (weak copies in this TU) --------------------------------------------

// Mac 0x278970 (D1) / 0x2789d0 (D0), thunks 0x278b90-0x278f40; Win sdd 0x631ab4
UploadToFactoryCommand::~UploadToFactoryCommand()
{
}

// Mac 0x278a40
void UploadToFactoryCommand::OnFinalRelease()
{
    NewContentCheckerObserver::OnFinalRelease();
    LDDApplicationObserver::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x278b50, Win 0x66e988 (ICF)
void UploadToFactoryCommand::OnSceneControlAttached(BrickConnectSceneControl*)
{
    UpdateEnable();
}

// Mac 0x278b60 (thunk 0x278c80)
void UploadToFactoryCommand::OnEvent19()
{
    UpdateEnable();
}

// Mac 0x278b70 (thunk 0x278d90)
void UploadToFactoryCommand::OnBrandChanged(Brand)
{
    UpdateEnable();
}

// Mac 0x278b80 (thunk 0x278db0)
void UploadToFactoryCommand::OnOnlineStatusChanged(bool)
{
    UpdateEnable();
}

// ---- UploadModelCommand ----------------------------------------------------------------------------

// Mac 0x278770, Win 0x631979
UploadModelCommand::UploadModelCommand(BrickConnectSceneControl* sceneControl, Command* editCommand)
    : UploadToFactoryCommand(sceneControl, editCommand)
{
}

// Mac 0x2793d0 (D1) / 0x279430 (D0), thunks 0x2794a0-0x279770; Win sdd 0x631ad3
UploadModelCommand::~UploadModelCommand()
{
}

// Mac 0x278810, Win 0x6316db
void UploadModelCommand::Do()
{
    bool generateBuildingGuide = LDDWebCommunicator::GetInstance()->GetGenerateBGBeforeUpload();
    LDDApplication* application = LDDApplication::GetInstance();
    if (!generateBuildingGuide)
        application->SetCheckBuildingGuide(false);
    else
        application->SetCheckBuildingGuide(true);
    LDDApplication::GetInstance()->SetUnsavedChanges(generateBuildingGuide);
    if (LDDApplication::GetInstance()->QuerySaveChanges()) {
        if (!GetSceneControl()->IsModified()) {
            m_checking = true;
            LDDApplication::GetInstance()->SetCheckBuildingGuide(
                LDDWebCommunicator::GetInstance()->GetGenerateBGBeforeUpload());
            StartCheck();
        }
    }
    LDDApplication::GetInstance()->SetCheckBuildingGuide(false);
    LDDApplication::GetInstance()->SetUnsavedChanges(false);
}

// Mac 0x278900, Win 0x63177d
void UploadModelCommand::UpdateEnable()
{
    if (LDDWebCommunicator::GetInstance()->GetUploadModelURL().IsEmpty()) {
        SetEnable(false);
        return;
    }
    UploadToFactoryCommand::UpdateEnable();
}

// Mac 0x278950, Win 0x6317a8
ModalControl* UploadModelCommand::GetDialog()
{
    return GetUploadModelDialog();
}

} // namespace LEGO
