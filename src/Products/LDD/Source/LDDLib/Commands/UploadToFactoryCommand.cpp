// LEGO Digital Designer — restored source
// TU 262 "UploadToFactoryCommand.cpp" (Mac 0x279980-0x27abbb; Win 0x684c9a-0x685206): the
// constructor, UpdateEnable, the new-content result and the upload dialog of
// UploadToFactoryCommand (the destructors / OnFinalRelease / observer forwards are in
// UploadModelCommand.cpp, TU 261). Static initialiser Mac 0x27ab40: <iostream> + a header static.
#include "UploadToFactoryCommand.h"

#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickDatabase.h"
#include "LDDApplication.h"
#include "UploadModelDialog.h"
#include "ObjectRegistry.h"
#include "TextLocale.h"

#include <iostream>

namespace LEGO {

// Mac 0x279980 (C2 with the VTT; the observers' Attach inlined)
UploadToFactoryCommand::UploadToFactoryCommand(BrickConnectSceneControl* sceneControl,
                                               Command* editCommand)
    : Command(false, false), m_checking(false), m_editCommand(editCommand)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    LDDApplicationObserver::Attach(LDDApplication::GetInstance());
    NewContentCheckerObserver::Attach(NewContentChecker::GetInstance());
    UpdateEnable();
}

// Mac 0x27a570, Win 0x684c9a
void UploadToFactoryCommand::UpdateEnable()
{
    bool enable;
    if (!LDDApplication::GetInstance()->IsOnline() || !GetSceneControl())
        enable = false;
    else
        enable = !GetSceneControl()->IsSceneEmpty();
    SetEnable(enable);
}

// Mac 0x27a5c0 (thunk 0x27aad0), Win 0x685013
void UploadToFactoryCommand::OnNewContentChecked(UInt32 status)
{
    if (m_checking) {
        GetUploadModelDialog()->EndModal();
        if (status == kNoNewContent) {
            GetDialog()->ShowModal();
        } else {
            LocalizedString message;
            switch (status) {
            case 1:
                message.Load(String("NEWAPPLICATION"));
                break;
            case 2:
                message.Load(String("NEWPALETTES"));
                break;
            case 3:
                message.Load(String("NEWBRICKS"));
                break;
            case 4:
                message.Load(String("NEWCONTENTCHECKERROR"));
                break;
            }
            LDDApplication::GetInstance()->Alert(1, message,
                                                 LocalizedString(String("DIALOGWARNING")),
                                                 LocalizedString(String("OK")), String(), String());
        }
    }
    m_checking = false;
}

// Mac 0x27a8c0, Win 0x684f3d
UploadModelDialog* UploadToFactoryCommand::GetUploadModelDialog()
{
    RefPointer<UploadModelDialog> dialog;
    RefPointer<Object> object = ObjectRegistry::GetInstance()->Find(UploadModelDialog::kRegistryId);
    if (!object.Get()) {
        BrickConnectSceneControl* sceneControl = GetSceneControl();
        dialog = new UploadModelDialog(sceneControl->GetManager(), sceneControl);
        dialog->Initialize();
        ObjectRegistry::GetInstance()->Register(UploadModelDialog::kRegistryId, dialog.Get());
    } else {
        dialog = dynamic_cast<UploadModelDialog*>(object.Get());
        if (!dialog.Get())
            return 0;
    }
    // the registry keeps the dialog
    return dialog.Get();
}

// Mac 0x27aaf0
void UploadToFactoryCommand::StartCheck()
{
    NewContentChecker::GetInstance()->Check(
        GetSceneControl()->GetSceneManager()->GetDatabase()->GetVersion());
    if (m_checking)
        GetUploadModelDialog()->ShowWaiting();
}

} // namespace LEGO
