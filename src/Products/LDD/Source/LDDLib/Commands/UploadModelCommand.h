// LEGO Digital Designer — restored source
// LEGO::UploadModelCommand — "upload model" (Lua ldd.commands.common.uploadToFactory, the
//   upload buttons of EditMode / ViewMode): asks to save the model (with the building guide
//   check when the server wants one), and when the saved model is unmodified starts the upload
//   through UploadToFactoryCommand with the UploadModelDialog. Disabled without an upload URL
//   for the application's brand.
//
// TU 261 "UploadModelCommand.cpp" (Mac 0x278770-0x27997b; Win 0x6316db-0x631ad3). The TU also
// emits UploadToFactoryCommand's destructors / OnFinalRelease / observer thunks and the
// NewContentCheckerObserver and ObserverN<NewContentChecker, …> instances. Static initialiser
// Mac 0x279900: <iostream> + a header static.
// RTTI (Mac): si UploadToFactoryCommand; operator new 0x5c. Mac vtable 0x73b16c: D1 0x2793d0 /
//   D0 0x279430, Do 3 0x278810, UpdateEnable 6 0x278900, GetDialog 12 0x278950.
// Win vtable 0x9d2f98 {Do 0x6316db, SetEnable, GetEnable, UpdateEnable 0x63177d,
//   OnSceneControlAttached 0x66e988, GetDialog 0x6317a8}, sdd 0x631ad3; constructor 0x631979.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "UploadToFactoryCommand.h"

namespace LEGO {

class BrickConnectSceneControl;
class ModalControl;

class UploadModelCommand : public UploadToFactoryCommand {
public:
    // `editCommand`: the command that returns to EditMode (AppMain passes its
    // ModeNavigationCommand for EditMode). Mac 0x278770, Win 0x631979
    UploadModelCommand(BrickConnectSceneControl* sceneControl, Command* editCommand);
    virtual ~UploadModelCommand();                                          // Mac 0x2793d0/0x279430, Win sdd 0x631ad3

    // The building-guide check and "unsaved changes" follow the server's
    // generateBGBeforeUpload while the application asks to save; an unmodified scene then
    // starts the new-content check (the dialog follows); both switches are reset.
    // Mac 0x278810, Win 0x6316db
    virtual void Do();                                                      // 3/0
    // Disabled without an upload URL, else UploadToFactoryCommand's rule.
    // Mac 0x278900, Win 0x63177d
    virtual void UpdateEnable();                                            // 6/3
    // The upload dialog. Mac 0x278950, Win 0x6317a8
    virtual ModalControl* GetDialog();                                      // 12/5
};

} // namespace LEGO
