// LEGO Digital Designer — restored source
// TU 256 "ScreenShotCommand.cpp": Mac 0x275c10-0x276b6b, Win 0x63120b-0x6316bc.
// Not code: Mac 0x275c10 / 0x276670 (array helpers), construction-vtable thunks, 0x276af0 =
// <iostream> + a header static.
#include "ScreenShotCommand.h"
#include "LDDApplication.h"
#include "LDDUtilities.h"
#include "Platform.h"
#include "TextLocale.h"
#include "Directory.h"
#include "Path.h"
#include "Window.h"

namespace LEGO {

// Mac 0x275d50, Win 0x631628
ScreenShotCommand::ScreenShotCommand(BrickConnectSceneControl* control)
    : Command(false)
{
    BrickConnectSceneControlObserver::Attach(control);
}

// Mac 0x2766e0 (D1), 0x276860 (D0), Win 0x6315b6 (sdd 0x6316bc)
ScreenShotCommand::~ScreenShotCommand()
{
}

// Mac 0x2765f0, Win 0x6315ae
void ScreenShotCommand::Do()
{
    TakeScreenShot(0);
}

// Mac 0x275e70, Win 0x63120b
void ScreenShotCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    SetEnable(control != 0 && !control->IsSceneEmpty());
}

// Mac 0x275eb0, Win 0x63122e
void ScreenShotCommand::TakeScreenShot(RenderQueue* queue)
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return;
    Path path(String(), 0);
    Platform::FileTypeArray types;
    types.PushBack(Platform::FileType(String("PNG Image (*.png)"), String("png")));
    UInt32 typeIndex = 0;
    Platform* platform = Platform::GetInstance();
    // The document's directory, else Platform directory 5 (created).
    RefPointer<Directory> directory = LDDApplication::GetInstance()->GetDocumentPath().GetParentDirectory();
    if (!directory)
        directory = platform->GetSystemDirectory(5, true);
    String name = LDDApplication::GetInstance()->GetDocumentName();
    Window* window = LDDApplication::GetInstance()->GetWindow();
    bool chosen = platform->GetSavePath(path, directory, name, types, &typeIndex, window,
                                        LocalizedString(String("SCREENSHOTTITLE")));
    if (!chosen)
        return;
    if (SaveScreenshot(control, path, Vector2(), 0, false, queue, false))
        return;
    Application::GetInstance()->Alert(2, LocalizedString(String("SCREENSHOTERROR")),
                                      LocalizedString(String("SCREENSHOTTITLE")),
                                      LocalizedString(String("OK")), String(), String());
}

// Mac 0x276610 (thunk 0x276620), Win 0x631d53
void ScreenShotCommand::OnAttached(BrickConnectSceneControl*)
{
    UpdateEnable();
}

// Mac 0x276640 (thunk 0x276650), Win 0x631956
void ScreenShotCommand::OnEvent19()
{
    UpdateEnable();
}

// Mac 0x276a20 (thunks 0x276ab0, 0x276ad0)
void ScreenShotCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

} // namespace LEGO
