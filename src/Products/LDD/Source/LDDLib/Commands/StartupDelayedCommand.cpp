// LEGO Digital Designer — restored source
// TU 259 "StartupDelayedCommand.cpp": Mac 0x277580-0x277d4b, Win 0x6090aa / 0x5ca0b4 / 0x5ca0c7.
// Not code: construction-vtable thunks, Mac 0x277cd0 = <iostream> + a header static.
#include "StartupDelayedCommand.h"
#include "EditMode.h"
#include "Mode.h"
#include "LDDApplication.h"
#include "Preferences.h"
#include "Path.h"
#include "Window.h"

namespace LEGO {

// Mac 0x277930 (D1, body 0x277bd0), 0x277990 (D0), Win 0x5ca0b4 (sdd 0x5ca0c7)
StartupDelayedCommand::~StartupDelayedCommand()
{
}

// Mac 0x277a00, Win 0x630465
void StartupDelayedCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x277580, Win 0x6090aa
void StartupDelayedCommand::Do()
{
    if (m_startMode) {
        m_startMode->Exit();
        m_startMode = 0;
    }
    LDDApplication* application = LDDApplication::GetInstance();
    application->GetWindow()->SetModalState(false);
    m_editMode->Enter();
    LDDApplication::GetInstance()->CloseDocument();

    const String& document = Application::GetInstance()->GetDocumentToOpen();
    if (!document.IsEmpty()) {
        // A document handed over by the system (double-click, drop on the icon).
        Path path(String(), 0);
        path.Append(document);
        LDDApplication::GetInstance()->OpenDocumentFile(path);
        return;
    }
    bool loadMostRecent;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        loadMostRecent = preferences->GetBool(String("LoadMostRecentModel"), false);
    }
    if (!loadMostRecent) {
        bool showStartHelp;
        {
            RefPointer<Preferences> preferences = Preferences::GetInstance();
            showStartHelp = preferences->GetBool(String("ShowStartHelp"), true);
        }
        if (showStartHelp)
            m_editMode->ShowStartHelp();
        return;
    }
    const BasicArray<String>& recent = LDDApplication::GetInstance()->GetRecentModels();
    if (recent.IsEmpty())
        return;
    Path path(String(), 0);
    path.Append(recent[0]);
    LDDApplication::GetInstance()->OpenDocumentFile(path);
}

} // namespace LEGO
