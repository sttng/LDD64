// LEGO Digital Designer — restored source
// LEGO::UploadToFactoryCommand — base of the upload commands: enabled while LDD is online and
//   the scene is not empty; an upload first checks the LEGO servers for new content
//   (NewContentChecker) with the upload dialog shown as a "please wait" box, then shows the
//   real dialog (GetDialog) or an alert about the new content / the failed check.
//
// TU 262 "UploadToFactoryCommand.cpp" (Mac 0x279980-0x27abbb; Win 0x684c9a-0x685206). The
// destructors and OnFinalRelease are emitted weak in UploadModelCommand.cpp (TU 261, Mac
// 0x278970-0x279300, the NewContentCheckerObserver / ObserverN instances with them). Static
// initialiser Mac 0x27ab40: <iostream> + a header static.
// RTTI (Mac): vmi {Command +0, BrickConnectSceneControlObserver +0x28, LDDApplicationObserver
//   +0x30, NewContentCheckerObserver +0x38}, virtual base Object (+0x54, sizeof 0x5c).
// Mac vtable 0x73b30c (13 entries): D1 0x278970 / D0 0x2789d0, OnFinalRelease 2 0x278a40, Do 3
//   (Command's), SetEnable, GetEnable, UpdateEnable 6 0x27a570, OnSceneControlAttached 7
//   0x278b50, the primary entries of the observer overriders OnEvent19 8 (0x278b60, thunk
//   0x278c80), OnBrandChanged 9 (0x278b70, thunk 0x278d90), OnOnlineStatusChanged 10 (0x278b80,
//   thunk 0x278db0), OnNewContentChecked 11 (0x27a5c0, thunk 0x27aad0), GetDialog 12 (pure).
// Win: UpdateEnable 0x684c9a, OnSceneControlAttached 0x66e988 (ICF), OnNewContentChecked
//   0x685013, GetUploadModelDialog 0x684f3d.
// Layout (Mac): Command +0x00..+0x28, BrickConnectSceneControlObserver +0x28 (scene control
//   +0x2c), LDDApplicationObserver +0x30 (application +0x34), NewContentCheckerObserver +0x38
//   (subjects +0x3c), checking +0x4c, edit command +0x50, Object +0x54.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Command.h"
#include "Brand.h"
#include "BrickConnectSceneControlObserver.h"
#include "LDDApplication.h"
#include "NewContentChecker.h"

namespace LEGO {

class BrickConnectSceneControl;
class ModalControl;
class UploadModelDialog;

class UploadToFactoryCommand : public Command,
                               public BrickConnectSceneControlObserver,
                               public LDDApplicationObserver,
                               public NewContentCheckerObserver {
public:
    // Watches the scene control, the application and the new-content checker, then
    // UpdateEnable. `editCommand` is kept (+0x50) but not used. Mac 0x279980 (C2 with the VTT)
    UploadToFactoryCommand(BrickConnectSceneControl* sceneControl, Command* editCommand);
    virtual ~UploadToFactoryCommand();                                       // Mac 0x278970/0x2789d0, Win sdd 0x631ab4

    // Enabled while the application is online and the scene control's scene is not empty.
    // Mac 0x27a570, Win 0x684c9a
    virtual void UpdateEnable();                                             // 6/3
    // UpdateEnable. Mac 0x278b50, Win 0x66e988 (ICF)
    virtual void OnSceneControlAttached(BrickConnectSceneControl* control);  // 7/4

protected:
    // ---- observers: UpdateEnable ----
    // (BrickConnectSceneControlObserver slot 19: the scene changed) Mac 0x278b60 (thunk 0x278c80)
    virtual void OnEvent19();                                                // 8
    // (LDDApplicationObserver slots 6 / 9) Mac 0x278b70 / 0x278b80 (thunks 0x278d90 / 0x278db0)
    virtual void OnBrandChanged(Brand brand);                                // 9
    virtual void OnOnlineStatusChanged(bool online);                         // 10
    using LDDApplicationObserver::OnAttached;
    using BrickConnectSceneControlObserver::OnAttached;
    using NewContentCheckerObserver::OnAttached;
    // ---- NewContentCheckerObserver: while checking: the wait box closes; no new content
    //      (0): the dialog (GetDialog) shows; else an alert (style 1, "DIALOGWARNING", "OK")
    //      with NEWAPPLICATION (1), NEWPALETTES (2), NEWBRICKS (3) or NEWCONTENTCHECKERROR (4).
    //      Not checking any more. Mac 0x27a5c0 (thunk 0x27aad0), Win 0x685013
    virtual void OnNewContentChecked(UInt32 status);                         // 11
public:
    // The dialog of the upload. Mac slot 12 / Win slot 5 (pure)
    virtual ModalControl* GetDialog() = 0;                                   // 12/5

protected:
    // The observers are left, then Command's part. Mac 0x278a40 (thunks 0x278c60 / 0x278d70 /
    // 0x278ea0 / 0x278fc0)
    virtual void OnFinalRelease();

    // The upload dialog of the ObjectRegistry ('BUMD'): created (UploadModelDialog of the
    // scene control's manager, Initialize) and registered on first use; null when the
    // registered object is not one. Mac 0x27a8c0, Win 0x684f3d
    UploadModelDialog* GetUploadModelDialog();
    // Starts the new-content check against the scene's brick database version and, while
    // checking, shows the upload dialog as a wait box. Mac 0x27aaf0
    void StartCheck();

    BrickConnectSceneControl* GetSceneControl() const
    {
        return BrickConnectSceneControlObserver::GetSubject();
    }

    bool     m_checking;      // Mac +0x4c an upload waits for the new-content check
    Command* m_editCommand;   // Mac +0x50 (not referenced)
                              // Object +0x54
};

} // namespace LEGO
